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
#include "../common/buffer.h"
#include "../common/primitive.h"
#include "../common/text_number.h"
#include "../common/zip.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AK_3MF_MAX_NODE_DEPTH 512u

typedef IOFloatRows AK3MFRows;

#define ak_3mf_rows_init    io_float_rows_init
#define ak_3mf_rows_destroy io_float_rows_destroy
#define ak_3mf_rows_get     io_float_rows_get
#define ak_3mf_row_component io_float_row_component

typedef IOBuffer AK3MFBuffer;

typedef struct AK3MFExportState {
  AkDoc           *doc;
  AkPrintDocument *print;
  AK3MFBuffer      resources;
  AK3MFBuffer      build;
  uint32_t         objectCount;
  uint32_t         nextObjectId;
  AkResult         result;
  bool             usesMaterialExtension;
  bool             usesProductionExtension;
  bool             usesProductionAlternativeExtension;
  bool             usesSliceExtension;
  bool             usesBeamLatticeExtension;
  bool             usesBeamBallExtension;
  bool             usesBooleanExtension;
  bool             usesDisplacementExtension;
  bool             usesVolumetricExtension;
  bool             usesImplicitExtension;
  bool             suppressBuildItems;
} AK3MFExportState;

typedef struct AK3MFDisplacementWrite {
  const AkPrintDisplacementTriangle *triangle;
  uint32_t                           remaining;
} AK3MFDisplacementWrite;

#define AK_3MF_BUF_INITIAL_CAP 4096u
#define ak_3mf_buf_reserve(BUF, EXTRA)                                        \
  io_buffer_reserve((BUF), (EXTRA), AK_3MF_BUF_INITIAL_CAP)
#define ak_3mf_buf_raw(BUF, DATA, LEN)                                        \
  io_buffer_raw((BUF), (DATA), (LEN), AK_3MF_BUF_INITIAL_CAP)
#define AK_3MF_BUF_LIT(BUF, LIT)                                              \
  IO_BUFFER_LIT((BUF), LIT, AK_3MF_BUF_INITIAL_CAP)
#define ak_3mf_buf_lit(BUF, LIT)                                              \
  io_buffer_cstr((BUF), (LIT), AK_3MF_BUF_INITIAL_CAP)
#define ak_3mf_buf_ch(BUF, CH)                                                \
  io_buffer_ch((BUF), (CH), AK_3MF_BUF_INITIAL_CAP)
#define ak_3mf_buf_free(BUF) io_buffer_free((BUF))

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
        AK_3MF_BUF_LIT(buf, "&amp;");
        break;
      case '<':
        AK_3MF_BUF_LIT(buf, "&lt;");
        break;
      case '"':
        AK_3MF_BUF_LIT(buf, "&quot;");
        break;
      case '\'':
        AK_3MF_BUF_LIT(buf, "&apos;");
        break;
      default:
        ak_3mf_buf_ch(buf, (char)*it);
        break;
    }
  }
}

static
void
ak_3mf_buf_3mf_path_attr(AK3MFBuffer * __restrict buf,
                         const char  * __restrict path) {
  if (!path)
    return;

  if (*path != '/' && *path != '\\')
    ak_3mf_buf_ch(buf, '/');
  ak_3mf_buf_attr(buf, path);
}

static
void
ak_3mf_append_production_open_attrs(
                                    AK3MFBuffer                 * __restrict buf,
                                    const AkPrintProductionItem * __restrict item,
                                    bool                                     allowPath) {
  if (!item)
    return;

  if (item->uuid) {
    AK_3MF_BUF_LIT(buf, "\" p:UUID=\"");
    ak_3mf_buf_attr(buf, item->uuid);
  }
  if (allowPath && item->path) {
    AK_3MF_BUF_LIT(buf, "\" p:path=\"");
    ak_3mf_buf_3mf_path_attr(buf, item->path);
  }
}

static
void
ak_3mf_append_production_object_open_attrs(
                                    AK3MFBuffer                 * __restrict buf,
                                    const AkPrintProductionItem * __restrict item) {
  ak_3mf_append_production_open_attrs(buf, item, false);
  if (item && item->modelResolution) {
    AK_3MF_BUF_LIT(buf, "\" pa:modelresolution=\"");
    ak_3mf_buf_attr(buf, item->modelResolution);
  }
}

static
void
ak_3mf_append_production_attrs(AK3MFBuffer                 * __restrict buf,
                               const AkPrintProductionItem * __restrict item,
                               bool                                     allowPath) {
  if (!item)
    return;

  if (item->uuid) {
    AK_3MF_BUF_LIT(buf, " p:UUID=\"");
    ak_3mf_buf_attr(buf, item->uuid);
    ak_3mf_buf_ch(buf, '"');
  }
  if (allowPath && item->path) {
    AK_3MF_BUF_LIT(buf, " p:path=\"");
    ak_3mf_buf_3mf_path_attr(buf, item->path);
    ak_3mf_buf_ch(buf, '"');
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
  size_t outLen;

  if (!ak_io_text_format_float6(tmp, sizeof(tmp), value, &outLen)) {
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
const AkPrintProductionItem*
ak_3mf_first_production_item(const AkPrintDocument      * __restrict print,
                             AkPrintProductionItemType               type) {
  const AkPrintProductionItem *item;

  for (item = print ? print->productionItems : NULL; item; item = item->next) {
    if (item->type == type)
      return item;
  }

  return NULL;
}

static
bool
ak_3mf_uses_production_alternative_extension(
                                      const AkPrintDocument * __restrict print) {
  const AkPrintProductionItem *item;

  for (item = print ? print->productionItems : NULL; item; item = item->next) {
    if (item->type == AK_PRINT_PRODUCTION_ALTERNATIVE)
      return true;
    if (item->type == AK_PRINT_PRODUCTION_OBJECT && item->modelResolution)
      return true;
  }

  return false;
}

static
const AkPrintProductionItem*
ak_3mf_only_production_item(const AkPrintDocument      * __restrict print,
                            AkPrintProductionItemType               type) {
  const AkPrintProductionItem *item;
  const AkPrintProductionItem *found;

  found = NULL;
  for (item = print ? print->productionItems : NULL; item; item = item->next) {
    if (item->type != type)
      continue;
    if (found)
      return NULL;
    found = item;
  }

  return found;
}

static
const AkPrintProductionItem*
ak_3mf_find_production_item(const AkPrintDocument      * __restrict print,
                            AkPrintProductionItemType               type,
                            uint32_t                                objectId,
                            uint32_t                                parentObjectId) {
  const AkPrintProductionItem *item;

  for (item = print ? print->productionItems : NULL; item; item = item->next) {
    if (item->type == type
        && item->objectId == objectId
        && item->parentObjectId == parentObjectId)
      return item;
  }

  return NULL;
}

static
const AkPrintProductionItem*
ak_3mf_production_object_for_export(AK3MFExportState * __restrict st,
                                    uint32_t                      objectId) {
  const AkPrintProductionItem *item;

  item = ak_3mf_find_production_item(st ? st->print : NULL,
                                     AK_PRINT_PRODUCTION_OBJECT,
                                     objectId,
                                     0u);
  if (item && ak_3mf_path_is_root_model(item->path))
    return item;

  if (st && st->print && st->objectCount == 0u) {
    item = ak_3mf_only_production_item(st->print, AK_PRINT_PRODUCTION_OBJECT);
    if (item)
      return item;
  }

  return NULL;
}

static
const AkPrintProductionItem*
ak_3mf_production_item_for_export(AK3MFExportState * __restrict st,
                                  uint32_t                      objectId) {
  const AkPrintProductionItem *item;

  item = ak_3mf_find_production_item(st ? st->print : NULL,
                                     AK_PRINT_PRODUCTION_ITEM,
                                     objectId,
                                     0u);
  if (item)
    return item;

  if (st && st->print && st->print->buildItemCount == 1u)
    return ak_3mf_only_production_item(st->print, AK_PRINT_PRODUCTION_ITEM);

  return NULL;
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
AkInput*
ak_3mf_find_color_input(AkMeshPrimitive * __restrict prim) {
  return io_primitive_find_set_input(prim,
                                     AK_INPUT_COLOR,
                                     AK_INPUT_COLOR,
                                     3u);
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
                     float                    x,
                     float                    y,
                     float                    z,
                     bool                     displacement) {
  const char *prefix;
  char        tmp[192];
  char       *p;
  size_t      outLen;
  size_t      prefixLen;

  if (displacement) {
    prefix    = "          <d:vertex x=\"";
    prefixLen = sizeof("          <d:vertex x=\"") - 1u;
  } else {
    prefix    = "          <vertex x=\"";
    prefixLen = sizeof("          <vertex x=\"") - 1u;
  }

  p = tmp;
  memcpy(p, prefix, prefixLen);
  p += prefixLen;
  if (!ak_io_text_format_float6(p,
                                 sizeof(tmp) - (size_t)(p - tmp),
                                 x,
                                 &outLen)) {
    vertices->result = AK_ERR;
    return;
  }
  p += outLen;
  memcpy(p, "\" y=\"", sizeof("\" y=\"") - 1u);
  p += sizeof("\" y=\"") - 1u;
  if (!ak_io_text_format_float6(p,
                                 sizeof(tmp) - (size_t)(p - tmp),
                                 y,
                                 &outLen)) {
    vertices->result = AK_ERR;
    return;
  }
  p += outLen;
  memcpy(p, "\" z=\"", sizeof("\" z=\"") - 1u);
  p += sizeof("\" z=\"") - 1u;
  if (!ak_io_text_format_float6(p,
                                 sizeof(tmp) - (size_t)(p - tmp),
                                 z,
                                 &outLen)) {
    vertices->result = AK_ERR;
    return;
  }
  p += outLen;
  memcpy(p, "\"/>\n", sizeof("\"/>\n") - 1u);
  p += sizeof("\"/>\n") - 1u;

  ak_3mf_buf_raw(vertices, tmp, (size_t)(p - tmp));
}

static
void
ak_3mf_append_color(AK3MFBuffer * __restrict colors, uint8_t rgba[4]) {
  static const char hex[] = "0123456789ABCDEF";
  uint32_t          i;

  AK_3MF_BUF_LIT(colors, "        <m:color color=\"#");
  for (i = 0; i < 4u; i++) {
    ak_3mf_buf_ch(colors, hex[rgba[i] >> 4u]);
    ak_3mf_buf_ch(colors, hex[rgba[i] & 0x0fu]);
  }
  AK_3MF_BUF_LIT(colors, "\"/>\n");
}

static
void
ak_3mf_append_displacement_triangle_attrs(
                       AK3MFBuffer                       * __restrict triangles,
                       const AkPrintDisplacementTriangle * __restrict displacement) {
  if (!displacement)
    return;

  if ((displacement->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_GROUP) != 0u) {
    AK_3MF_BUF_LIT(triangles, "\" did=\"");
    ak_3mf_buf_u32(triangles, displacement->groupId);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D1) != 0u) {
    AK_3MF_BUF_LIT(triangles, "\" d1=\"");
    ak_3mf_buf_u32(triangles, displacement->d1);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D2) != 0u) {
    AK_3MF_BUF_LIT(triangles, "\" d2=\"");
    ak_3mf_buf_u32(triangles, displacement->d2);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D3) != 0u) {
    AK_3MF_BUF_LIT(triangles, "\" d3=\"");
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
  const char *prefix;
  char        tmp[128];
  char       *p;
  size_t      prefixLen;

  if (displacementMesh) {
    prefix    = "          <d:triangle v1=\"";
    prefixLen = sizeof("          <d:triangle v1=\"") - 1u;
  } else {
    prefix    = "          <triangle v1=\"";
    prefixLen = sizeof("          <triangle v1=\"") - 1u;
  }

  if (propertyId == 0u && !displacement) {
    p = tmp;
    memcpy(p, prefix, prefixLen);
    p += prefixLen;
    p = ak_io_text_format_uint64(p, i0);
    memcpy(p, "\" v2=\"", sizeof("\" v2=\"") - 1u);
    p += sizeof("\" v2=\"") - 1u;
    p = ak_io_text_format_uint64(p, i1);
    memcpy(p, "\" v3=\"", sizeof("\" v3=\"") - 1u);
    p += sizeof("\" v3=\"") - 1u;
    p = ak_io_text_format_uint64(p, i2);
    memcpy(p, "\"/>\n", sizeof("\"/>\n") - 1u);
    p += sizeof("\"/>\n") - 1u;
    ak_3mf_buf_raw(triangles, tmp, (size_t)(p - tmp));
    return;
  }

  ak_3mf_buf_raw(triangles, prefix, prefixLen);
  ak_3mf_buf_u32(triangles, i0);
  AK_3MF_BUF_LIT(triangles, "\" v2=\"");
  ak_3mf_buf_u32(triangles, i1);
  AK_3MF_BUF_LIT(triangles, "\" v3=\"");
  ak_3mf_buf_u32(triangles, i2);
  if (propertyId != 0u) {
    AK_3MF_BUF_LIT(triangles, "\" pid=\"");
    ak_3mf_buf_u32(triangles, propertyId);
    AK_3MF_BUF_LIT(triangles, "\" p1=\"");
    ak_3mf_buf_u32(triangles, i0);
    AK_3MF_BUF_LIT(triangles, "\" p2=\"");
    ak_3mf_buf_u32(triangles, i1);
    AK_3MF_BUF_LIT(triangles, "\" p3=\"");
    ak_3mf_buf_u32(triangles, i2);
  }
  ak_3mf_append_displacement_triangle_attrs(triangles, displacement);
  AK_3MF_BUF_LIT(triangles, "\"/>\n");
}

static
void
ak_3mf_append_slice_object_attrs(AK3MFBuffer               * __restrict buf,
                                 const AkPrintSliceObject  * __restrict object) {
  if (!object)
    return;

  if (object->meshResolution) {
    AK_3MF_BUF_LIT(buf, "\" s:meshresolution=\"");
    ak_3mf_buf_attr(buf, object->meshResolution);
  }
  if (object->sliceStackId != 0u) {
    AK_3MF_BUF_LIT(buf, "\" s:slicestackid=\"");
    ak_3mf_buf_u32(buf, object->sliceStackId);
  }
  if (object->slicePath) {
    AK_3MF_BUF_LIT(buf, "\" s:slicepath=\"/");
    ak_3mf_buf_attr(buf, object->slicePath);
  }
}

static
void
ak_3mf_write_production_alternatives(AK3MFExportState * __restrict st,
                                     AK3MFBuffer      * __restrict buf,
                                     uint32_t                       parentObjectId) {
  const AkPrintProductionItem *item;
  bool                         opened;

  if (!st || !st->print || !st->usesProductionAlternativeExtension)
    return;

  opened = false;
  for (item = st->print->productionItems; item; item = item->next) {
    if (item->type != AK_PRINT_PRODUCTION_ALTERNATIVE
        || item->parentObjectId != parentObjectId)
      continue;

    if (!opened) {
      AK_3MF_BUF_LIT(buf, "        <pa:alternatives>\n");
      opened = true;
    }

    AK_3MF_BUF_LIT(buf, "          <pa:alternative objectid=\"");
    ak_3mf_buf_u32(buf, item->objectId);
    if (item->uuid) {
      AK_3MF_BUF_LIT(buf, "\" UUID=\"");
      ak_3mf_buf_attr(buf, item->uuid);
    }
    if (item->path) {
      AK_3MF_BUF_LIT(buf, "\" path=\"");
      ak_3mf_buf_3mf_path_attr(buf, item->path);
    }
    if (item->modelResolution) {
      AK_3MF_BUF_LIT(buf, "\" modelresolution=\"");
      ak_3mf_buf_attr(buf, item->modelResolution);
    }
    AK_3MF_BUF_LIT(buf, "\"/>\n");
  }

  if (opened)
    AK_3MF_BUF_LIT(buf, "        </pa:alternatives>\n");
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

  AK_3MF_BUF_LIT(buf, "            <b:beam v1=\"");
  ak_3mf_buf_u32(buf, beam->v1);
  AK_3MF_BUF_LIT(buf, "\" v2=\"");
  ak_3mf_buf_u32(buf, beam->v2);
  if ((beam->flags & AK_PRINT_BEAM_HAS_R1) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" r1=\"");
    ak_3mf_buf_float(buf, beam->r1);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_R2) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" r2=\"");
    ak_3mf_buf_float(buf, beam->r2);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_P1) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" p1=\"");
    ak_3mf_buf_u32(buf, beam->p1);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_P2) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" p2=\"");
    ak_3mf_buf_u32(buf, beam->p2);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_PID) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" pid=\"");
    ak_3mf_buf_u32(buf, beam->pid);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_CAP1) != 0u && beam->cap1) {
    AK_3MF_BUF_LIT(buf, "\" cap1=\"");
    ak_3mf_buf_attr(buf, beam->cap1);
  }
  if ((beam->flags & AK_PRINT_BEAM_HAS_CAP2) != 0u && beam->cap2) {
    AK_3MF_BUF_LIT(buf, "\" cap2=\"");
    ak_3mf_buf_attr(buf, beam->cap2);
  }
  AK_3MF_BUF_LIT(buf, "\"/>\n");
}

static
void
ak_3mf_write_beam_ball(AK3MFBuffer           * __restrict buf,
                       const AkPrintBeamBall * __restrict ball) {
  if (!ball)
    return;

  AK_3MF_BUF_LIT(buf, "            <b2:ball vindex=\"");
  ak_3mf_buf_u32(buf, ball->vindex);
  if ((ball->flags & AK_PRINT_BEAM_BALL_HAS_RADIUS) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" r=\"");
    ak_3mf_buf_float(buf, ball->radius);
  }
  if ((ball->flags & AK_PRINT_BEAM_BALL_HAS_P) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" p=\"");
    ak_3mf_buf_u32(buf, ball->p);
  }
  if ((ball->flags & AK_PRINT_BEAM_BALL_HAS_PID) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" pid=\"");
    ak_3mf_buf_u32(buf, ball->pid);
  }
  AK_3MF_BUF_LIT(buf, "\"/>\n");
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

  AK_3MF_BUF_LIT(buf, "          <b:beamlattice radius=\"");
  ak_3mf_buf_float(buf, lattice->radius);
  AK_3MF_BUF_LIT(buf, "\" minlength=\"");
  ak_3mf_buf_float(buf, lattice->minLength);
  if (lattice->clippingMode) {
    AK_3MF_BUF_LIT(buf, "\" clippingmode=\"");
    ak_3mf_buf_attr(buf, lattice->clippingMode);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_CLIPPING_MESH) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" clippingmesh=\"");
    ak_3mf_buf_u32(buf, lattice->clippingMesh);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_REPRESENTATION_MESH) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" representationmesh=\"");
    ak_3mf_buf_u32(buf, lattice->representationMesh);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_PID) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" pid=\"");
    ak_3mf_buf_u32(buf, lattice->pid);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_PINDEX) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" pindex=\"");
    ak_3mf_buf_u32(buf, lattice->pindex);
  }
  if (lattice->cap) {
    AK_3MF_BUF_LIT(buf, "\" cap=\"");
    ak_3mf_buf_attr(buf, lattice->cap);
  }
  if (lattice->ballMode) {
    AK_3MF_BUF_LIT(buf, "\" b2:ballmode=\"");
    ak_3mf_buf_attr(buf, lattice->ballMode);
  }
  if ((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_BALL_RADIUS) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" b2:ballradius=\"");
    ak_3mf_buf_float(buf, lattice->ballRadius);
  }
  AK_3MF_BUF_LIT(buf, "\">\n"
                      "            <b:beams>\n");

  for (i = 0u; i < lattice->beamCount && beam; i++, beam = beam->next)
    ak_3mf_write_beam(buf, beam);

  AK_3MF_BUF_LIT(buf, "            </b:beams>\n");
  if (lattice->ballCount > 0u) {
    AK_3MF_BUF_LIT(buf, "            <b2:balls>\n");
    for (i = 0u; i < lattice->ballCount && ball; i++, ball = ball->next)
      ak_3mf_write_beam_ball(buf, ball);
    AK_3MF_BUF_LIT(buf, "            </b2:balls>\n");
  }
  AK_3MF_BUF_LIT(buf, "          </b:beamlattice>\n");
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

  ak_3mf_append_vertex(vertices, a[0], a[1], a[2], displacementMesh);
  ak_3mf_append_vertex(vertices, b[0], b[1], b[2], displacementMesh);
  ak_3mf_append_vertex(vertices, c[0], c[1], c[2], displacementMesh);
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
ak_3mf_emit_indexed_position_primitive(AK3MFRows       * __restrict rows,
                                       AkMeshPrimitive * __restrict prim,
                                       AkInput         * __restrict posInput,
                                       AK3MFBuffer     * __restrict vertices,
                                       AK3MFBuffer     * __restrict triangles,
                                       uint32_t        * __restrict vertexCount,
                                       uint32_t        * __restrict triangleCount) {
  IOTriangleIter iter;
  uint32_t       vi;
  uint32_t       tri[3];

  if (posInput->accessor->count > UINT32_MAX)
    return false;

  for (vi = 0u; vi < posInput->accessor->count; vi++) {
    const float *row;

    row = ak_3mf_rows_get(rows, vi);
    ak_3mf_append_vertex(vertices,
                         ak_3mf_row_component(row, rows->componentCount, 0u, 0.0f),
                         ak_3mf_row_component(row, rows->componentCount, 1u, 0.0f),
                         ak_3mf_row_component(row, rows->componentCount, 2u, 0.0f),
                         false);
  }

  *vertexCount = posInput->accessor->count;
  if (!io_triangle_iter_init(&iter, prim))
    return true;

  while (io_triangle_iter_next(&iter, tri)) {
    AkUInt i0;
    AkUInt i1;
    AkUInt i2;

    i0 = io_primitive_input_index(prim, posInput, tri[0]);
    i1 = io_primitive_input_index(prim, posInput, tri[1]);
    i2 = io_primitive_input_index(prim, posInput, tri[2]);
    if (i0 >= *vertexCount)
      i0 = 0u;
    if (i1 >= *vertexCount)
      i1 = 0u;
    if (i2 >= *vertexCount)
      i2 = 0u;
    ak_3mf_append_triangle(triangles,
                           (uint32_t)i0,
                           (uint32_t)i1,
                           (uint32_t)i2,
                           0u,
                           false,
                           NULL);
    (*triangleCount)++;
  }

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
  IOTriangleIter iter;
  uint32_t       tri[3];

  if (!io_triangle_iter_init(&iter, prim))
    return true;

  while (io_triangle_iter_next(&iter, tri)) {
    if (!ak_3mf_emit_triangle(rows, colorRows, prim, posInput, colorInput,
                              tri[0], tri[1], tri[2],
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

static AK_NOINLINE
void
ak_3mf_write_build_item_open(AK3MFBuffer                 * __restrict buf,
                             uint32_t                                 objectId,
                             const AkPrintProductionItem * __restrict item) {
  AK_3MF_BUF_LIT(buf, "      <item objectid=\"");
  ak_3mf_buf_u32(buf, objectId);
  ak_3mf_append_production_open_attrs(buf, item, true);
}

static AK_NOINLINE
void
ak_3mf_write_build_item(AK3MFBuffer                 * __restrict buf,
                        uint32_t                                 objectId,
                        const AkPrintProductionItem * __restrict item) {
  ak_3mf_write_build_item_open(buf, objectId, item);
  AK_3MF_BUF_LIT(buf, "\"/>\n");
}

static AK_NOINLINE
void
ak_3mf_write_build_item_transform(
                             AK3MFBuffer                 * __restrict buf,
                             uint32_t                                 objectId,
                             const AkPrintProductionItem * __restrict item,
                             mat4                                     world) {
  ak_3mf_write_build_item_open(buf, objectId, item);
  AK_3MF_BUF_LIT(buf, "\" transform=\"");
  ak_3mf_append_transform(buf, world);
  AK_3MF_BUF_LIT(buf, "\"/>\n");
}

static
bool
ak_3mf_write_plain_indexed_primitive(AK3MFExportState * __restrict st,
                                     AkMeshPrimitive  * __restrict prim,
                                     AkInput          * __restrict posInput,
                                     AK3MFRows        * __restrict rows,
                                     mat4                           world) {
  IOTriangleIter iter;
  uint32_t       vertexCount;
  uint32_t       objectId;
  uint32_t       vi;
  uint32_t       tri[3];

  vertexCount = posInput->accessor->count;
  if (!io_triangle_iter_init(&iter, prim))
    return true;

  objectId = st->nextObjectId++;
  AK_3MF_BUF_LIT(&st->resources, "      <object id=\"");
  ak_3mf_buf_u32(&st->resources, objectId);
  AK_3MF_BUF_LIT(&st->resources, "\" type=\"model\">\n"
                                "        <mesh>\n"
                                "          <vertices>\n");

  for (vi = 0u; vi < vertexCount; vi++) {
    const float *row;

    row = ak_3mf_rows_get(rows, vi);
    ak_3mf_append_vertex(&st->resources,
                         ak_3mf_row_component(row, rows->componentCount, 0u, 0.0f),
                         ak_3mf_row_component(row, rows->componentCount, 1u, 0.0f),
                         ak_3mf_row_component(row, rows->componentCount, 2u, 0.0f),
                         false);
  }

  AK_3MF_BUF_LIT(&st->resources, "          </vertices>\n"
                                "          <triangles>\n");

  while (io_triangle_iter_next(&iter, tri)) {
    AkUInt i0;
    AkUInt i1;
    AkUInt i2;

    i0 = io_primitive_input_index(prim, posInput, tri[0]);
    i1 = io_primitive_input_index(prim, posInput, tri[1]);
    i2 = io_primitive_input_index(prim, posInput, tri[2]);
    if (i0 >= vertexCount)
      i0 = 0u;
    if (i1 >= vertexCount)
      i1 = 0u;
    if (i2 >= vertexCount)
      i2 = 0u;
    ak_3mf_append_triangle(&st->resources,
                           (uint32_t)i0,
                           (uint32_t)i1,
                           (uint32_t)i2,
                           0u,
                           false,
                           NULL);
  }

  AK_3MF_BUF_LIT(&st->resources, "          </triangles>\n"
                                "        </mesh>\n"
                                "      </object>\n");
  ak_3mf_write_build_item_transform(&st->build, objectId, NULL, world);

  st->objectCount++;
  return st->resources.result == AK_OK && st->build.result == AK_OK;
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
  const AkPrintProductionItem *productionObject;
  const AkPrintProductionItem *productionItem;
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

  posInput = io_primitive_find_input(prim, AK_INPUT_POSITION);
  if (!posInput || !posInput->accessor)
    return true;

  colorInput = ak_3mf_find_color_input(prim);
  if (!ak_3mf_rows_init(&rows, posInput->accessor))
    return false;

  if (!st->print
      && !st->suppressBuildItems
      && prim->type == AK_PRIMITIVE_TRIANGLES
      && !colorInput) {
    ok = ak_3mf_write_plain_indexed_primitive(st, prim, posInput, &rows, world);
    ak_3mf_rows_destroy(&rows);
    return ok;
  }

  memset(&colorRows, 0, sizeof(colorRows));
  hasColorRows = colorInput && ak_3mf_rows_init(&colorRows, colorInput->accessor);
  propertyId   = hasColorRows ? st->nextObjectId++ : 0u;

  io_buffer_init(&vertices);
  io_buffer_init(&colors);
  io_buffer_init(&triangles);
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

  if (!displacementMesh
      && !hasColorRows
      && prim->type == AK_PRIMITIVE_TRIANGLES) {
    ok = ak_3mf_emit_indexed_position_primitive(&rows,
                                                prim,
                                                posInput,
                                                &vertices,
                                                &triangles,
                                                &vertexCount,
                                                &triangleCount);
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
  productionObject = ak_3mf_production_object_for_export(st, objectId);
  if (productionObject
      && productionObject->objectId != 0u
      && productionObject->objectId != propertyId) {
    objectId = productionObject->objectId;
    if (st->nextObjectId <= objectId)
      st->nextObjectId = objectId + 1u;
  }
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
    AK_3MF_BUF_LIT(&st->resources, "      <m:colorgroup id=\"");
    ak_3mf_buf_u32(&st->resources, propertyId);
    AK_3MF_BUF_LIT(&st->resources, "\">\n");
    ak_3mf_buf_raw(&st->resources, colors.data, colors.len);
    AK_3MF_BUF_LIT(&st->resources, "      </m:colorgroup>\n");
    st->usesMaterialExtension = true;
  }

  AK_3MF_BUF_LIT(&st->resources, "      <object id=\"");
  ak_3mf_buf_u32(&st->resources, objectId);
  ak_3mf_append_production_object_open_attrs(&st->resources, productionObject);
  ak_3mf_append_slice_object_attrs(&st->resources, sliceObject);
  AK_3MF_BUF_LIT(&st->resources, "\" type=\"model\">\n");
  if (displacementMesh) {
    AK_3MF_BUF_LIT(&st->resources, "        <d:displacementmesh>\n"
                                  "          <d:vertices>\n");
  } else {
    AK_3MF_BUF_LIT(&st->resources, "        <mesh");
    if (volumetricMesh
        && (volumetricMesh->flags & AK_PRINT_VOLUMETRIC_MESH_HAS_VOLUME_ID) != 0u) {
      AK_3MF_BUF_LIT(&st->resources, " volumeid=\"");
      ak_3mf_buf_u32(&st->resources, volumetricMesh->volumeId);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    AK_3MF_BUF_LIT(&st->resources, ">\n"
                                  "          <vertices>\n");
  }
  ak_3mf_buf_raw(&st->resources, vertices.data, vertices.len);
  ak_3mf_buf_lit(&st->resources, displacementMesh
                                ? "          </d:vertices>\n"
                                : "          </vertices>\n");
  if (triangleCount > 0u) {
    if (displacementMesh) {
      AK_3MF_BUF_LIT(&st->resources, "          <d:triangles");
      if ((displacementMesh->flags
           & AK_PRINT_DISPLACEMENT_MESH_HAS_DEFAULT_GROUP) != 0u) {
        AK_3MF_BUF_LIT(&st->resources, " did=\"");
        ak_3mf_buf_u32(&st->resources, displacementMesh->defaultGroupId);
        ak_3mf_buf_ch(&st->resources, '"');
      }
      AK_3MF_BUF_LIT(&st->resources, ">\n");
    } else {
      AK_3MF_BUF_LIT(&st->resources, "          <triangles>\n");
    }
    ak_3mf_buf_raw(&st->resources, triangles.data, triangles.len);
    ak_3mf_buf_lit(&st->resources, displacementMesh
                                  ? "          </d:triangles>\n"
                                  : "          </triangles>\n");
  }
  ak_3mf_write_beam_lattice(st, &st->resources, beamLattice);
  ak_3mf_buf_lit(&st->resources, displacementMesh
                                ? "        </d:displacementmesh>\n"
                                : "        </mesh>\n");
  ak_3mf_write_production_alternatives(st, &st->resources, objectId);
  AK_3MF_BUF_LIT(&st->resources, "      </object>\n");

  if (!st->suppressBuildItems) {
    productionItem = ak_3mf_production_item_for_export(st, objectId);
    ak_3mf_write_build_item_transform(&st->build,
                                      objectId,
                                      productionItem,
                                      world);
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

  AK_3MF_BUF_LIT(buf, "      <d:displacement2d id=\"");
  ak_3mf_buf_u32(buf, displacement->id);
  AK_3MF_BUF_LIT(buf, "\" path=\"/");
  ak_3mf_buf_attr(buf, displacement->imagePath);
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_2D_HAS_CHANNEL) != 0u
      && displacement->channel) {
    AK_3MF_BUF_LIT(buf, "\" channel=\"");
    ak_3mf_buf_attr(buf, displacement->channel);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_2D_HAS_TILESTYLE_U) != 0u
      && displacement->tileStyleU) {
    AK_3MF_BUF_LIT(buf, "\" tilestyleu=\"");
    ak_3mf_buf_attr(buf, displacement->tileStyleU);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_2D_HAS_TILESTYLE_V) != 0u
      && displacement->tileStyleV) {
    AK_3MF_BUF_LIT(buf, "\" tilestylev=\"");
    ak_3mf_buf_attr(buf, displacement->tileStyleV);
  }
  if ((displacement->flags & AK_PRINT_DISPLACEMENT_2D_HAS_FILTER) != 0u
      && displacement->filter) {
    AK_3MF_BUF_LIT(buf, "\" filter=\"");
    ak_3mf_buf_attr(buf, displacement->filter);
  }
  AK_3MF_BUF_LIT(buf, "\"/>\n");
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
  AK_3MF_BUF_LIT(&st->resources, "      <d:normvectorgroup id=\"");
  ak_3mf_buf_u32(&st->resources, group->id);
  AK_3MF_BUF_LIT(&st->resources, "\">\n");
  for (i = 0u; i < group->vectorCount && vector; i++, vector = vector->next) {
    AK_3MF_BUF_LIT(&st->resources, "        <d:normvector x=\"");
    ak_3mf_buf_float(&st->resources, vector->x);
    AK_3MF_BUF_LIT(&st->resources, "\" y=\"");
    ak_3mf_buf_float(&st->resources, vector->y);
    AK_3MF_BUF_LIT(&st->resources, "\" z=\"");
    ak_3mf_buf_float(&st->resources, vector->z);
    AK_3MF_BUF_LIT(&st->resources, "\"/>\n");
  }
  AK_3MF_BUF_LIT(&st->resources, "      </d:normvectorgroup>\n");
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
  AK_3MF_BUF_LIT(&st->resources, "      <d:disp2dgroup id=\"");
  ak_3mf_buf_u32(&st->resources, group->id);
  AK_3MF_BUF_LIT(&st->resources, "\" dispid=\"");
  ak_3mf_buf_u32(&st->resources, group->displacementId);
  AK_3MF_BUF_LIT(&st->resources, "\" nid=\"");
  ak_3mf_buf_u32(&st->resources, group->normVectorGroupId);
  AK_3MF_BUF_LIT(&st->resources, "\" height=\"");
  ak_3mf_buf_float(&st->resources, group->height);
  if ((group->flags & AK_PRINT_DISP2D_GROUP_HAS_OFFSET) != 0u) {
    AK_3MF_BUF_LIT(&st->resources, "\" offset=\"");
    ak_3mf_buf_float(&st->resources, group->offset);
  }
  AK_3MF_BUF_LIT(&st->resources, "\">\n");
  for (i = 0u; i < group->coordCount && coord; i++, coord = coord->next) {
    AK_3MF_BUF_LIT(&st->resources, "        <d:disp2dcoord u=\"");
    ak_3mf_buf_float(&st->resources, coord->u);
    AK_3MF_BUF_LIT(&st->resources, "\" v=\"");
    ak_3mf_buf_float(&st->resources, coord->v);
    AK_3MF_BUF_LIT(&st->resources, "\" n=\"");
    ak_3mf_buf_u32(&st->resources, coord->normVectorIndex);
    if ((coord->flags & AK_PRINT_DISP2D_COORD_HAS_FACTOR) != 0u) {
      AK_3MF_BUF_LIT(&st->resources, "\" f=\"");
      ak_3mf_buf_float(&st->resources, coord->factor);
    }
    AK_3MF_BUF_LIT(&st->resources, "\"/>\n");
  }
  AK_3MF_BUF_LIT(&st->resources, "      </d:disp2dgroup>\n");
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

  AK_3MF_BUF_LIT(buf, "\" path=\"/");
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
  AK_3MF_BUF_LIT(&st->resources, "      <v:image3d id=\"");
  ak_3mf_buf_u32(&st->resources, image->id);
  ak_3mf_buf_ch(&st->resources, '"');
  if (image->name) {
    AK_3MF_BUF_LIT(&st->resources, " name=\"");
    ak_3mf_buf_attr(&st->resources, image->name);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  AK_3MF_BUF_LIT(&st->resources, ">\n"
                                "        <v:imagestack rowcount=\"");
  ak_3mf_buf_u32(&st->resources, image->rowCount);
  AK_3MF_BUF_LIT(&st->resources, "\" columncount=\"");
  ak_3mf_buf_u32(&st->resources, image->columnCount);
  AK_3MF_BUF_LIT(&st->resources, "\" sheetcount=\"");
  ak_3mf_buf_u32(&st->resources, image->sheetCount);
  AK_3MF_BUF_LIT(&st->resources, "\">\n");

  for (i = 0u; i < image->imageSheetCount && sheet; i++, sheet = sheet->next) {
    AK_3MF_BUF_LIT(&st->resources, "          <v:imagesheet path=\"/");
    ak_3mf_buf_attr(&st->resources, sheet->path);
    AK_3MF_BUF_LIT(&st->resources, "\"/>\n");
  }

  AK_3MF_BUF_LIT(&st->resources, "        </v:imagestack>\n"
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

  AK_3MF_BUF_LIT(&st->resources, "      <v:functionfromimage3d id=\"");
  ak_3mf_buf_u32(&st->resources, function->id);
  AK_3MF_BUF_LIT(&st->resources, "\" image3did=\"");
  ak_3mf_buf_u32(&st->resources, function->image3DId);
  ak_3mf_buf_ch(&st->resources, '"');
  if (function->displayName) {
    AK_3MF_BUF_LIT(&st->resources, " displayname=\"");
    ak_3mf_buf_attr(&st->resources, function->displayName);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_OFFSET) != 0u) {
    AK_3MF_BUF_LIT(&st->resources, " valueoffset=\"");
    ak_3mf_buf_float(&st->resources, function->valueOffset);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_SCALE) != 0u) {
    AK_3MF_BUF_LIT(&st->resources, " valuescale=\"");
    ak_3mf_buf_float(&st->resources, function->valueScale);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_FILTER) != 0u
      && function->filter) {
    AK_3MF_BUF_LIT(&st->resources, " filter=\"");
    ak_3mf_buf_attr(&st->resources, function->filter);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_U) != 0u
      && function->tileStyleU) {
    AK_3MF_BUF_LIT(&st->resources, " tilestyleu=\"");
    ak_3mf_buf_attr(&st->resources, function->tileStyleU);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_V) != 0u
      && function->tileStyleV) {
    AK_3MF_BUF_LIT(&st->resources, " tilestylev=\"");
    ak_3mf_buf_attr(&st->resources, function->tileStyleV);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  if ((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_W) != 0u
      && function->tileStyleW) {
    AK_3MF_BUF_LIT(&st->resources, " tilestylew=\"");
    ak_3mf_buf_attr(&st->resources, function->tileStyleW);
    ak_3mf_buf_ch(&st->resources, '"');
  }
  AK_3MF_BUF_LIT(&st->resources, "/>\n");
}

static
void
ak_3mf_write_implicit_function(AK3MFExportState                 * __restrict st,
                               const AkPrintImplicitFunction    * __restrict function) {
  if (!st || !function)
    return;
  if (function->path && !ak_3mf_path_is_root_model(function->path))
    return;
  if (!function->xml)
    return;

  AK_3MF_BUF_LIT(&st->resources, "      ");
  ak_3mf_buf_lit(&st->resources, function->xml);
  ak_3mf_buf_ch(&st->resources, '\n');
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

  AK_3MF_BUF_LIT(buf, "        <v:");
  ak_3mf_buf_lit(buf, tag);
  AK_3MF_BUF_LIT(buf, " functionid=\"");
  ak_3mf_buf_u32(buf, element->functionId);
  ak_3mf_buf_ch(buf, '"');
  if (element->channel) {
    AK_3MF_BUF_LIT(buf, " channel=\"");
    ak_3mf_buf_attr(buf, element->channel);
    ak_3mf_buf_ch(buf, '"');
  }
  if (element->type == AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY) {
    if (element->name) {
      AK_3MF_BUF_LIT(buf, " name=\"");
      ak_3mf_buf_attr(buf, element->name);
      ak_3mf_buf_ch(buf, '"');
    }
    AK_3MF_BUF_LIT(buf, " required=\"");
    ak_3mf_buf_lit(buf,
                   (element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_REQUIRED) != 0u
                   ? "true"
                   : "false");
    ak_3mf_buf_ch(buf, '"');
  }
  if ((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_HAS_TRANSFORM) != 0u) {
    AK_3MF_BUF_LIT(buf, " transform=\"");
    ak_3mf_append_flat_transform(buf, element->matrix);
    ak_3mf_buf_ch(buf, '"');
  }
  if ((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_HAS_MIN_FEATURE_SIZE) != 0u) {
    AK_3MF_BUF_LIT(buf, " minfeaturesize=\"");
    ak_3mf_buf_float(buf, element->minFeatureSize);
    ak_3mf_buf_ch(buf, '"');
  }
  if ((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_HAS_FALLBACK_VALUE) != 0u) {
    AK_3MF_BUF_LIT(buf, " fallbackvalue=\"");
    ak_3mf_buf_float(buf, element->fallbackValue);
    ak_3mf_buf_ch(buf, '"');
  }
  AK_3MF_BUF_LIT(buf, "/>\n");
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

  AK_3MF_BUF_LIT(&st->resources, "      <v:volumedata id=\"");
  ak_3mf_buf_u32(&st->resources, volume->id);
  AK_3MF_BUF_LIT(&st->resources, "\">\n");

  if (hasComposite) {
    AK_3MF_BUF_LIT(&st->resources, "        <v:composite basematerialid=\"");
    ak_3mf_buf_u32(&st->resources, volume->baseMaterialId);
    AK_3MF_BUF_LIT(&st->resources, "\">\n");
    ak_3mf_write_volume_elements_of_type(&st->resources,
                                         element,
                                         total,
                                         AK_PRINT_VOLUMETRIC_ELEMENT_MATERIAL_MAPPING);
    AK_3MF_BUF_LIT(&st->resources, "        </v:composite>\n");
  }

  ak_3mf_write_volume_elements_of_type(&st->resources,
                                       element,
                                       total,
                                       AK_PRINT_VOLUMETRIC_ELEMENT_COLOR);
  ak_3mf_write_volume_elements_of_type(&st->resources,
                                       element,
                                       total,
                                       AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY);
  AK_3MF_BUF_LIT(&st->resources, "      </v:volumedata>\n");
}

static
bool
ak_3mf_write_volumetric_resources(AK3MFExportState * __restrict st) {
  const AkPrintImage3D             *image;
  const AkPrintFunctionFromImage3D *function;
  const AkPrintImplicitFunction    *implicitFunction;
  const AkPrintVolumeData          *volume;

  if (!st || !st->print || !st->usesVolumetricExtension)
    return true;

  for (image = st->print->image3Ds; image; image = image->next)
    ak_3mf_write_image3d(st, image);
  for (function = st->print->functionFromImage3Ds; function; function = function->next)
    ak_3mf_write_function_from_image3d(st, function);
  for (implicitFunction = st->print->implicitFunctions;
       implicitFunction;
       implicitFunction = implicitFunction->next)
    ak_3mf_write_implicit_function(st, implicitFunction);
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
    const AkPrintProductionItem *productionObject;
    const AkPrintProductionItem *productionItem;
    uint32_t objectId;

    if (levelSet->path && !ak_3mf_path_is_root_model(levelSet->path))
      continue;

    objectId = levelSet->objectId;
    if (objectId == 0u || objectId < st->nextObjectId)
      objectId = st->nextObjectId;
    if (st->nextObjectId <= objectId)
      st->nextObjectId = objectId + 1u;

    AK_3MF_BUF_LIT(&st->resources, "      <object id=\"");
    ak_3mf_buf_u32(&st->resources, objectId);
    productionObject = ak_3mf_production_object_for_export(st, objectId);
    ak_3mf_append_production_object_open_attrs(&st->resources, productionObject);
    AK_3MF_BUF_LIT(&st->resources, "\" type=\"model\">\n"
                                  "        <v:levelset functionid=\"");
    ak_3mf_buf_u32(&st->resources, levelSet->functionId);
    ak_3mf_buf_ch(&st->resources, '"');
    if (levelSet->channel) {
      AK_3MF_BUF_LIT(&st->resources, " channel=\"");
      ak_3mf_buf_attr(&st->resources, levelSet->channel);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if (levelSet->meshId != 0u) {
      AK_3MF_BUF_LIT(&st->resources, " meshid=\"");
      ak_3mf_buf_u32(&st->resources, levelSet->meshId);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_VOLUME_ID) != 0u) {
      AK_3MF_BUF_LIT(&st->resources, " volumeid=\"");
      ak_3mf_buf_u32(&st->resources, levelSet->volumeId);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_TRANSFORM) != 0u) {
      AK_3MF_BUF_LIT(&st->resources, " transform=\"");
      ak_3mf_append_flat_transform(&st->resources, levelSet->matrix);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_MIN_FEATURE_SIZE) != 0u) {
      AK_3MF_BUF_LIT(&st->resources, " minfeaturesize=\"");
      ak_3mf_buf_float(&st->resources, levelSet->minFeatureSize);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_MESH_BBOX_ONLY) != 0u) {
      AK_3MF_BUF_LIT(&st->resources, " meshbboxonly=\"true\"");
    }
    if ((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_FALLBACK_VALUE) != 0u) {
      AK_3MF_BUF_LIT(&st->resources, " fallbackvalue=\"");
      ak_3mf_buf_float(&st->resources, levelSet->fallbackValue);
      ak_3mf_buf_ch(&st->resources, '"');
    }
    AK_3MF_BUF_LIT(&st->resources, "/>\n");
    ak_3mf_write_production_alternatives(st, &st->resources, objectId);
    AK_3MF_BUF_LIT(&st->resources, "      </object>\n");

    productionItem = ak_3mf_production_item_for_export(st, objectId);
    ak_3mf_write_build_item(&st->build, objectId, productionItem);
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

  AK_3MF_BUF_LIT(buf, "          <bo:boolean objectid=\"");
  ak_3mf_buf_u32(buf, operand->objectId);
  if ((operand->flags & AK_PRINT_BOOLEAN_OPERAND_HAS_TRANSFORM) != 0u) {
    AK_3MF_BUF_LIT(buf, "\" transform=\"");
    ak_3mf_append_flat_transform(buf, operand->matrix);
  }
  ak_3mf_append_optional_3mf_path(buf, operand->path);
  AK_3MF_BUF_LIT(buf, "\"/>\n");
}

static
bool
ak_3mf_write_boolean_shapes(AK3MFExportState * __restrict st) {
  const AkPrintBooleanShape *shape;

  if (!st || !st->print || !st->usesBooleanExtension)
    return true;

  for (shape = st->print->booleanShapes; shape; shape = shape->next) {
    const AkPrintBooleanOperand *operand;
    const AkPrintProductionItem *productionObject;
    const AkPrintProductionItem *productionItem;
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

    AK_3MF_BUF_LIT(&st->resources, "      <object id=\"");
    ak_3mf_buf_u32(&st->resources, objectId);
    productionObject = ak_3mf_production_object_for_export(st, objectId);
    ak_3mf_append_production_object_open_attrs(&st->resources, productionObject);
    AK_3MF_BUF_LIT(&st->resources, "\" type=\"model\">\n"
                                  "        <bo:booleanshape objectid=\"");
    ak_3mf_buf_u32(&st->resources, shape->baseObjectId);
    AK_3MF_BUF_LIT(&st->resources, "\" operation=\"");
    ak_3mf_buf_lit(&st->resources, ak_3mf_boolean_operation_name(shape->operation));
    if ((shape->flags & AK_PRINT_BOOLEAN_SHAPE_HAS_TRANSFORM) != 0u) {
      AK_3MF_BUF_LIT(&st->resources, "\" transform=\"");
      ak_3mf_append_flat_transform(&st->resources, shape->matrix);
    }
    ak_3mf_append_optional_3mf_path(&st->resources, shape->basePath);
    AK_3MF_BUF_LIT(&st->resources, "\">\n");

    for (i = 0u; i < shape->operandCount && operand; i++, operand = operand->next)
      ak_3mf_write_boolean_operand(&st->resources, operand);

    AK_3MF_BUF_LIT(&st->resources, "        </bo:booleanshape>\n");
    ak_3mf_write_production_alternatives(st, &st->resources, objectId);
    AK_3MF_BUF_LIT(&st->resources, "      </object>\n");

    productionItem = ak_3mf_production_item_for_export(st, objectId);
    ak_3mf_write_build_item(&st->build, objectId, productionItem);
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

  AK_3MF_BUF_LIT(buf, "        <s:sliceref slicestackid=\"");
  ak_3mf_buf_u32(buf, ref->stackId);
  if (ref->path) {
    AK_3MF_BUF_LIT(buf, "\" slicepath=\"/");
    ak_3mf_buf_attr(buf, ref->path);
  }
  AK_3MF_BUF_LIT(buf, "\" ztop=\"");
  ak_3mf_buf_float(buf, ref->zTop);
  AK_3MF_BUF_LIT(buf, "\"/>\n");
}

static
void
ak_3mf_write_empty_slice(AK3MFBuffer         * __restrict buf,
                         const AkPrintSlice  * __restrict slice) {
  if (!slice)
    return;

  AK_3MF_BUF_LIT(buf, "        <s:slice ztop=\"");
  ak_3mf_buf_float(buf, slice->zTop);
  AK_3MF_BUF_LIT(buf, "\"/>\n");
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
      AK_3MF_BUF_LIT(&st->resources, "      <s:slicestack id=\"");
      ak_3mf_buf_u32(&st->resources, stack->id);
      AK_3MF_BUF_LIT(&st->resources, "\" zbottom=\"");
      ak_3mf_buf_float(&st->resources, stack->zBottom);
      AK_3MF_BUF_LIT(&st->resources, "\">\n");
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
      AK_3MF_BUF_LIT(&st->resources, "      </s:slicestack>\n");
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

  io_buffer_init(model);
  unitName = ak_3mf_export_unit_name(st->doc);

  AK_3MF_BUF_LIT(model, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<model unit=\"");
  ak_3mf_buf_lit(model, unitName);
  AK_3MF_BUF_LIT(model, "\" xml:lang=\"en-US\" "
                        "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\"");
  if (st->usesMaterialExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:m=\"http://schemas.microsoft.com/3dmanufacturing/material/2015/02\"");
  }
  if (st->usesProductionExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\"");
  }
  if (st->usesProductionAlternativeExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:pa=\"http://schemas.microsoft.com/3dmanufacturing/production/alternatives/2021/04\"");
  }
  if (st->usesSliceExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:s=\"http://schemas.microsoft.com/3dmanufacturing/slice/2015/07\"");
  }
  if (st->usesBeamLatticeExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:b=\"http://schemas.microsoft.com/3dmanufacturing/beamlattice/2017/02\"");
  }
  if (st->usesBeamBallExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:b2=\"http://schemas.microsoft.com/3dmanufacturing/beamlattice/balls/2020/07\"");
  }
  if (st->usesBooleanExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:bo=\"http://schemas.3mf.io/3dmanufacturing/booleanoperations/2023/07\"");
  }
  if (st->usesDisplacementExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:d=\"http://schemas.3mf.io/3dmanufacturing/displacement/2023/10\"");
  }
  if (st->usesVolumetricExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:v=\"http://schemas.3mf.io/3dmanufacturing/volumetric/2022/01\"");
  }
  if (st->usesImplicitExtension) {
    AK_3MF_BUF_LIT(model,
                   " xmlns:i=\"http://schemas.3mf.io/3dmanufacturing/implicit/2023/12\"");
  }
  if (st->usesMaterialExtension
      || st->usesProductionExtension
      || st->usesProductionAlternativeExtension
      || st->usesSliceExtension
      || st->usesBeamLatticeExtension
      || st->usesBeamBallExtension
      || st->usesBooleanExtension
      || st->usesDisplacementExtension
      || st->usesVolumetricExtension
      || st->usesImplicitExtension) {
    bool any;

    any = false;
    AK_3MF_BUF_LIT(model, " requiredextensions=\"");
    if (st->usesMaterialExtension) {
      AK_3MF_BUF_LIT(model, "m");
      any = true;
    }
    if (st->usesProductionExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      AK_3MF_BUF_LIT(model, "p");
      any = true;
    }
    if (st->usesProductionAlternativeExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      AK_3MF_BUF_LIT(model, "pa");
      any = true;
    }
    if (st->usesSliceExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      AK_3MF_BUF_LIT(model, "s");
      any = true;
    }
    if (st->usesBeamLatticeExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      AK_3MF_BUF_LIT(model, "b");
      any = true;
    }
    if (st->usesBeamBallExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      AK_3MF_BUF_LIT(model, "b2");
      any = true;
    }
    if (st->usesBooleanExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      AK_3MF_BUF_LIT(model, "bo");
      any = true;
    }
    if (st->usesDisplacementExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      AK_3MF_BUF_LIT(model, "d");
      any = true;
    }
    if (st->usesVolumetricExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      AK_3MF_BUF_LIT(model, "v");
      any = true;
    }
    if (st->usesImplicitExtension) {
      if (any)
        ak_3mf_buf_ch(model, ' ');
      AK_3MF_BUF_LIT(model, "i");
    }
    ak_3mf_buf_ch(model, '"');
  }
  AK_3MF_BUF_LIT(model, ">\n"
                        "  <resources>\n");
  ak_3mf_buf_raw(model, st->resources.data, st->resources.len);
  AK_3MF_BUF_LIT(model, "  </resources>\n"
                        "  <build");
  if (st->usesProductionExtension) {
    ak_3mf_append_production_attrs(model,
                                   ak_3mf_first_production_item(st->print,
                                                                AK_PRINT_PRODUCTION_BUILD),
                                   false);
  }
  AK_3MF_BUF_LIT(model, ">\n");
  ak_3mf_buf_raw(model, st->build.data, st->build.len);
  AK_3MF_BUF_LIT(model, "  </build>\n"
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

  io_buffer_init(contentTypes);

  AK_3MF_BUF_LIT(contentTypes,
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
                 "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
                 "  <Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n");

  for (part = print ? print->parts : NULL; part; part = part->next) {
    const char *name;

    if (!ak_3mf_extra_part_exportable(part) || !part->contentType)
      continue;

    name = ak_3mf_zip_part_name(part->name);
    AK_3MF_BUF_LIT(contentTypes, "  <Override PartName=\"/");
    ak_3mf_buf_attr(contentTypes, name);
    AK_3MF_BUF_LIT(contentTypes, "\" ContentType=\"");
    ak_3mf_buf_attr(contentTypes, part->contentType);
    AK_3MF_BUF_LIT(contentTypes, "\"/>\n");
  }

  AK_3MF_BUF_LIT(contentTypes, "</Types>\n");
  return contentTypes->result == AK_OK;
}

static
bool
ak_3mf_build_rels_xml(const AkPrintDocument * __restrict print,
                      AK3MFBuffer           * __restrict rels) {
  const AkPrintPackagePart *part;
  uint32_t                  relId;

  io_buffer_init(rels);
  relId = 1u;

  AK_3MF_BUF_LIT(rels,
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
                 "  <Relationship Target=\"/3D/3dmodel.model\" Id=\"rel0\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>\n");

  for (part = print ? print->parts : NULL; part; part = part->next) {
    const char *name;

    if (!ak_3mf_extra_part_exportable(part) || !part->relationshipType)
      continue;

    name = ak_3mf_zip_part_name(part->name);
    AK_3MF_BUF_LIT(rels, "  <Relationship Target=\"/");
    ak_3mf_buf_attr(rels, name);
    if (part->relationshipId) {
      AK_3MF_BUF_LIT(rels, "\" Id=\"");
      ak_3mf_buf_attr(rels, part->relationshipId);
      relId++;
    } else {
      AK_3MF_BUF_LIT(rels, "\" Id=\"rel");
      ak_3mf_buf_u32(rels, relId++);
    }
    AK_3MF_BUF_LIT(rels, "\" Type=\"");
    ak_3mf_buf_attr(rels, part->relationshipType);
    if (part->relationshipTargetMode) {
      AK_3MF_BUF_LIT(rels, "\" TargetMode=\"");
      ak_3mf_buf_attr(rels, part->relationshipTargetMode);
    }
    AK_3MF_BUF_LIT(rels, "\"/>\n");
  }

  AK_3MF_BUF_LIT(rels, "</Relationships>\n");
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
  io_buffer_init(&st.resources);
  io_buffer_init(&st.build);
  st.doc          = doc;
  st.print        = ak_printDocument(doc);
  st.result       = AK_OK;
  st.nextObjectId = 1u;
  st.usesProductionExtension = st.print && st.print->productionItemCount > 0u;
  st.usesProductionAlternativeExtension =
    ak_3mf_uses_production_alternative_extension(st.print);
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
  st.usesImplicitExtension = st.print && st.print->implicitFunctionCount > 0u;
  st.usesVolumetricExtension = st.print
                               && (st.print->image3DCount > 0u
                                   || st.print->imageSheetCount > 0u
                                   || st.print->functionFromImage3DCount > 0u
                                   || st.print->implicitFunctionCount > 0u
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
