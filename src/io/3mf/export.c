/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "3mf.h"
#include "../common/primitive.h"
#include "../common/text_number.h"
#include "../common/zip.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AK_3MF_MAX_NODE_DEPTH 512u

typedef struct AK3MFRows {
  AkAccessor *accessor;
  float      *scratch;
  uint32_t    componentCount;
  bool        direct;
} AK3MFRows;

typedef struct AK3MFBuffer {
  char    *data;
  size_t   len;
  size_t   cap;
  AkResult result;
} AK3MFBuffer;

typedef struct AK3MFExportState {
  AkDoc           *doc;
  AkPrintDocument *print;
  AK3MFBuffer      resources;
  AK3MFBuffer      build;
  uint32_t         objectCount;
  uint32_t         nextObjectId;
  AkResult         result;
  bool             usesMaterialExtension;
  bool             usesSliceExtension;
  bool             usesBeamLatticeExtension;
  bool             usesBeamBallExtension;
  bool             usesBooleanExtension;
  bool             usesDisplacementExtension;
  bool             usesVolumetricExtension;
  bool             suppressBuildItems;
} AK3MFExportState;

typedef struct AK3MFDisplacementWrite {
  const AkPrintDisplacementTriangle *triangle;
  uint32_t                           remaining;
} AK3MFDisplacementWrite;

static
bool
ak_3mf_buf_reserve(AK3MFBuffer * __restrict buf, size_t extra) {
  char  *data;
  size_t newCap;

  if (buf->result != AK_OK)
    return false;
  if (extra <= buf->cap - buf->len)
    return true;

  newCap = buf->cap ? buf->cap * 2u : 4096u;
  while (extra > newCap - buf->len) {
    if (newCap > SIZE_MAX / 2u) {
      buf->result = AK_ERR;
      return false;
    }
    newCap *= 2u;
  }

  data = realloc(buf->data, newCap);
  if (!data) {
    buf->result = AK_ERR;
    return false;
  }

  buf->data = data;
  buf->cap  = newCap;
  return true;
}

static
void
ak_3mf_buf_raw(AK3MFBuffer * __restrict buf,
               const void  * __restrict data,
               size_t                   len) {
  if (!ak_3mf_buf_reserve(buf, len))
    return;

  memcpy(buf->data + buf->len, data, len);
  buf->len += len;
}

static
void
ak_3mf_buf_lit(AK3MFBuffer * __restrict buf,
               const char  * __restrict lit) {
  ak_3mf_buf_raw(buf, lit, strlen(lit));
}

static
void
ak_3mf_buf_ch(AK3MFBuffer * __restrict buf, char ch) {
  if (!ak_3mf_buf_reserve(buf, 1u))
    return;

  buf->data[buf->len++] = ch;
}

static
void
ak_3mf_buf_attr(AK3MFBuffer * __restrict buf,
                const char  * __restrict value) {
  const unsigned char *it;

  if (!value)
    return;

  for (it = (const unsigned char *)value; *it; it++) {
    switch (*it) {
      case '&':
        ak_3mf_buf_lit(buf, "&amp;");
        break;
      case '<':
        ak_3mf_buf_lit(buf, "&lt;");
        break;
      case '"':
        ak_3mf_buf_lit(buf, "&quot;");
        break;
      case '\'':
        ak_3mf_buf_lit(buf, "&apos;");
        break;
      default:
        ak_3mf_buf_ch(buf, (char)*it);
        break;
    }
  }
}

static
void
ak_3mf_buf_u32(AK3MFBuffer * __restrict buf, uint32_t value) {
  char  tmp[24];
  char *end;

  end = ak_io_text_format_uint64(tmp, value);
  ak_3mf_buf_raw(buf, tmp, (size_t)(end - tmp));
}

static
void
ak_3mf_buf_float(AK3MFBuffer * __restrict buf, float value) {
  char   tmp[48];
  int    len;
  size_t outLen;

  if (!isfinite(value)) {
    buf->result = AK_ERR;
    return;
  }

  if (ak_io_text_format_fixed_float(tmp, sizeof(tmp), value, 6u, &outLen)) {
    ak_3mf_buf_raw(buf, tmp, outLen);
    return;
  }

  len = snprintf(tmp, sizeof(tmp), "%.6g", (double)value);
  if (len <= 0 || (size_t)len >= sizeof(tmp)) {
    buf->result = AK_ERR;
    return;
  }

  outLen = (size_t)len;
  if (!ak_io_text_normalize_number(tmp, &outLen)) {
    buf->result = AK_ERR;
    return;
  }

  ak_3mf_buf_raw(buf, tmp, outLen);
}

static
bool
ak_3mf_path_is_root_model(const char * __restrict path) {
  if (!path)
    return true;

  while (*path == '/' || *path == '\\')
    path++;
  return strcmp(path, "3D/3dmodel.model") == 0;
}

static
const AkPrintSliceObject*
ak_3mf_first_slice_object(const AkPrintDocument * __restrict print) {
  return print ? print->sliceObjects : NULL;
}

static
const AkPrintSliceObject*
ak_3mf_find_slice_object(const AkPrintDocument * __restrict print,
                         uint32_t                            objectId) {
  const AkPrintSliceObject *object;

  for (object = print ? print->sliceObjects : NULL; object; object = object->next) {
    if (object->objectId == objectId)
      return object;
  }

  return NULL;
}

static
const AkPrintSliceObject*
ak_3mf_slice_object_for_export(AK3MFExportState * __restrict st,
                               uint32_t                      objectId) {
  const AkPrintSliceObject *object;

  object = ak_3mf_find_slice_object(st ? st->print : NULL, objectId);
  if (object && ak_3mf_path_is_root_model(object->path))
    return object;

  if (st
      && st->print
      && st->print->sliceObjectCount == 1u
      && st->objectCount == 0u) {
    object = ak_3mf_first_slice_object(st->print);
    if (object && ak_3mf_path_is_root_model(object->path))
      return object;
  }

  return NULL;
}

static
const AkPrintBeamLattice*
ak_3mf_first_beam_lattice(const AkPrintDocument * __restrict print) {
  return print ? print->beamLattices : NULL;
}

static
bool
ak_3mf_uses_beam_ball_extension(const AkPrintDocument * __restrict print) {
  const AkPrintBeamLattice *lattice;

  if (!print)
    return false;
  if (print->beamBallCount > 0u)
    return true;

  for (lattice = print->beamLattices; lattice; lattice = lattice->next) {
    if (lattice->ballMode
        || (lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_BALL_RADIUS) != 0u)
      return true;
  }

  return false;
}

static
const AkPrintBeamLattice*
ak_3mf_find_beam_lattice(const AkPrintDocument * __restrict print,
                         uint32_t                            objectId) {
  const AkPrintBeamLattice *lattice;

  for (lattice = print ? print->beamLattices : NULL; lattice; lattice = lattice->next) {
    if (lattice->objectId == objectId)
      return lattice;
  }

  return NULL;
}

static
const AkPrintBeamLattice*
ak_3mf_beam_lattice_for_export(AK3MFExportState * __restrict st,
                               uint32_t                      objectId) {
  const AkPrintBeamLattice *lattice;

  lattice = ak_3mf_find_beam_lattice(st ? st->print : NULL, objectId);
  if (lattice && ak_3mf_path_is_root_model(lattice->path))
    return lattice;

  if (st
      && st->print
      && st->print->beamLatticeCount == 1u
      && st->objectCount == 0u) {
    lattice = ak_3mf_first_beam_lattice(st->print);
    if (lattice && ak_3mf_path_is_root_model(lattice->path))
      return lattice;
  }

  return NULL;
}

static
const AkPrintDisplacementMesh*
ak_3mf_first_displacement_mesh(const AkPrintDocument * __restrict print) {
  return print ? print->displacementMeshes : NULL;
}

static
const AkPrintDisplacementMesh*
ak_3mf_find_displacement_mesh(const AkPrintDocument * __restrict print,
                              uint32_t                            objectId) {
  const AkPrintDisplacementMesh *mesh;

  for (mesh = print ? print->displacementMeshes : NULL; mesh; mesh = mesh->next) {
    if (mesh->objectId == objectId)
      return mesh;
  }

  return NULL;
}

static
const AkPrintDisplacementMesh*
ak_3mf_displacement_mesh_for_export(AK3MFExportState * __restrict st,
                                    uint32_t                      objectId) {
  const AkPrintDisplacementMesh *mesh;

  mesh = ak_3mf_find_displacement_mesh(st ? st->print : NULL, objectId);
  if (mesh && ak_3mf_path_is_root_model(mesh->path))
    return mesh;

  if (st
      && st->print
      && st->print->displacementMeshCount == 1u
      && st->objectCount == 0u) {
    mesh = ak_3mf_first_displacement_mesh(st->print);
    if (mesh && ak_3mf_path_is_root_model(mesh->path))
      return mesh;
  }

  return NULL;
}

static
const AkPrintVolumetricMesh*
ak_3mf_first_volumetric_mesh(const AkPrintDocument * __restrict print) {
  return print ? print->volumetricMeshes : NULL;
}

static
const AkPrintVolumetricMesh*
ak_3mf_find_volumetric_mesh(const AkPrintDocument * __restrict print,
                            uint32_t                            objectId) {
  const AkPrintVolumetricMesh *mesh;

  for (mesh = print ? print->volumetricMeshes : NULL; mesh; mesh = mesh->next) {
    if (mesh->objectId == objectId)
      return mesh;
  }

  return NULL;
}

static
const AkPrintVolumetricMesh*
ak_3mf_volumetric_mesh_for_export(AK3MFExportState * __restrict st,
                                  uint32_t                      objectId) {
  const AkPrintVolumetricMesh *mesh;

  mesh = ak_3mf_find_volumetric_mesh(st ? st->print : NULL, objectId);
  if (mesh && ak_3mf_path_is_root_model(mesh->path))
    return mesh;

  if (st
      && st->print
      && st->print->volumetricMeshCount == 1u
      && st->objectCount == 0u) {
    mesh = ak_3mf_first_volumetric_mesh(st->print);
    if (mesh && ak_3mf_path_is_root_model(mesh->path))
      return mesh;
  }

  return NULL;
}

static
void
ak_3mf_buf_free(AK3MFBuffer * __restrict buf) {
  free(buf->data);
  memset(buf, 0, sizeof(*buf));
}

static
bool
ak_3mf_rows_init(AK3MFRows * __restrict rows,
                 AkAccessor * __restrict acc) {
  size_t floatCount;

  memset(rows, 0, sizeof(*rows));
  if (!acc || acc->count == 0 || acc->componentCount == 0)
    return false;

  rows->accessor       = acc;
  rows->componentCount = acc->componentCount;
  rows->direct         = io_accessor_float_direct(acc);
  if (rows->direct)
    return true;

  if ((size_t)acc->count > (size_t)-1 / acc->componentCount)
    return false;

  floatCount    = (size_t)acc->count * acc->componentCount;
  rows->scratch = malloc(sizeof(float) * floatCount);
  if (!rows->scratch)
    return false;

  if (ak_accessorAsFloat(acc, rows->scratch, floatCount) != floatCount) {
    free(rows->scratch);
    rows->scratch = NULL;
    return false;
  }

  return true;
}

static
void
ak_3mf_rows_destroy(AK3MFRows * __restrict rows) {
  free(rows->scratch);
  rows->scratch = NULL;
}

static
const float*
ak_3mf_rows_get(AK3MFRows * __restrict rows, uint32_t index) {
  if (index >= rows->accessor->count)
    index = 0;

  return rows->direct
         ? io_accessor_float_row(rows->accessor, index)
         : rows->scratch + (size_t)index * rows->componentCount;
}

static
float
ak_3mf_row_component(const float * __restrict row,
                     uint32_t                 componentCount,
                     uint32_t                 component,
                     float                    fallback) {
  return component < componentCount ? row[component] : fallback;
}

static
AkInput*
ak_3mf_find_position_input(AkMeshPrimitive * __restrict prim) {
  AkInput *input;

  if (!prim)
    return NULL;
  if (prim->pos)
    return prim->pos;

  for (input = prim->input; input; input = input->next) {
    if (input->semantic == AK_INPUT_POSITION)
      return input;
  }

  return NULL;
}

static
AkInput*
ak_3mf_find_color_input(AkMeshPrimitive * __restrict prim) {
  AkInput *input;
  AkInput *fallback;

  fallback = NULL;
  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if (input->semantic != AK_INPUT_COLOR
        || !input->accessor
        || input->accessor->componentCount < 3u)
      continue;
    if (input->set == 0)
      return input;
    if (!fallback)
      fallback = input;
  }

  return fallback;
}

static
void
ak_3mf_vertex_position(AK3MFRows       * __restrict rows,
                       AkMeshPrimitive * __restrict prim,
                       AkInput         * __restrict posInput,
                       uint32_t                     vertexIndex,
                       vec3                         out) {
  const float *row;
  uint32_t     posIndex;

  posIndex = io_primitive_input_index(prim, posInput, vertexIndex);
  row      = ak_3mf_rows_get(rows, posIndex);

  out[0] = ak_3mf_row_component(row, rows->componentCount, 0u, 0.0f);
  out[1] = ak_3mf_row_component(row, rows->componentCount, 1u, 0.0f);
  out[2] = ak_3mf_row_component(row, rows->componentCount, 2u, 0.0f);
}

static
float
ak_3mf_color_component(AK3MFRows   * __restrict rows,
                       const float * __restrict row,
                       uint32_t                 component,
                       float                    fallback) {
  float value;

  value = ak_3mf_row_component(row, rows->componentCount, component, fallback);
  if (!rows->accessor->normalized) {
    switch (rows->accessor->componentType) {
      case AKT_UBYTE:
        value *= 1.0f / 255.0f;
        break;
      case AKT_USHORT:
        value *= 1.0f / 65535.0f;
        break;
      default:
        break;
    }
  }

  if (value < 0.0f)
    value = 0.0f;
  else if (value > 1.0f)
    value = 1.0f;

  return value;
}

static
uint8_t
ak_3mf_color_u8(float value) {
  if (value < 0.0f)
    value = 0.0f;
  else if (value > 1.0f)
    value = 1.0f;

  return (uint8_t)(value * 255.0f + 0.5f);
}

static
void
ak_3mf_vertex_color(AK3MFRows       * __restrict rows,
                    AkMeshPrimitive * __restrict prim,
                    AkInput         * __restrict colorInput,
                    uint32_t                     vertexIndex,
                    uint8_t                      rgba[4]) {
  const float *row;
  uint32_t     colorIndex;

  rgba[0] = 255u;
  rgba[1] = 255u;
  rgba[2] = 255u;
  rgba[3] = 255u;
  if (!rows || !colorInput)
    return;

  colorIndex = io_primitive_input_index(prim, colorInput, vertexIndex);
  row        = ak_3mf_rows_get(rows, colorIndex);

  rgba[0] = ak_3mf_color_u8(ak_3mf_color_component(rows, row, 0u, 1.0f));
  rgba[1] = ak_3mf_color_u8(ak_3mf_color_component(rows, row, 1u, 1.0f));
  rgba[2] = ak_3mf_color_u8(ak_3mf_color_component(rows, row, 2u, 1.0f));
  rgba[3] = ak_3mf_color_u8(ak_3mf_color_component(rows, row, 3u, 1.0f));
}

static
void
ak_3mf_append_vertex(AK3MFBuffer * __restrict vertices,
                     vec3                     pos,
                     bool                     displacement) {
  ak_3mf_buf_lit(vertices, displacement ? "          <d:vertex x=\""
                                        : "          <vertex x=\"");
  ak_3mf_buf_float(vertices, pos[0]);
  ak_3mf_buf_lit(vertices, "\" y=\"");
  ak_3mf_buf_float(vertices, pos[1]);
  ak_3mf_buf_lit(vertices, "\" z=\"");
  ak_3mf_buf_float(vertices, pos[2]);
  ak_3mf_buf_lit(vertices, "\"/>\n");
}

static
void
ak_3mf_append_color(AK3MFBuffer * __restrict colors, uint8_t rgba[4]) {
  static const char hex[] = "0123456789ABCDEF";
  uint32_t          i;

  ak_3mf_buf_lit(colors, "        <m:color color=\"#");
  for (i = 0; i < 4u; i++) {
    ak_3mf_buf_ch(colors, hex[rgba[i] >> 4u]);
    ak_3mf_buf_ch(colors, hex[rgba[i] & 0x0fu]);
  }
  ak_3mf_buf_lit(colors, "\"/>\n");
}

static
void
ak_3mf_append_displacement_triangle_attrs(
                       AK3MFBuffer                       * __restrict triangles,
                       const AkPrintDisplacementTriangle * __restrict displacement) {
  if (!displacement)
    return;

  if ((displacement->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_GROUP) != 0u) {
    ak_3mf_buf_lit(triangles, "\" did=\"");
    ak_3mf_buf_u32(triangles, displacement->groupId);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D1) != 0u) {
    ak_3mf_buf_lit(triangles, "\" d1=\"");
    ak_3mf_buf_u32(triangles, displacement->d1);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D2) != 0u) {
    ak_3mf_buf_lit(triangles, "\" d2=\"");
    ak_3mf_buf_u32(triangles, displacement->d2);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D3) != 0u) {
    ak_3mf_buf_lit(triangles, "\" d3=\"");
    ak_3mf_buf_u32(triangles, displacement->d3);
  }
}

static
const AkPrintDisplacementTriangle*
ak_3mf_displacement_write_next(AK3MFDisplacementWrite * __restrict writer) {
  const AkPrintDisplacementTriangle *triangle;

  if (!writer || !writer->triangle || writer->remaining == 0u)
    return NULL;

  triangle = writer->triangle;
  writer->triangle = writer->triangle->next;
  writer->remaining--;
  return triangle;
}

static
void
ak_3mf_append_triangle(AK3MFBuffer * __restrict triangles,
                       uint32_t                 i0,
                       uint32_t                 i1,
                       uint32_t                 i2,
                       uint32_t                 propertyId,
                       bool                     displacementMesh,
                       const AkPrintDisplacementTriangle * __restrict displacement) {
  ak_3mf_buf_lit(triangles, displacementMesh ? "          <d:triangle v1=\""
                                             : "          <triangle v1=\"");
  ak_3mf_buf_u32(triangles, i0);
  ak_3mf_buf_lit(triangles, "\" v2=\"");
  ak_3mf_buf_u32(triangles, i1);
  ak_3mf_buf_lit(triangles, "\" v3=\"");
  ak_3mf_buf_u32(triangles, i2);
  if (propertyId != 0u) {
    ak_3mf_buf_lit(triangles, "\" pid=\"");
    ak_3mf_buf_u32(triangles, propertyId);
    ak_3mf_buf_lit(triangles, "\" p1=\"");
    ak_3mf_buf_u32(triangles, i0);
    ak_3mf_buf_lit(triangles, "\" p2=\"");
    ak_3mf_buf_u32(triangles, i1);
    ak_3mf_buf_lit(triangles, "\" p3=\"");
    ak_3mf_buf_u32(triangles, i2);
  }
  ak_3mf_append_displacement_triangle_attrs(triangles, displacement);
  ak_3mf_buf_lit(triangles, "\"/>\n");
}

static
void
ak_3mf_append_slice_object_attrs(AK3MFBuffer               * __restrict buf,
                                 const AkPrintSliceObject  * __restrict object) {
  if (!object)
    return;

  if (object->meshResolution) {
    ak_3mf_buf_lit(buf, "\" s:meshresolution=\"");
    ak_3mf_buf_attr(buf, object->meshResolution);
  }
  if (object->sliceStackId != 0u) {
    ak_3mf_buf_lit(buf, "\" s:slicestackid=\"");
    ak_3mf_buf_u32(buf, object->sliceStackId);
  }
  if (object->slicePath) {
    ak_3mf_buf_lit(buf, "\" s:slicepath=\"/");
    ak_3mf_buf_attr(buf, object->slicePath);
  }
}

static
const AkPrintBeam*
ak_3mf_first_beam_for_lattice(const AkPrintDocument     * __restrict print,
                              const AkPrintBeamLattice * __restrict lattice) {
  const AkPrintBeamLattice *it;
  const AkPrintBeam        *beam;
  uint32_t                  i;

  beam = print ? print->beams : NULL;
  for (it = print ? print->beamLattices : NULL; it && it != lattice; it = it->next) {
    for (i = 0u; i < it->beamCount && beam; i++)
      beam = beam->next;
  }

  return it == lattice ? beam : NULL;
}

static
const AkPrintBeamBall*
ak_3mf_first_ball_for_lattice(const AkPrintDocument     * __restrict print,
                              const AkPrintBeamLattice * __restrict lattice) {
  const AkPrintBeamLattice *it;
  const AkPrintBeamBall    *ball;
  uint32_t                  i;

  ball = print ? print->beamBalls : NULL;
  for (it = print ? print->beamLattices : NULL; it && it != lattice; it = it->next) {
    for (i = 0u; i < it->ballCount && ball; i++)
      ball = ball->next;
  }

  return it == lattice ? ball : NULL;
}

static
const AkPrintDisplacementTriangle*
ak_3mf_first_displacement_triangle_for_mesh(
                               const AkPrintDocument         * __restrict print,
                               const AkPrintDisplacementMesh * __restrict mesh) {
  const AkPrintDisplacementMesh     *it;
  const AkPrintDisplacementTriangle *triangle;
  uint32_t                           i;

  triangle = print ? print->displacementTriangles : NULL;
  for (it = print ? print->displacementMeshes : NULL; it && it != mesh; it = it->next) {
    for (i = 0u; i < it->triangleCount && triangle; i++)
      triangle = triangle->next;
  }

  return it == mesh ? triangle : NULL;
}

static
void
ak_3mf_write_beam(AK3MFBuffer       * __restrict buf,
                  const AkPrintBeam * __restrict beam) {
  if (!beam)
    return;

  ak_3mf_buf_lit(buf, "            <b:beam v1=\"");
  ak_3mf_buf_u32(buf, beam->v1);
  ak_3mf_buf_lit(buf, "\" v2=\"");
  ak_3mf_buf_u32(buf, beam->v2);
  if ((beam->flags & AK_PRINT_BEAM_HAS_R1) != 0u) {
    ak_3mf_buf_lit(buf, "\" r1=\"");
    ak_3mf_buf_float(buf, beam->r1);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_R2) != 0u) {
    ak_3mf_buf_lit(buf, "\" r2=\"");
    ak_3mf_buf_float(buf, beam->r2);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_P1) != 0u) {
    ak_3mf_buf_lit(buf, "\" p1=\"");
    ak_3mf_buf_u32(buf, beam->p1);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_P2) != 0u) {
    ak_3mf_buf_lit(buf, "\" p2=\"");
    ak_3mf_buf_u32(buf, beam->p2);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_PID) != 0u) {
    ak_3mf_buf_lit(buf, "\" pid=\"");
    ak_3mf_buf_u32(buf, beam->pid);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_CAP1) != 0u && beam->cap1) {
    ak_3mf_buf_lit(buf, "\" cap1=\"");
    ak_3mf_buf_attr(buf, beam->cap1);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_CAP2) != 0u && beam->cap2) {
    ak_3mf_buf_lit(buf, "\" cap2=\"");
    ak_3mf_buf_attr(buf, beam->cap2);
  }
  ak_3mf_buf_lit(buf, "\"/>\n");
}

static
void
ak_3mf_write_beam_ball(AK3MFBuffer           * __restrict buf,
                       const AkPrintBeamBall * __restrict ball) {
  if (!ball)
    return;

  ak_3mf_buf_lit(buf, "            <b2:ball vindex=\"");
  ak_3mf_buf_u32(buf, ball->vindex);
  if ((ball->flags & AK_PRINT_BEAM_BALL_HAS_RADIUS) != 0u) {
    ak_3mf_buf_lit(buf, "\" r=\"");
    ak_3mf_buf_float(buf, ball->radius);
  }
  if ((ball->flags & AK_PRINT_BEAM_BALL_HAS_P) != 0u) {
    ak_3mf_buf_lit(buf, "\" p=\"");
    ak_3mf_buf_u32(buf, ball->p);
  }
  if ((ball->flags & AK_PRINT_BEAM_BALL_HAS_PID) != 0u) {
    ak_3mf_buf_lit(buf, "\" pid=\"");
    ak_3mf_buf_u32(buf, ball->pid);
  }
  ak_3mf_buf_lit(buf, "\"/>\n");
}

static
void
ak_3mf_write_beam_lattice(AK3MFExportState         * __restrict st,
                          AK3MFBuffer              * __restrict buf,
                          const AkPrintBeamLattice * __restrict lattice) {
  const AkPrintBeam     *beam;
  const AkPrintBeamBall *ball;
  uint32_t               i;

  if (!st || !buf || !lattice)
    return;

  beam = ak_3mf_first_beam_for_lattice(st->print, lattice);
  ball = ak_3mf_first_ball_for_lattice(st->print, lattice);

  ak_3mf_buf_lit(buf, "          <b:beamlattice radius=\"");
  ak_3mf_buf_float(buf, lattice->radius);
  ak_3mf_buf_lit(buf, "\" minlength=\"");
  ak_3mf_buf_float(buf, lattice->minLength);
  if (lattice->clippingMode) {
    ak_3mf_buf_lit(buf, "\" clippingmode=\"");
    ak_3mf_buf_attr(buf, lattice->clippingMode);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_CLIPPING_MESH) != 0u) {
    ak_3mf_buf_lit(buf, "\" clippingmesh=\"");
    ak_3mf_buf_u32(buf, lattice->clippingMesh);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_REPRESENTATION_MESH) != 0u) {
    ak_3mf_buf_lit(buf, "\" representationmesh=\"");
    ak_3mf_buf_u32(buf, lattice->representationMesh);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_PID) != 0u) {
    ak_3mf_buf_lit(buf, "\" pid=\"");
    ak_3mf_buf_u32(buf, lattice->pid);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_PINDEX) != 0u) {
    ak_3mf_buf_lit(buf, "\" pindex=\"");
    ak_3mf_buf_u32(buf, lattice->pindex);
  }
  if (lattice->cap) {
    ak_3mf_buf_lit(buf, "\" cap=\"");
    ak_3mf_buf_attr(buf, lattice->cap);
  }
  if (lattice->ballMode) {
    ak_3mf_buf_lit(buf, "\" b2:ballmode=\"");
    ak_3mf_buf_attr(buf, lattice->ballMode);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_BALL_RADIUS) != 0u) {
    ak_3mf_buf_lit(buf, "\" b2:ballradius=\"");
    ak_3mf_buf_float(buf, lattice->ballRadius);
  }
  ak_3mf_buf_lit(buf, "\">\n"
                      "            <b:beams>\n");

  for (i = 0u; i < lattice->beamCount && beam; i++, beam = beam->next)
    ak_3mf_write_beam(buf, beam);

  ak_3mf_buf_lit(buf, "            </b:beams>\n");
  if (lattice->ballCount > 0u) {
    ak_3mf_buf_lit(buf, "            <b2:balls>\n");
    for (i = 0u; i < lattice->ballCount && ball; i++, ball = ball->next)
      ak_3mf_write_beam_ball(buf, ball);
    ak_3mf_buf_lit(buf, "            </b2:balls>\n");
  }
  ak_3mf_buf_lit(buf, "          </b:beamlattice>\n");
}

static
bool
ak_3mf_emit_triangle(AK3MFRows       * __restrict rows,
                     AK3MFRows       * __restrict colorRows,
                     AkMeshPrimitive * __restrict prim,
                     AkInput         * __restrict posInput,
                     AkInput         * __restrict colorInput,
                     uint32_t                     i0,
                     uint32_t                     i1,
                     uint32_t                     i2,
                     AK3MFBuffer     * __restrict vertices,
                     AK3MFBuffer     * __restrict colors,
                     AK3MFBuffer     * __restrict triangles,
                     uint32_t                     propertyId,
                     bool                         displacementMesh,
                     AK3MFDisplacementWrite * __restrict displacement,
                     uint32_t        * __restrict vertexCount,
                     uint32_t        * __restrict triangleCount) {
  vec3 a;
  vec3 b;
  vec3 c;
  uint8_t rgba[4];

  if (*vertexCount > UINT32_MAX - 3u)
    return false;

  ak_3mf_vertex_position(rows, prim, posInput, i0, a);
  ak_3mf_vertex_position(rows, prim, posInput, i1, b);
  ak_3mf_vertex_position(rows, prim, posInput, i2, c);

  ak_3mf_append_vertex(vertices, a, displacementMesh);
  ak_3mf_append_vertex(vertices, b, displacementMesh);
  ak_3mf_append_vertex(vertices, c, displacementMesh);
  if (propertyId != 0u) {
    ak_3mf_vertex_color(colorRows, prim, colorInput, i0, rgba);
    ak_3mf_append_color(colors, rgba);
    ak_3mf_vertex_color(colorRows, prim, colorInput, i1, rgba);
    ak_3mf_append_color(colors, rgba);
    ak_3mf_vertex_color(colorRows, prim, colorInput, i2, rgba);
    ak_3mf_append_color(colors, rgba);
  }
  ak_3mf_append_triangle(triangles,
                         *vertexCount,
                         *vertexCount + 1u,
                         *vertexCount + 2u,
                         propertyId,
                         displacementMesh,
                         ak_3mf_displacement_write_next(displacement));

  *vertexCount += 3u;
  (*triangleCount)++;
  return vertices->result == AK_OK && triangles->result == AK_OK;
}

static
bool
ak_3mf_emit_triangles_primitive(AK3MFRows       * __restrict rows,
                                AK3MFRows       * __restrict colorRows,
                                AkMeshPrimitive * __restrict prim,
                                AkInput         * __restrict posInput,
                                AkInput         * __restrict colorInput,
                                AK3MFBuffer     * __restrict vertices,
                                AK3MFBuffer     * __restrict colors,
                                AK3MFBuffer     * __restrict triangles,
                                uint32_t                     propertyId,
                                bool                         displacementMesh,
                                AK3MFDisplacementWrite * __restrict displacement,
                                uint32_t        * __restrict vertexCount,
                                uint32_t        * __restrict triangleCount) {
  AkTriangleMode mode;
  uint32_t       count;
  uint32_t       i;

  count = io_primitive_vertex_count(prim);
  mode  = ((AkTriangles *)prim)->mode;
  if (mode == 0)
    mode = AK_TRIANGLES;

  if (mode == AK_TRIANGLE_STRIP) {
    for (i = 0; i + 2u < count; i++) {
      if (i & 1u) {
        if (!ak_3mf_emit_triangle(rows, colorRows, prim, posInput, colorInput,
                                  i + 1u, i, i + 2u,
                                  vertices, colors, triangles, propertyId,
                                  displacementMesh, displacement,
                                  vertexCount, triangleCount))
          return false;
      } else {
        if (!ak_3mf_emit_triangle(rows, colorRows, prim, posInput, colorInput,
                                  i, i + 1u, i + 2u,
                                  vertices, colors, triangles, propertyId,
                                  displacementMesh, displacement,
                                  vertexCount, triangleCount))
          return false;
      }
    }
    return true;
  }

  if (mode == AK_TRIANGLE_FAN) {
    for (i = 1u; i + 1u < count; i++) {
      if (!ak_3mf_emit_triangle(rows, colorRows, prim, posInput, colorInput,
                                0u, i, i + 1u,
                                vertices, colors, triangles, propertyId,
                                displacementMesh, displacement,
                                vertexCount, triangleCount))
        return false;
    }
    return true;
  }

  for (i = 0; i + 2u < count; i += 3u) {
    if (!ak_3mf_emit_triangle(rows, colorRows, prim, posInput, colorInput,
                              i, i + 1u, i + 2u,
                              vertices, colors, triangles, propertyId,
                              displacementMesh, displacement,
                              vertexCount, triangleCount))
      return false;
  }

  return true;
}

static
bool
ak_3mf_emit_polygon_primitive(AK3MFRows       * __restrict rows,
                              AK3MFRows       * __restrict colorRows,
                              AkMeshPrimitive * __restrict prim,
                              AkInput         * __restrict posInput,
                              AkInput         * __restrict colorInput,
                              AK3MFBuffer     * __restrict vertices,
                              AK3MFBuffer     * __restrict colors,
                              AK3MFBuffer     * __restrict triangles,
                              uint32_t                     propertyId,
                              bool                         displacementMesh,
                              AK3MFDisplacementWrite * __restrict displacement,
                              uint32_t        * __restrict vertexCount,
                              uint32_t        * __restrict triangleCount) {
  AkPolygon *poly;
  size_t     cursor;
  size_t     i;

  poly = (AkPolygon *)prim;
  if (!poly->vcount || poly->vcount->count == 0)
    return true;

  cursor = 0;
  for (i = 0; i < poly->vcount->count; i++) {
    uint32_t vc;
    uint32_t j;

    vc = poly->vcount->items[i];
    if (vc < 3u) {
      cursor += vc;
      continue;
    }

    for (j = 1u; j + 1u < vc; j++) {
      if (!ak_3mf_emit_triangle(rows, colorRows, prim, posInput, colorInput,
                                (uint32_t)cursor,
                                (uint32_t)(cursor + j),
                                (uint32_t)(cursor + j + 1u),
                                vertices, colors, triangles, propertyId,
                                displacementMesh, displacement,
                                vertexCount, triangleCount))
        return false;
    }
    cursor += vc;
  }

  return true;
}

static
void
ak_3mf_append_transform(AK3MFBuffer * __restrict buf, mat4 world) {
  const float values[12] = {
    world[0][0], world[1][0], world[2][0],
    world[0][1], world[1][1], world[2][1],
    world[0][2], world[1][2], world[2][2],
    world[3][0], world[3][1], world[3][2]
  };
  uint32_t i;

  for (i = 0; i < 12u; i++) {
    if (i > 0)
      ak_3mf_buf_ch(buf, ' ');
    ak_3mf_buf_float(buf, values[i]);
  }
}

static
bool
ak_3mf_write_primitive(AK3MFExportState * __restrict st,
                       AkMeshPrimitive  * __restrict prim,
                       mat4                           world) {
  AK3MFBuffer vertices;
  AK3MFBuffer colors;
  AK3MFBuffer triangles;
  AK3MFRows   rows;
  AK3MFRows   colorRows;
  AkInput    *posInput;
  AkInput    *colorInput;
  uint32_t    vertexCount;
  uint32_t    triangleCount;
  uint32_t    objectId;
  uint32_t    propertyId;
  const AkPrintSliceObject *sliceObject;
  const AkPrintBeamLattice *beamLattice;
  const AkPrintDisplacementMesh *displacementMesh;
  const AkPrintVolumetricMesh *volumetricMesh;
  AK3MFDisplacementWrite displacementWrite;
  bool        ok;
  bool        hasColorRows;

  if (!prim)
    return true;
  if (prim->type != AK_PRIMITIVE_TRIANGLES
      && prim->type != AK_PRIMITIVE_POLYGONS)
    return true;

  posInput = ak_3mf_find_position_input(prim);
  if (!posInput || !posInput->accessor)
    return true;

  if (!ak_3mf_rows_init(&rows, posInput->accessor))
    return false;

  memset(&colorRows, 0, sizeof(colorRows));
  colorInput   = ak_3mf_find_color_input(prim);
  hasColorRows = colorInput && ak_3mf_rows_init(&colorRows, colorInput->accessor);
  propertyId   = hasColorRows ? st->nextObjectId++ : 0u;

  memset(&vertices, 0, sizeof(vertices));
  memset(&colors, 0, sizeof(colors));
  memset(&triangles, 0, sizeof(triangles));
  vertices.result  = AK_OK;
  colors.result    = AK_OK;
  triangles.result = AK_OK;
  vertexCount      = 0;
  triangleCount    = 0;
  beamLattice      = ak_3mf_beam_lattice_for_export(st, st->nextObjectId);
  displacementMesh = ak_3mf_displacement_mesh_for_export(st, st->nextObjectId);
  volumetricMesh   = ak_3mf_volumetric_mesh_for_export(st, st->nextObjectId);
  memset(&displacementWrite, 0, sizeof(displacementWrite));
  if (displacementMesh) {
    displacementWrite.triangle  = ak_3mf_first_displacement_triangle_for_mesh(st->print,
                                                                              displacementMesh);
    displacementWrite.remaining = displacementMesh->triangleCount;
  }

  if (beamLattice
      && !displacementMesh
      && !hasColorRows
      && prim->type == AK_PRIMITIVE_TRIANGLES) {
    AkTriangleMode mode;
    uint32_t       vi;
    uint32_t       count;
    uint32_t       i;

    ok = true;
    if (posInput->accessor->count > UINT32_MAX)
      ok = false;
    for (vi = 0u; ok && vi < posInput->accessor->count; vi++) {
      const float *row;
      vec3         pos;

      row    = ak_3mf_rows_get(&rows, vi);
      pos[0] = ak_3mf_row_component(row, rows.componentCount, 0u, 0.0f);
      pos[1] = ak_3mf_row_component(row, rows.componentCount, 1u, 0.0f);
      pos[2] = ak_3mf_row_component(row, rows.componentCount, 2u, 0.0f);
      ak_3mf_append_vertex(&vertices, pos, false);
      vertexCount++;
    }

    mode = ((AkTriangles *)prim)->mode;
    if (mode == 0)
      mode = AK_TRIANGLES;
    count = io_primitive_vertex_count(prim);
    if (ok && mode == AK_TRIANGLES) {
      for (i = 0u; i + 2u < count; i += 3u) {
        uint32_t i0;
        uint32_t i1;
        uint32_t i2;

        i0 = io_primitive_input_index(prim, posInput, i + 0u);
        i1 = io_primitive_input_index(prim, posInput, i + 1u);
        i2 = io_primitive_input_index(prim, posInput, i + 2u);
        if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) {
          ok = false;
          break;
        }
        ak_3mf_append_triangle(&triangles, i0, i1, i2, 0u, false, NULL);
        triangleCount++;
      }
    }
  } else if (prim->type == AK_PRIMITIVE_TRIANGLES) {
    ok = ak_3mf_emit_triangles_primitive(&rows,
                                         hasColorRows ? &colorRows : NULL,
                                         prim,
                                         posInput,
                                         hasColorRows ? colorInput : NULL,
                                         &vertices,
                                         &colors,
                                         &triangles,
                                         propertyId,
                                         displacementMesh != NULL,
                                         displacementMesh ? &displacementWrite : NULL,
                                         &vertexCount,
                                         &triangleCount);
  } else {
    ok = ak_3mf_emit_polygon_primitive(&rows,
                                       hasColorRows ? &colorRows : NULL,
                                       prim,
                                       posInput,
                                       hasColorRows ? colorInput : NULL,
                                       &vertices,
                                       &colors,
                                       &triangles,
                                       propertyId,
                                       displacementMesh != NULL,
                                       displacementMesh ? &displacementWrite : NULL,
                                       &vertexCount,
                                       &triangleCount);
  }

  ak_3mf_rows_destroy(&rows);
  if (hasColorRows)
    ak_3mf_rows_destroy(&colorRows);
  if (!ok || vertices.result != AK_OK || colors.result != AK_OK || triangles.result != AK_OK) {
    ak_3mf_buf_free(&vertices);
    ak_3mf_buf_free(&colors);
    ak_3mf_buf_free(&triangles);
    return false;
  }

  if (triangleCount == 0 && !beamLattice) {
    ak_3mf_buf_free(&vertices);
    ak_3mf_buf_free(&colors);
    ak_3mf_buf_free(&triangles);
    return true;
  }

  objectId = st->nextObjectId++;
  sliceObject = ak_3mf_slice_object_for_export(st, objectId);
  if (!beamLattice)
    beamLattice = ak_3mf_beam_lattice_for_export(st, objectId);
  if (!displacementMesh)
    displacementMesh = ak_3mf_displacement_mesh_for_export(st, objectId);
  if (!volumetricMesh)
    volumetricMesh = ak_3mf_volumetric_mesh_for_export(st, objectId);
  if (sliceObject
      && st->print
      && st->print->sliceObjectCount == 1u
      && st->objectCount == 0u
      && sliceObject->objectId != 0u
      && sliceObject->objectId != propertyId) {
    objectId = sliceObject->objectId;
    if (st->nextObjectId <= objectId)
      st->nextObjectId = objectId + 1u;
  }
  if (beamLattice
      && st->print
      && st->print->beamLatticeCount == 1u
      && st->objectCount == 0u
      && beamLattice->objectId != 0u
      && beamLattice->objectId != propertyId) {
    objectId = beamLattice->objectId;
    if (st->nextObjectId <= objectId)
      st->nextObjectId = objectId + 1u;
  }
  if (displacementMesh
      && st->print
      && st->print->displacementMeshCount == 1u
      && st->objectCount == 0u
      && displacementMesh->objectId != 0u
      && displacementMesh->objectId != propertyId) {
    objectId = displacementMesh->objectId;
    if (st->nextObjectId <= objectId)
      st->nextObjectId = objectId + 1u;
  }
  if (volumetricMesh
      && st->print
      && st->print->volumetricMeshCount == 1u
      && st->objectCount == 0u
      && volumetricMesh->objectId != 0u
      && volumetricMesh->objectId != propertyId) {
    objectId = volumetricMesh->objectId;
    if (st->nextObjectId <= objectId)
      st->nextObjectId = objectId + 1u;
  }

  if (propertyId != 0u) {
    ak_3mf_buf_lit(&st->resources, "      <m:colorgroup id=\"");
    ak_3mf_buf_u32(&st->resources, propertyId);
    ak_3mf_buf_lit(&st->resources, "\">\n");
    ak_3mf_buf_raw(&st->resources, colors.data, colors.len);
    ak_3mf_buf_lit(&st->resources, "      </m:colorgroup>\n");
    st->usesMaterialExtension = true;
  }

  ak_3mf_buf_lit(&st->resources, "      <object id=\"");
  ak_3mf_buf_u32(&st->resources, objectId);
  ak_3mf_append_slice_object_attrs(&st->resources, sliceObject);
  ak_3mf_buf_lit(&st->resources, "\" type=\"model\">\n");
  if (displacementMesh) {
    ak_3mf_buf_lit(&st->resources, "        <d:displacementmesh>\n"
                                  "          <d:vertices>\n");
  } else {
    ak_3mf_buf_lit(&st->resources, "        <mesh");
    if (volumetricMesh
        && (volumetricMesh->flags & AK_PRINT_VOLUMETRIC_MESH_HAS_VOLUME_ID) != 0u) {
      ak_3mf_buf_lit(&st->resources, " volumeid=\"");
      ak_3mf_buf_u32(&st->resources, volumetricMesh->volumeId);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    ak_3mf_buf_lit(&st->resources, ">\n"
                                  "          <vertices>\n");
  }
  ak_3mf_buf_raw(&st->resources, vertices.data, vertices.len);
  ak_3mf_buf_lit(&st->resources, displacementMesh
                                ? "          </d:vertices>\n"
                                : "          </vertices>\n");
  if (triangleCount > 0u) {
    if (displacementMesh) {
      ak_3mf_buf_lit(&st->resources, "          <d:triangles");
      if ((displacementMesh->flags
           & AK_PRINT_DISPLACEMENT_MESH_HAS_DEFAULT_GROUP) != 0u) {
        ak_3mf_buf_lit(&st->resources, " did=\"");
        ak_3mf_buf_u32(&st->resources, displacementMesh->defaultGroupId);
        ak_3mf_buf_ch(&st->resources, '"');
      }
      ak_3mf_buf_lit(&st->resources, ">\n");
    } else {
      ak_3mf_buf_lit(&st->resources, "          <triangles>\n");
    }
    ak_3mf_buf_raw(&st->resources, triangles.data, triangles.len);
    ak_3mf_buf_lit(&st->resources, displacementMesh
                                  ? "          </d:triangles>\n"
                                  : "          </triangles>\n");
  }
  ak_3mf_write_beam_lattice(st, &st->resources, beamLattice);
  ak_3mf_buf_lit(&st->resources, displacementMesh
                                ? "        </d:displacementmesh>\n"
                                  "      </object>\n"
                                : "        </mesh>\n"
                                  "      </object>\n");

  if (!st->suppressBuildItems) {
    ak_3mf_buf_lit(&st->build, "      <item objectid=\"");
    ak_3mf_buf_u32(&st->build, objectId);
    ak_3mf_buf_lit(&st->build, "\" transform=\"");
    ak_3mf_append_transform(&st->build, world);
    ak_3mf_buf_lit(&st->build, "\"/>\n");
  }

  st->objectCount++;

  ak_3mf_buf_free(&vertices);
  ak_3mf_buf_free(&colors);
  ak_3mf_buf_free(&triangles);

  return st->resources.result == AK_OK && st->build.result == AK_OK;
}

static
bool
ak_3mf_write_mesh_instance(AK3MFExportState * __restrict st,
                           AkGeometry       * __restrict geom,
                           mat4                           world) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;

  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return true;

  mesh = ak_objGet(geom->gdata);
  if (!mesh || !mesh->primitive)
    return true;

  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (!ak_3mf_write_primitive(st, prim, world))
      return false;
  }

  return true;
}

static
AkGeometry*
ak_3mf_instance_geometry(AkInstanceGeometry * __restrict inst) {
  void *obj;

  if (!inst)
    return NULL;

  obj = ak_instanceObject(&inst->base);
  return obj;
}

static
bool
ak_3mf_write_node(AK3MFExportState * __restrict st,
                  AkNode           * __restrict node,
                  mat4                           parentWorld,
                  uint32_t                       depth) {
  AkInstanceBase *base;
  AkInstanceNode *nodeRef;
  AkNode         *child;
  AkMatrix        localMatrix;
  mat4            world;

  if (!node)
    return true;
  if (depth > AK_3MF_MAX_NODE_DEPTH)
    return false;

  ak_transformCombine(node->transform, localMatrix.val[0]);
  glm_mat4_mul(parentWorld, localMatrix.val, world);

  for (base = node->geometry ? &node->geometry->base : NULL;
       base;
       base = base->next) {
    AkInstanceGeometry *inst;
    AkGeometry         *geom;

    if (base->type != AK_INSTANCE_GEOMETRY)
      continue;

    inst = (AkInstanceGeometry *)base;
    geom = ak_3mf_instance_geometry(inst);
    if (!ak_3mf_write_mesh_instance(st, geom, world))
      return false;
  }

  for (child = node->chld; child; child = child->next) {
    if (!ak_3mf_write_node(st, child, world, depth + 1u))
      return false;
  }

  for (nodeRef = node->node; nodeRef; nodeRef = nodeRef->next) {
    AkNode *target;

    target = ak_instanceNodeTarget(nodeRef);
    if (target && !ak_3mf_write_node(st, target, world, depth + 1u))
      return false;
  }

  return true;
}

static
bool
ak_3mf_write_scene(AK3MFExportState * __restrict st) {
  mat4 identity;

  glm_mat4_identity(identity);
  if (st->doc->scene && st->doc->scene->node)
    return ak_3mf_write_node(st, st->doc->scene->node, identity, 0u);

  return true;
}

static
bool
ak_3mf_write_library_fallback(AK3MFExportState * __restrict st) {
  AkGeometry *geom;
  mat4        identity;
  bool        savedSuppressBuildItems;

  if (st->objectCount > 0)
    return true;

  glm_mat4_identity(identity);
  savedSuppressBuildItems = st->suppressBuildItems;
  if (st->usesBooleanExtension
      || (st->usesVolumetricExtension
          && st->print
          && st->print->levelSetCount > 0u))
    st->suppressBuildItems = true;
  for (geom = st->doc->lib.geometries.first; geom; geom = geom->next) {
    if (!ak_3mf_write_mesh_instance(st, geom, identity)) {
      st->suppressBuildItems = savedSuppressBuildItems;
      return false;
    }
  }
  st->suppressBuildItems = savedSuppressBuildItems;

  return true;
}

static
const AkPrintNormVector*
ak_3mf_first_norm_vector_for_group(
                               const AkPrintDocument        * __restrict print,
                               const AkPrintNormVectorGroup * __restrict group) {
  const AkPrintNormVectorGroup *it;
  const AkPrintNormVector      *vector;
  uint32_t                      i;

  vector = print ? print->normVectors : NULL;
  for (it = print ? print->normVectorGroups : NULL; it && it != group; it = it->next) {
    for (i = 0u; i < it->vectorCount && vector; i++)
      vector = vector->next;
  }

  return it == group ? vector : NULL;
}

static
const AkPrintDisp2DCoord*
ak_3mf_first_disp2d_coord_for_group(const AkPrintDocument    * __restrict print,
                                    const AkPrintDisp2DGroup * __restrict group) {
  const AkPrintDisp2DGroup *it;
  const AkPrintDisp2DCoord *coord;
  uint32_t                  i;

  coord = print ? print->disp2DCoords : NULL;
  for (it = print ? print->disp2DGroups : NULL; it && it != group; it = it->next) {
    for (i = 0u; i < it->coordCount && coord; i++)
      coord = coord->next;
  }

  return it == group ? coord : NULL;
}

static
void
ak_3mf_write_displacement2d(AK3MFBuffer                 * __restrict buf,
                            const AkPrintDisplacement2D * __restrict displacement) {
  if (!buf || !displacement || !displacement->imagePath)
    return;

  ak_3mf_buf_lit(buf, "      <d:displacement2d id=\"");
  ak_3mf_buf_u32(buf, displacement->id);
  ak_3mf_buf_lit(buf, "\" path=\"/");
  ak_3mf_buf_attr(buf, displacement->imagePath);
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_2D_HAS_CHANNEL) != 0u
      && displacement->channel) {
    ak_3mf_buf_lit(buf, "\" channel=\"");
    ak_3mf_buf_attr(buf, displacement->channel);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_2D_HAS_TILESTYLE_U) != 0u
      && displacement->tileStyleU) {
    ak_3mf_buf_lit(buf, "\" tilestyleu=\"");
    ak_3mf_buf_attr(buf, displacement->tileStyleU);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_2D_HAS_TILESTYLE_V) != 0u
      && displacement->tileStyleV) {
    ak_3mf_buf_lit(buf, "\" tilestylev=\"");
    ak_3mf_buf_attr(buf, displacement->tileStyleV);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_2D_HAS_FILTER) != 0u
      && displacement->filter) {
    ak_3mf_buf_lit(buf, "\" filter=\"");
    ak_3mf_buf_attr(buf, displacement->filter);
  }
  ak_3mf_buf_lit(buf, "\"/>\n");
}

static
void
ak_3mf_write_norm_vector_group(AK3MFExportState              * __restrict st,
                               const AkPrintNormVectorGroup  * __restrict group) {
  const AkPrintNormVector *vector;
  uint32_t                 i;

  if (!st || !group)
    return;
  if (group->path && !ak_3mf_path_is_root_model(group->path))
    return;

  vector = ak_3mf_first_norm_vector_for_group(st->print, group);
  ak_3mf_buf_lit(&st->resources, "      <d:normvectorgroup id=\"");
  ak_3mf_buf_u32(&st->resources, group->id);
  ak_3mf_buf_lit(&st->resources, "\">\n");
  for (i = 0u; i < group->vectorCount && vector; i++, vector = vector->next) {
    ak_3mf_buf_lit(&st->resources, "        <d:normvector x=\"");
    ak_3mf_buf_float(&st->resources, vector->x);
    ak_3mf_buf_lit(&st->resources, "\" y=\"");
    ak_3mf_buf_float(&st->resources, vector->y);
    ak_3mf_buf_lit(&st->resources, "\" z=\"");
    ak_3mf_buf_float(&st->resources, vector->z);
    ak_3mf_buf_lit(&st->resources, "\"/>\n");
  }
  ak_3mf_buf_lit(&st->resources, "      </d:normvectorgroup>\n");
}

static
void
ak_3mf_write_disp2d_group(AK3MFExportState         * __restrict st,
                          const AkPrintDisp2DGroup * __restrict group) {
  const AkPrintDisp2DCoord *coord;
  uint32_t                  i;

  if (!st || !group)
    return;
  if (group->path && !ak_3mf_path_is_root_model(group->path))
    return;

  coord = ak_3mf_first_disp2d_coord_for_group(st->print, group);
  ak_3mf_buf_lit(&st->resources, "      <d:disp2dgroup id=\"");
  ak_3mf_buf_u32(&st->resources, group->id);
  ak_3mf_buf_lit(&st->resources, "\" dispid=\"");
  ak_3mf_buf_u32(&st->resources, group->displacementId);
  ak_3mf_buf_lit(&st->resources, "\" nid=\"");
  ak_3mf_buf_u32(&st->resources, group->normVectorGroupId);
  ak_3mf_buf_lit(&st->resources, "\" height=\"");
  ak_3mf_buf_float(&st->resources, group->height);
  if ((group->flags & AK_PRINT_DISP2D_GROUP_HAS_OFFSET) != 0u) {
    ak_3mf_buf_lit(&st->resources, "\" offset=\"");
    ak_3mf_buf_float(&st->resources, group->offset);
  }
  ak_3mf_buf_lit(&st->resources, "\">\n");
  for (i = 0u; i < group->coordCount && coord; i++, coord = coord->next) {
    ak_3mf_buf_lit(&st->resources, "        <d:disp2dcoord u=\"");
    ak_3mf_buf_float(&st->resources, coord->u);
    ak_3mf_buf_lit(&st->resources, "\" v=\"");
    ak_3mf_buf_float(&st->resources, coord->v);
    ak_3mf_buf_lit(&st->resources, "\" n=\"");
    ak_3mf_buf_u32(&st->resources, coord->normVectorIndex);
    if ((coord->flags & AK_PRINT_DISP2D_COORD_HAS_FACTOR) != 0u) {
      ak_3mf_buf_lit(&st->resources, "\" f=\"");
      ak_3mf_buf_float(&st->resources, coord->factor);
    }
    ak_3mf_buf_lit(&st->resources, "\"/>\n");
  }
  ak_3mf_buf_lit(&st->resources, "      </d:disp2dgroup>\n");
}

static
bool
ak_3mf_write_displacement_resources(AK3MFExportState * __restrict st) {
  const AkPrintDisplacement2D *displacement;
  const AkPrintNormVectorGroup *normGroup;
  const AkPrintDisp2DGroup     *dispGroup;

  if (!st || !st->print || !st->usesDisplacementExtension)
    return true;

  for (displacement = st->print->displacement2Ds;
       displacement;
       displacement = displacement->next) {
    if (!displacement->path || ak_3mf_path_is_root_model(displacement->path))
      ak_3mf_write_displacement2d(&st->resources, displacement);
  }
  for (normGroup = st->print->normVectorGroups;
       normGroup;
       normGroup = normGroup->next)
    ak_3mf_write_norm_vector_group(st, normGroup);
  for (dispGroup = st->print->disp2DGroups;
       dispGroup;
       dispGroup = dispGroup->next)
    ak_3mf_write_disp2d_group(st, dispGroup);

  return st->resources.result == AK_OK;
}

static
const AkPrintBooleanOperand*
ak_3mf_first_boolean_operand_for_shape(const AkPrintDocument     * __restrict print,
                                       const AkPrintBooleanShape * __restrict shape) {
  const AkPrintBooleanShape   *it;
  const AkPrintBooleanOperand *operand;
  uint32_t                     i;

  operand = print ? print->booleanOperands : NULL;
  for (it = print ? print->booleanShapes : NULL; it && it != shape; it = it->next) {
    for (i = 0u; i < it->operandCount && operand; i++)
      operand = operand->next;
  }

  return it == shape ? operand : NULL;
}

static
const char*
ak_3mf_boolean_operation_name(AkPrintBooleanOperation operation) {
  switch (operation) {
    case AK_PRINT_BOOLEAN_OPERATION_DIFFERENCE:
      return "difference";
    case AK_PRINT_BOOLEAN_OPERATION_INTERSECTION:
      return "intersection";
    case AK_PRINT_BOOLEAN_OPERATION_UNION:
    case AK_PRINT_BOOLEAN_OPERATION_UNKNOWN:
    default:
      return "union";
  }
}

static
void
ak_3mf_append_flat_transform(AK3MFBuffer         * __restrict buf,
                             const float          matrix[16]) {
  const float values[12] = {
    matrix[0], matrix[4], matrix[8],
    matrix[1], matrix[5], matrix[9],
    matrix[2], matrix[6], matrix[10],
    matrix[12], matrix[13], matrix[14]
  };
  uint32_t i;

  for (i = 0; i < 12u; i++) {
    if (i > 0)
      ak_3mf_buf_ch(buf, ' ');
    ak_3mf_buf_float(buf, values[i]);
  }
}

static
void
ak_3mf_append_optional_3mf_path(AK3MFBuffer * __restrict buf,
                                const char  * __restrict path) {
  if (!path)
    return;

  ak_3mf_buf_lit(buf, "\" path=\"/");
  ak_3mf_buf_attr(buf, path);
}

static
const AkPrintImageSheet*
ak_3mf_first_image_sheet_for_image(const AkPrintDocument * __restrict print,
                                   const AkPrintImage3D  * __restrict image) {
  const AkPrintImage3D    *it;
  const AkPrintImageSheet *sheet;
  uint32_t                 i;

  sheet = print ? print->imageSheets : NULL;
  for (it = print ? print->image3Ds : NULL; it && it != image; it = it->next) {
    for (i = 0u; i < it->imageSheetCount && sheet; i++)
      sheet = sheet->next;
  }

  return it == image ? sheet : NULL;
}

static
const AkPrintVolumetricElement*
ak_3mf_first_volumetric_element_for_volume(
                                  const AkPrintDocument   * __restrict print,
                                  const AkPrintVolumeData * __restrict volume) {
  const AkPrintVolumeData        *it;
  const AkPrintVolumetricElement *element;
  uint32_t                        i;

  element = print ? print->volumetricElements : NULL;
  for (it = print ? print->volumeData : NULL; it && it != volume; it = it->next) {
    uint32_t count;

    count = it->materialMappingCount + it->colorCount + it->propertyCount;
    for (i = 0u; i < count && element; i++)
      element = element->next;
  }

  return it == volume ? element : NULL;
}

static
void
ak_3mf_write_image3d(AK3MFExportState      * __restrict st,
                     const AkPrintImage3D  * __restrict image) {
  const AkPrintImageSheet *sheet;
  uint32_t                 i;

  if (!st || !image)
    return;
  if (image->path && !ak_3mf_path_is_root_model(image->path))
    return;

  sheet = ak_3mf_first_image_sheet_for_image(st->print, image);
  ak_3mf_buf_lit(&st->resources, "      <v:image3d id=\"");
  ak_3mf_buf_u32(&st->resources, image->id);
  ak_3mf_buf_ch(&st->resources, '"');
  if (image->name) {
    ak_3mf_buf_lit(&st->resources, " name=\"");
    ak_3mf_buf_attr(&st->resources, image->name);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  ak_3mf_buf_lit(&st->resources, ">\n"
                                "        <v:imagestack rowcount=\"");
  ak_3mf_buf_u32(&st->resources, image->rowCount);
  ak_3mf_buf_lit(&st->resources, "\" columncount=\"");
  ak_3mf_buf_u32(&st->resources, image->columnCount);
  ak_3mf_buf_lit(&st->resources, "\" sheetcount=\"");
  ak_3mf_buf_u32(&st->resources, image->sheetCount);
  ak_3mf_buf_lit(&st->resources, "\">\n");

  for (i = 0u; i < image->imageSheetCount && sheet; i++, sheet = sheet->next) {
    ak_3mf_buf_lit(&st->resources, "          <v:imagesheet path=\"/");
    ak_3mf_buf_attr(&st->resources, sheet->path);
    ak_3mf_buf_lit(&st->resources, "\"/>\n");
  }

  ak_3mf_buf_lit(&st->resources, "        </v:imagestack>\n"
                                "      </v:image3d>\n");
}

static
void
ak_3mf_write_function_from_image3d(
                         AK3MFExportState                    * __restrict st,
                         const AkPrintFunctionFromImage3D    * __restrict function) {
  if (!st || !function)
    return;
  if (function->path && !ak_3mf_path_is_root_model(function->path))
    return;

  ak_3mf_buf_lit(&st->resources, "      <v:functionfromimage3d id=\"");
  ak_3mf_buf_u32(&st->resources, function->id);
  ak_3mf_buf_lit(&st->resources, "\" image3did=\"");
  ak_3mf_buf_u32(&st->resources, function->image3DId);
  ak_3mf_buf_ch(&st->resources, '"');
  if (function->displayName) {
    ak_3mf_buf_lit(&st->resources, " displayname=\"");
    ak_3mf_buf_attr(&st->resources, function->displayName);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_OFFSET) != 0u) {
    ak_3mf_buf_lit(&st->resources, " valueoffset=\"");
    ak_3mf_buf_float(&st->resources, function->valueOffset);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_SCALE) != 0u) {
    ak_3mf_buf_lit(&st->resources, " valuescale=\"");
    ak_3mf_buf_float(&st->resources, function->valueScale);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_FILTER) != 0u
      && function->filter) {
    ak_3mf_buf_lit(&st->resources, " filter=\"");
    ak_3mf_buf_attr(&st->resources, function->filter);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_U) != 0u
      && function->tileStyleU) {
    ak_3mf_buf_lit(&st->resources, " tilestyleu=\"");
    ak_3mf_buf_attr(&st->resources, function->tileStyleU);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_V) != 0u
      && function->tileStyleV) {
    ak_3mf_buf_lit(&st->resources, " tilestylev=\"");
    ak_3mf_buf_attr(&st->resources, function->tileStyleV);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_W) != 0u
      && function->tileStyleW) {
    ak_3mf_buf_lit(&st->resources, " tilestylew=\"");
    ak_3mf_buf_attr(&st->resources, function->tileStyleW);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  ak_3mf_buf_lit(&st->resources, "/>\n");
}

static
void
ak_3mf_write_volumetric_element(AK3MFBuffer                    * __restrict buf,
                                const AkPrintVolumetricElement * __restrict element) {
  const char *tag;

  if (!buf || !element)
    return;

  switch (element->type) {
    case AK_PRINT_VOLUMETRIC_ELEMENT_MATERIAL_MAPPING:
      tag = "materialmapping";
      break;
    case AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY:
      tag = "property";
      break;
    case AK_PRINT_VOLUMETRIC_ELEMENT_COLOR:
    default:
      tag = "color";
      break;
  }

  ak_3mf_buf_lit(buf, "        <v:");
  ak_3mf_buf_lit(buf, tag);
  ak_3mf_buf_lit(buf, " functionid=\"");
  ak_3mf_buf_u32(buf, element->functionId);
  ak_3mf_buf_ch(buf, '"');
  if (element->channel) {
    ak_3mf_buf_lit(buf, " channel=\"");
    ak_3mf_buf_attr(buf, element->channel);
    ak_3mf_buf_ch(buf, '"');
  }
  if (element->type == AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY) {
    if (element->name) {
      ak_3mf_buf_lit(buf, " name=\"");
      ak_3mf_buf_attr(buf, element->name);
      ak_3mf_buf_ch(buf, '"');
    }
    ak_3mf_buf_lit(buf, " required=\"");
    ak_3mf_buf_lit(buf,
                   (element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_REQUIRED) != 0u
                   ? "true"
                   : "false");
    ak_3mf_buf_ch(buf, '"');
  }
  if ((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_HAS_TRANSFORM) != 0u) {
    ak_3mf_buf_lit(buf, " transform=\"");
    ak_3mf_append_flat_transform(buf, element->matrix);
    ak_3mf_buf_ch(buf, '"');
  }
  if ((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_HAS_MIN_FEATURE_SIZE) != 0u) {
    ak_3mf_buf_lit(buf, " minfeaturesize=\"");
    ak_3mf_buf_float(buf, element->minFeatureSize);
    ak_3mf_buf_ch(buf, '"');
  }
  if ((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_HAS_FALLBACK_VALUE) != 0u) {
    ak_3mf_buf_lit(buf, " fallbackvalue=\"");
    ak_3mf_buf_float(buf, element->fallbackValue);
    ak_3mf_buf_ch(buf, '"');
  }
  ak_3mf_buf_lit(buf, "/>\n");
}

static
void
ak_3mf_write_volume_elements_of_type(
                                AK3MFBuffer                    * __restrict buf,
                                const AkPrintVolumetricElement * __restrict element,
                                uint32_t                                     total,
                                AkPrintVolumetricElementType                 type) {
  uint32_t i;

  for (i = 0u; i < total && element; i++, element = element->next) {
    if (element->type == type)
      ak_3mf_write_volumetric_element(buf, element);
  }
}

static
void
ak_3mf_write_volume_data(AK3MFExportState        * __restrict st,
                         const AkPrintVolumeData * __restrict volume) {
  const AkPrintVolumetricElement *element;
  uint32_t                        total;
  bool                            hasComposite;

  if (!st || !volume)
    return;
  if (volume->path && !ak_3mf_path_is_root_model(volume->path))
    return;

  element      = ak_3mf_first_volumetric_element_for_volume(st->print, volume);
  total        = volume->materialMappingCount + volume->colorCount + volume->propertyCount;
  hasComposite = volume->materialMappingCount > 0u
                 || (volume->flags & AK_PRINT_VOLUME_DATA_HAS_BASE_MATERIAL_ID) != 0u;

  ak_3mf_buf_lit(&st->resources, "      <v:volumedata id=\"");
  ak_3mf_buf_u32(&st->resources, volume->id);
  ak_3mf_buf_lit(&st->resources, "\">\n");

  if (hasComposite) {
    ak_3mf_buf_lit(&st->resources, "        <v:composite basematerialid=\"");
    ak_3mf_buf_u32(&st->resources, volume->baseMaterialId);
    ak_3mf_buf_lit(&st->resources, "\">\n");
    ak_3mf_write_volume_elements_of_type(&st->resources,
                                         element,
                                         total,
                                         AK_PRINT_VOLUMETRIC_ELEMENT_MATERIAL_MAPPING);
    ak_3mf_buf_lit(&st->resources, "        </v:composite>\n");
  }

  ak_3mf_write_volume_elements_of_type(&st->resources,
                                       element,
                                       total,
                                       AK_PRINT_VOLUMETRIC_ELEMENT_COLOR);
  ak_3mf_write_volume_elements_of_type(&st->resources,
                                       element,
                                       total,
                                       AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY);
  ak_3mf_buf_lit(&st->resources, "      </v:volumedata>\n");
}

static
bool
ak_3mf_write_volumetric_resources(AK3MFExportState * __restrict st) {
  const AkPrintImage3D             *image;
  const AkPrintFunctionFromImage3D *function;
  const AkPrintVolumeData          *volume;

  if (!st || !st->print || !st->usesVolumetricExtension)
    return true;

  for (image = st->print->image3Ds; image; image = image->next)
    ak_3mf_write_image3d(st, image);
  for (function = st->print->functionFromImage3Ds; function; function = function->next)
    ak_3mf_write_function_from_image3d(st, function);
  for (volume = st->print->volumeData; volume; volume = volume->next)
    ak_3mf_write_volume_data(st, volume);

  return st->resources.result == AK_OK;
}

static
bool
ak_3mf_write_level_sets(AK3MFExportState * __restrict st) {
  const AkPrintLevelSet *levelSet;

  if (!st || !st->print || !st->usesVolumetricExtension)
    return true;

  for (levelSet = st->print->levelSets; levelSet; levelSet = levelSet->next) {
    uint32_t objectId;

    if (levelSet->path && !ak_3mf_path_is_root_model(levelSet->path))
      continue;

    objectId = levelSet->objectId;
    if (objectId == 0u || objectId < st->nextObjectId)
      objectId = st->nextObjectId;
    if (st->nextObjectId <= objectId)
      st->nextObjectId = objectId + 1u;

    ak_3mf_buf_lit(&st->resources, "      <object id=\"");
    ak_3mf_buf_u32(&st->resources, objectId);
    ak_3mf_buf_lit(&st->resources, "\" type=\"model\">\n"
                                  "        <v:levelset functionid=\"");
    ak_3mf_buf_u32(&st->resources, levelSet->functionId);
    ak_3mf_buf_ch(&st->resources, '"');
    if (levelSet->channel) {
      ak_3mf_buf_lit(&st->resources, " channel=\"");
      ak_3mf_buf_attr(&st->resources, levelSet->channel);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if (levelSet->meshId != 0u) {
      ak_3mf_buf_lit(&st->resources, " meshid=\"");
      ak_3mf_buf_u32(&st->resources, levelSet->meshId);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_VOLUME_ID) != 0u) {
      ak_3mf_buf_lit(&st->resources, " volumeid=\"");
      ak_3mf_buf_u32(&st->resources, levelSet->volumeId);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_TRANSFORM) != 0u) {
      ak_3mf_buf_lit(&st->resources, " transform=\"");
      ak_3mf_append_flat_transform(&st->resources, levelSet->matrix);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_MIN_FEATURE_SIZE) != 0u) {
      ak_3mf_buf_lit(&st->resources, " minfeaturesize=\"");
      ak_3mf_buf_float(&st->resources, levelSet->minFeatureSize);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_MESH_BBOX_ONLY) != 0u) {
      ak_3mf_buf_lit(&st->resources, " meshbboxonly=\"true\"");
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_FALLBACK_VALUE) != 0u) {
      ak_3mf_buf_lit(&st->resources, " fallbackvalue=\"");
      ak_3mf_buf_float(&st->resources, levelSet->fallbackValue);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    ak_3mf_buf_lit(&st->resources, "/>\n"
                                  "      </object>\n");

    ak_3mf_buf_lit(&st->build, "      <item objectid=\"");
    ak_3mf_buf_u32(&st->build, objectId);
    ak_3mf_buf_lit(&st->build, "\"/>\n");
    st->objectCount++;
  }

  return st->resources.result == AK_OK && st->build.result == AK_OK;
}

static
void
ak_3mf_write_boolean_operand(AK3MFBuffer                 * __restrict buf,
                             const AkPrintBooleanOperand * __restrict operand) {
  if (!operand)
    return;

  ak_3mf_buf_lit(buf, "          <bo:boolean objectid=\"");
  ak_3mf_buf_u32(buf, operand->objectId);
  if ((operand->flags & AK_PRINT_BOOLEAN_OPERAND_HAS_TRANSFORM) != 0u) {
    ak_3mf_buf_lit(buf, "\" transform=\"");
    ak_3mf_append_flat_transform(buf, operand->matrix);
  }
  ak_3mf_append_optional_3mf_path(buf, operand->path);
  ak_3mf_buf_lit(buf, "\"/>\n");
}

static
bool
ak_3mf_write_boolean_shapes(AK3MFExportState * __restrict st) {
  const AkPrintBooleanShape *shape;

  if (!st || !st->print || !st->usesBooleanExtension)
    return true;

  for (shape = st->print->booleanShapes; shape; shape = shape->next) {
    const AkPrintBooleanOperand *operand;
    uint32_t                     i;
    uint32_t                     objectId;

    if (shape->path && !ak_3mf_path_is_root_model(shape->path))
      continue;

    objectId = shape->objectId;
    if (objectId == 0u || objectId < st->nextObjectId)
      objectId = st->nextObjectId;
    if (st->nextObjectId <= objectId)
      st->nextObjectId = objectId + 1u;

    operand = ak_3mf_first_boolean_operand_for_shape(st->print, shape);

    ak_3mf_buf_lit(&st->resources, "      <object id=\"");
    ak_3mf_buf_u32(&st->resources, objectId);
    ak_3mf_buf_lit(&st->resources, "\" type=\"model\">\n"
                                  "        <bo:booleanshape objectid=\"");
    ak_3mf_buf_u32(&st->resources, shape->baseObjectId);
    ak_3mf_buf_lit(&st->resources, "\" operation=\"");
    ak_3mf_buf_lit(&st->resources, ak_3mf_boolean_operation_name(shape->operation));
    if ((shape->flags & AK_PRINT_BOOLEAN_SHAPE_HAS_TRANSFORM) != 0u) {
      ak_3mf_buf_lit(&st->resources, "\" transform=\"");
      ak_3mf_append_flat_transform(&st->resources, shape->matrix);
    }
    ak_3mf_append_optional_3mf_path(&st->resources, shape->basePath);
    ak_3mf_buf_lit(&st->resources, "\">\n");

    for (i = 0u; i < shape->operandCount && operand; i++, operand = operand->next)
      ak_3mf_write_boolean_operand(&st->resources, operand);

    ak_3mf_buf_lit(&st->resources, "        </bo:booleanshape>\n"
                                  "      </object>\n");

    ak_3mf_buf_lit(&st->build, "      <item objectid=\"");
    ak_3mf_buf_u32(&st->build, objectId);
    ak_3mf_buf_lit(&st->build, "\"/>\n");
    st->objectCount++;
  }

  return st->resources.result == AK_OK && st->build.result == AK_OK;
}

static
void
ak_3mf_write_slice_ref(AK3MFBuffer            * __restrict buf,
                       const AkPrintSliceRef  * __restrict ref) {
  if (!ref)
    return;

  ak_3mf_buf_lit(buf, "        <s:sliceref slicestackid=\"");
  ak_3mf_buf_u32(buf, ref->stackId);
  if (ref->path) {
    ak_3mf_buf_lit(buf, "\" slicepath=\"/");
    ak_3mf_buf_attr(buf, ref->path);
  }
  ak_3mf_buf_lit(buf, "\" ztop=\"");
  ak_3mf_buf_float(buf, ref->zTop);
  ak_3mf_buf_lit(buf, "\"/>\n");
}

static
void
ak_3mf_write_empty_slice(AK3MFBuffer         * __restrict buf,
                         const AkPrintSlice  * __restrict slice) {
  if (!slice)
    return;

  ak_3mf_buf_lit(buf, "        <s:slice ztop=\"");
  ak_3mf_buf_float(buf, slice->zTop);
  ak_3mf_buf_lit(buf, "\"/>\n");
}

static
bool
ak_3mf_write_slice_stacks(AK3MFExportState * __restrict st) {
  const AkPrintSliceStack *stack;
  const AkPrintSliceRef   *ref;
  const AkPrintSlice      *slice;

  if (!st || !st->print || !st->usesSliceExtension)
    return true;

  ref   = st->print->sliceRefs;
  slice = st->print->slices;
  for (stack = st->print->sliceStacks; stack; stack = stack->next) {
    uint32_t i;
    bool     rootStack;

    rootStack = ak_3mf_path_is_root_model(stack->path);
    if (rootStack) {
      ak_3mf_buf_lit(&st->resources, "      <s:slicestack id=\"");
      ak_3mf_buf_u32(&st->resources, stack->id);
      ak_3mf_buf_lit(&st->resources, "\" zbottom=\"");
      ak_3mf_buf_float(&st->resources, stack->zBottom);
      ak_3mf_buf_lit(&st->resources, "\">\n");
    }

    for (i = 0u; i < stack->sliceRefCount && ref; i++, ref = ref->next) {
      if (rootStack)
        ak_3mf_write_slice_ref(&st->resources, ref);
    }

    for (i = 0u; i < stack->sliceCount && slice; i++, slice = slice->next) {
      if (rootStack
          && slice->vertexCount == 0u
          && slice->polygonCount == 0u
          && slice->segmentCount == 0u)
        ak_3mf_write_empty_slice(&st->resources, slice);
    }

    if (rootStack)
      ak_3mf_buf_lit(&st->resources, "      </s:slicestack>\n");
  }

  return st->resources.result == AK_OK;
}

static
const char*
ak_3mf_export_unit_name(AkDoc * __restrict doc) {
  double dist;

  dist = doc && doc->unit ? doc->unit->dist : 0.001;
  if (fabs(dist - 0.000001) < 0.000000001)
    return "micron";
  if (fabs(dist - 0.001) < 0.0000001)
    return "millimeter";
  if (fabs(dist - 0.01) < 0.0000001)
    return "centimeter";
  if (fabs(dist - 0.0254) < 0.0000001)
    return "inch";
  if (fabs(dist - 0.3048) < 0.0000001)
    return "foot";
  if (fabs(dist - 1.0) < 0.0000001)
    return "meter";

  return "millimeter";
}

static
bool
ak_3mf_build_model_xml(AK3MFExportState * __restrict st,
                       AK3MFBuffer      * __restrict model) {
  const char *unitName;

  memset(model, 0, sizeof(*model));
  model->result = AK_OK;
  unitName      = ak_3mf_export_unit_name(st->doc);

  ak_3mf_buf_lit(model, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<model unit=\"");
  ak_3mf_buf_lit(model, unitName);
  ak_3mf_buf_lit(model, "\" xml:lang=\"en-US\" "
                        "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\"");
  if (st->usesMaterialExtension) {
    ak_3mf_buf_lit(model,
                   " xmlns:m=\"http://schemas.microsoft.com/3dmanufacturing/material/2015/02\"");
  }
  if (st->usesSliceExtension) {
    ak_3mf_buf_lit(model,
                   " xmlns:s=\"http://schemas.microsoft.com/3dmanufacturing/slice/2015/07\"");
  }
  if (st->usesBeamLatticeExtension) {
    ak_3mf_buf_lit(model,
                   " xmlns:b=\"http://schemas.microsoft.com/3dmanufacturing/beamlattice/2017/02\"");
  }
  if (st->usesBeamBallExtension) {
    ak_3mf_buf_lit(model,
                   " xmlns:b2=\"http://schemas.microsoft.com/3dmanufacturing/beamlattice/balls/2020/07\"");
  }
  if (st->usesBooleanExtension) {
    ak_3mf_buf_lit(model,
                   " xmlns:bo=\"http://schemas.3mf.io/3dmanufacturing/booleanoperations/2023/07\"");
  }
  if (st->usesDisplacementExtension) {
    ak_3mf_buf_lit(model,
                   " xmlns:d=\"http://schemas.3mf.io/3dmanufacturing/displacement/2023/10\"");
  }
  if (st->usesVolumetricExtension) {
    ak_3mf_buf_lit(model,
                   " xmlns:v=\"http://schemas.3mf.io/3dmanufacturing/volumetric/2022/01\"");
  }
  if (st->usesMaterialExtension
      || st->usesSliceExtension
      || st->usesBeamLatticeExtension
      || st->usesBeamBallExtension
      || st->usesBooleanExtension
      || st->usesDisplacementExtension
      || st->usesVolumetricExtension) {
    bool any;

    any = false;
    ak_3mf_buf_lit(model, " requiredextensions=\"");
    if (st->usesMaterialExtension) {
      ak_3mf_buf_lit(model, "m");
      any = true;
    }
    if (st->usesSliceExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      ak_3mf_buf_lit(model, "s");
      any = true;
    }
    if (st->usesBeamLatticeExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      ak_3mf_buf_lit(model, "b");
      any = true;
    }
    if (st->usesBeamBallExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      ak_3mf_buf_lit(model, "b2");
      any = true;
    }
    if (st->usesBooleanExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      ak_3mf_buf_lit(model, "bo");
      any = true;
    }
    if (st->usesDisplacementExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      ak_3mf_buf_lit(model, "d");
      any = true;
    }
    if (st->usesVolumetricExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      ak_3mf_buf_lit(model, "v");
    }
    ak_3mf_buf_ch(model, '"');
  }
  ak_3mf_buf_lit(model, ">\n"
                        "  <resources>\n");
  ak_3mf_buf_raw(model, st->resources.data, st->resources.len);
  ak_3mf_buf_lit(model, "  </resources>\n"
                        "  <build>\n");
  ak_3mf_buf_raw(model, st->build.data, st->build.len);
  ak_3mf_buf_lit(model, "  </build>\n"
                        "</model>\n");

  return model->result == AK_OK;
}

static
const char*
ak_3mf_zip_part_name(const char * __restrict name) {
  if (!name)
    return NULL;
  while (*name == '/' || *name == '\\')
    name++;
  return *name ? name : NULL;
}

static
bool
ak_3mf_extra_part_exportable(const AkPrintPackagePart * __restrict part) {
  const char *name;

  if (!part || !part->name || (!part->data && part->size > 0u))
    return false;

  name = ak_3mf_zip_part_name(part->name);
  if (!name)
    return false;
  if (strcmp(name, "[Content_Types].xml") == 0
      || strcmp(name, "_rels/.rels") == 0
      || strcmp(name, "3D/3dmodel.model") == 0)
    return false;

  return true;
}

static
size_t
ak_3mf_count_extra_parts(const AkPrintDocument * __restrict print) {
  const AkPrintPackagePart *part;
  size_t                    count;

  count = 0u;
  for (part = print ? print->parts : NULL; part; part = part->next) {
    if (ak_3mf_extra_part_exportable(part))
      count++;
  }

  return count;
}

static
bool
ak_3mf_build_content_types_xml(const AkPrintDocument * __restrict print,
                               AK3MFBuffer           * __restrict contentTypes) {
  const AkPrintPackagePart *part;

  memset(contentTypes, 0, sizeof(*contentTypes));
  contentTypes->result = AK_OK;

  ak_3mf_buf_lit(contentTypes,
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
                 "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
                 "  <Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n");

  for (part = print ? print->parts : NULL; part; part = part->next) {
    const char *name;

    if (!ak_3mf_extra_part_exportable(part) || !part->contentType)
      continue;

    name = ak_3mf_zip_part_name(part->name);
    ak_3mf_buf_lit(contentTypes, "  <Override PartName=\"/");
    ak_3mf_buf_attr(contentTypes, name);
    ak_3mf_buf_lit(contentTypes, "\" ContentType=\"");
    ak_3mf_buf_attr(contentTypes, part->contentType);
    ak_3mf_buf_lit(contentTypes, "\"/>\n");
  }

  ak_3mf_buf_lit(contentTypes, "</Types>\n");
  return contentTypes->result == AK_OK;
}

static
bool
ak_3mf_build_rels_xml(const AkPrintDocument * __restrict print,
                      AK3MFBuffer           * __restrict rels) {
  const AkPrintPackagePart *part;
  uint32_t                  relId;

  memset(rels, 0, sizeof(*rels));
  rels->result = AK_OK;
  relId        = 1u;

  ak_3mf_buf_lit(rels,
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
                 "  <Relationship Target=\"/3D/3dmodel.model\" Id=\"rel0\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>\n");

  for (part = print ? print->parts : NULL; part; part = part->next) {
    const char *name;

    if (!ak_3mf_extra_part_exportable(part) || !part->relationshipType)
      continue;

    name = ak_3mf_zip_part_name(part->name);
    ak_3mf_buf_lit(rels, "  <Relationship Target=\"/");
    ak_3mf_buf_attr(rels, name);
    ak_3mf_buf_lit(rels, "\" Id=\"rel");
    ak_3mf_buf_u32(rels, relId++);
    ak_3mf_buf_lit(rels, "\" Type=\"");
    ak_3mf_buf_attr(rels, part->relationshipType);
    ak_3mf_buf_lit(rels, "\"/>\n");
  }

  ak_3mf_buf_lit(rels, "</Relationships>\n");
  return rels->result == AK_OK;
}

AK_HIDE
AkResult
ak_3mf_export(AkDoc * __restrict doc, const char * __restrict filepath) {
  AK3MFExportState st;
  AK3MFBuffer      model;
  AK3MFBuffer      contentTypes;
  AK3MFBuffer      rels;
  AkZipWriteEntry *entries;
  AkPrintDocument *print;
  AkPrintPackagePart *part;
  size_t           extraPartCount;
  size_t           entryCount;
  size_t           entryIndex;
  AkResult         result;

  if (!doc || !filepath)
    return AK_ERR;

  memset(&st, 0, sizeof(st));
  st.doc              = doc;
  st.print            = ak_printDocument(doc);
  st.result           = AK_OK;
  st.resources.result = AK_OK;
  st.build.result     = AK_OK;
  st.nextObjectId     = 1u;
  st.usesSliceExtension = st.print
                          && (st.print->sliceStackCount > 0u
                              || st.print->sliceObjectCount > 0u);
  st.usesBeamLatticeExtension = st.print && st.print->beamLatticeCount > 0u;
  st.usesBeamBallExtension    = ak_3mf_uses_beam_ball_extension(st.print);
  st.usesBooleanExtension     = st.print && st.print->booleanShapeCount > 0u;
  st.usesDisplacementExtension = st.print
                                 && (st.print->displacement2DCount > 0u
                                     || st.print->normVectorGroupCount > 0u
                                     || st.print->disp2DGroupCount > 0u
                                     || st.print->displacementMeshCount > 0u);
  st.usesVolumetricExtension = st.print
                               && (st.print->image3DCount > 0u
                                   || st.print->imageSheetCount > 0u
                                   || st.print->functionFromImage3DCount > 0u
                                   || st.print->volumeDataCount > 0u
                                   || st.print->volumetricElementCount > 0u
                                   || st.print->volumetricMeshCount > 0u
                                   || st.print->levelSetCount > 0u);

  if (!ak_3mf_write_slice_stacks(&st)
      || !ak_3mf_write_displacement_resources(&st)
      || !ak_3mf_write_volumetric_resources(&st)
      || !ak_3mf_write_scene(&st)
      || !ak_3mf_write_library_fallback(&st)
      || !ak_3mf_write_level_sets(&st)
      || !ak_3mf_write_boolean_shapes(&st)
      || st.resources.result != AK_OK
      || st.build.result != AK_OK
      || !ak_3mf_build_model_xml(&st, &model)) {
    ak_3mf_buf_free(&st.resources);
    ak_3mf_buf_free(&st.build);
    return AK_ERR;
  }

  print          = st.print;
  extraPartCount = ak_3mf_count_extra_parts(print);
  entryCount     = 3u + extraPartCount;
  entries        = calloc(entryCount, sizeof(*entries));
  if (!entries) {
    ak_3mf_buf_free(&model);
    ak_3mf_buf_free(&st.resources);
    ak_3mf_buf_free(&st.build);
    return AK_ERR;
  }

  if (!ak_3mf_build_content_types_xml(print, &contentTypes)
      || !ak_3mf_build_rels_xml(print, &rels)) {
    free(entries);
    ak_3mf_buf_free(&contentTypes);
    ak_3mf_buf_free(&rels);
    ak_3mf_buf_free(&model);
    ak_3mf_buf_free(&st.resources);
    ak_3mf_buf_free(&st.build);
    return AK_ERR;
  }

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes.data;
  entries[0].size = contentTypes.len;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels.data;
  entries[1].size = rels.len;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = model.data;
  entries[2].size = model.len;

  entryIndex = 3u;
  for (part = print ? print->parts : NULL; part; part = part->next) {
    if (!ak_3mf_extra_part_exportable(part))
      continue;
    entries[entryIndex].name = ak_3mf_zip_part_name(part->name);
    entries[entryIndex].data = part->data;
    entries[entryIndex].size = part->size;
    entryIndex++;
  }

  result = ak_zip_write_stored(filepath, entries, entryCount);

  free(entries);
  ak_3mf_buf_free(&contentTypes);
  ak_3mf_buf_free(&rels);
  ak_3mf_buf_free(&model);
  ak_3mf_buf_free(&st.resources);
  ak_3mf_buf_free(&st.build);

  return result;
}
