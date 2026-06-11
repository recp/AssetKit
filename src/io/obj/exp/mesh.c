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

#include "mesh.h"
#include "material.h"
#include "writer.h"
#include "../../common/primitive.h"

#include <stdlib.h>
#include <string.h>

typedef struct WOBJExpRows {
  AkAccessor *accessor;
  float      *scratch;
  uint32_t    componentCount;
  bool        direct;
} WOBJExpRows;

static
AkInput*
wobj_find_input(AkMeshPrimitive * __restrict prim,
                AkInputSemantic              semantic) {
  AkInput *input;

  if (!prim)
    return NULL;

  if (semantic == AK_INPUT_POSITION && prim->pos)
    return prim->pos;

  for (input = prim->input; input; input = input->next) {
    if (input->semantic == semantic)
      return input;
  }

  return NULL;
}

static
AkInput*
wobj_find_vertex_color_input(AkMeshPrimitive * __restrict prim,
                             AkInput         * __restrict posInput) {
  AkInput *input;

  if (!prim || !posInput || !posInput->accessor)
    return NULL;

  for (input = prim->input; input; input = input->next) {
    if (input->semantic != AK_INPUT_COLOR
        || !input->accessor
        || input->accessor->count != posInput->accessor->count
        || input->accessor->componentCount < 3u
        || input->indexOffset != posInput->indexOffset)
      continue;
    return input;
  }

  return NULL;
}

static
AkInput*
wobj_find_texcoord_input(AkMeshPrimitive * __restrict prim) {
  AkInput *input;
  AkInput *fallback;

  fallback = NULL;
  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if (input->semantic != AK_INPUT_TEXCOORD && input->semantic != AK_INPUT_UV)
      continue;
    if (input->set == 0)
      return input;
    if (!fallback)
      fallback = input;
  }

  return fallback;
}

static
bool
wobj_rows_init(WOBJExpRows * __restrict rows,
               AkAccessor  * __restrict acc) {
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
wobj_rows_destroy(WOBJExpRows * __restrict rows) {
  free(rows->scratch);
  rows->scratch = NULL;
}

static
const float*
wobj_rows_get(WOBJExpRows * __restrict rows, uint32_t index) {
  if (index >= rows->accessor->count)
    index = 0;

  return rows->direct
         ? io_accessor_float_row(rows->accessor, index)
         : rows->scratch + (size_t)index * rows->componentCount;
}

static
float
wobj_row_component(const float * __restrict row,
                   uint32_t                 componentCount,
                   uint32_t                 component,
                   float                    fallback) {
  return component < componentCount ? row[component] : fallback;
}

static
float
wobj_color_component(WOBJExpRows       * __restrict rows,
                     const float       * __restrict row,
                     uint32_t                       component,
                     float                          fallback) {
  float value;

  value = wobj_row_component(row, rows->componentCount, component, fallback);

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

  return value;
}

static
bool
wobj_count_add(uint32_t * __restrict value, uint32_t add) {
  if (UINT32_MAX - *value < add)
    return false;
  *value += add;
  return true;
}

static
void
wobj_write_v3(WOBJExpWriter * __restrict w,
              const char    * __restrict prefix,
              vec3                       value) {
  wobj_w_lit(w, prefix);
  wobj_w_float(w, value[0]);
  wobj_w_ch(w, ' ');
  wobj_w_float(w, value[1]);
  wobj_w_ch(w, ' ');
  wobj_w_float(w, value[2]);
  wobj_w_ch(w, '\n');
}

static
bool
wobj_write_positions(WOBJExpState * __restrict st,
                     WOBJExpRows  * __restrict rows,
                     WOBJExpRows  * __restrict colorRows,
                     mat4                       world) {
  uint32_t i;

  if (!wobj_count_add(&st->vCount, rows->accessor->count))
    return false;

  for (i = 0; i < rows->accessor->count; i++) {
    const float *row;
    vec3         in;
    vec3         out;

    row   = wobj_rows_get(rows, i);
    in[0] = wobj_row_component(row, rows->componentCount, 0, 0.0f);
    in[1] = wobj_row_component(row, rows->componentCount, 1, 0.0f);
    in[2] = wobj_row_component(row, rows->componentCount, 2, 0.0f);
    glm_mat4_mulv3(world, in, 1.0f, out);

    wobj_w_lit(&st->w, "v ");
    wobj_w_float(&st->w, out[0]);
    wobj_w_ch(&st->w, ' ');
    wobj_w_float(&st->w, out[1]);
    wobj_w_ch(&st->w, ' ');
    wobj_w_float(&st->w, out[2]);

    if (colorRows) {
      row = wobj_rows_get(colorRows, i);
      wobj_w_ch(&st->w, ' ');
      wobj_w_float(&st->w, wobj_color_component(colorRows, row, 0, 1.0f));
      wobj_w_ch(&st->w, ' ');
      wobj_w_float(&st->w, wobj_color_component(colorRows, row, 1, 1.0f));
      wobj_w_ch(&st->w, ' ');
      wobj_w_float(&st->w, wobj_color_component(colorRows, row, 2, 1.0f));

      if (colorRows->componentCount > 3u) {
        wobj_w_ch(&st->w, ' ');
        wobj_w_float(&st->w, wobj_color_component(colorRows, row, 3, 1.0f));
      }

      wobj_w_ch(&st->w, '\n');
    } else {
      wobj_w_ch(&st->w, '\n');
    }
  }

  return true;
}

static
bool
wobj_write_texcoords(WOBJExpState * __restrict st,
                     WOBJExpRows  * __restrict rows) {
  uint32_t i;

  if (!wobj_count_add(&st->vtCount, rows->accessor->count))
    return false;

  for (i = 0; i < rows->accessor->count; i++) {
    const float *row;

    row = wobj_rows_get(rows, i);
    wobj_w_lit(&st->w, "vt ");
    wobj_w_float(&st->w, wobj_row_component(row, rows->componentCount, 0, 0.0f));
    wobj_w_ch(&st->w, ' ');
    wobj_w_float(&st->w, wobj_row_component(row, rows->componentCount, 1, 0.0f));
    if (rows->componentCount > 2u) {
      wobj_w_ch(&st->w, ' ');
      wobj_w_float(&st->w, row[2]);
    }
    wobj_w_ch(&st->w, '\n');
  }

  return true;
}

static
bool
wobj_write_normals(WOBJExpState * __restrict st,
                   WOBJExpRows  * __restrict rows,
                   mat4                       world) {
  mat4     normalMatrix;
  uint32_t i;

  if (!wobj_count_add(&st->vnCount, rows->accessor->count))
    return false;

  glm_mat4_inv((vec4 *)world, normalMatrix);
  glm_mat4_transpose(normalMatrix);

  for (i = 0; i < rows->accessor->count; i++) {
    const float *row;
    vec3         in;
    vec3         out;

    row   = wobj_rows_get(rows, i);
    in[0] = wobj_row_component(row, rows->componentCount, 0, 0.0f);
    in[1] = wobj_row_component(row, rows->componentCount, 1, 0.0f);
    in[2] = wobj_row_component(row, rows->componentCount, 2, 1.0f);
    glm_mat4_mulv3(normalMatrix, in, 0.0f, out);
    glm_vec3_normalize(out);
    wobj_write_v3(&st->w, "vn ", out);
  }

  return true;
}

static
uint32_t
wobj_input_obj_index(AkMeshPrimitive * __restrict prim,
                     AkInput         * __restrict input,
                     uint32_t                     vertexIndex,
                     uint32_t                     base,
                     uint32_t                     count) {
  uint32_t index;

  if (!input || count == 0)
    return 0;

  index = io_primitive_input_index(prim, input, vertexIndex);
  if (index >= count)
    index = 0;

  return base + index + 1u;
}

static
void
wobj_write_ref(WOBJExpState * __restrict st,
               uint32_t                  v,
               uint32_t                  vt,
               uint32_t                  vn,
               bool                      hasTexcoord,
               bool                      hasNormal) {
  wobj_w_uint(&st->w, v);
  if (hasTexcoord || hasNormal) {
    wobj_w_ch(&st->w, '/');
    if (hasTexcoord)
      wobj_w_uint(&st->w, vt);

    if (hasNormal) {
      wobj_w_ch(&st->w, '/');
      wobj_w_uint(&st->w, vn);
    }
  }
}

static
void
wobj_write_tuple(WOBJExpState    * __restrict st,
                 AkMeshPrimitive * __restrict prim,
                 AkInput         * __restrict posInput,
                 AkInput         * __restrict texInput,
                 AkInput         * __restrict normInput,
                 uint32_t                     vertexIndex,
                 uint32_t                     vBase,
                 uint32_t                     vtBase,
                 uint32_t                     vnBase,
                 uint32_t                     vCount,
                 uint32_t                     vtCount,
                 uint32_t                     vnCount) {
  uint32_t v;
  uint32_t vt;
  uint32_t vn;
  bool     hasTexcoord;
  bool     hasNormal;

  hasTexcoord = texInput && vtCount  > 0;
  hasNormal   = normInput && vnCount > 0;

  v  = wobj_input_obj_index(prim, posInput, vertexIndex, vBase, vCount);
  vt = hasTexcoord
       ? wobj_input_obj_index(prim, texInput, vertexIndex, vtBase, vtCount)
       : 0u;
  vn = hasNormal
       ? wobj_input_obj_index(prim, normInput, vertexIndex, vnBase, vnCount)
       : 0u;

  wobj_write_ref(st, v, vt, vn, hasTexcoord, hasNormal);
}

static
void
wobj_write_face_vertices(WOBJExpState    * __restrict st,
                         AkMeshPrimitive * __restrict prim,
                         AkInput         * __restrict posInput,
                         AkInput         * __restrict texInput,
                         AkInput         * __restrict normInput,
                         const uint32_t  * __restrict vertexIndices,
                         uint32_t                     vertexCount,
                         uint32_t                     vBase,
                         uint32_t                     vtBase,
                         uint32_t                     vnBase,
                         uint32_t                     vCount,
                         uint32_t                     vtCount,
                         uint32_t                     vnCount) {
  uint32_t i;

  wobj_w_lit(&st->w, "f");
  for (i = 0; i < vertexCount; i++) {
    wobj_w_ch(&st->w, ' ');
    wobj_write_tuple(st,
                     prim,
                     posInput,
                     texInput,
                     normInput,
                     vertexIndices[i],
                     vBase,
                     vtBase,
                     vnBase,
                     vCount,
                     vtCount,
                     vnCount);
  }
  wobj_w_ch(&st->w, '\n');
}

static
void
wobj_write_smooth_state(WOBJExpState    * __restrict st,
                        AkMeshPrimitive * __restrict prim) {
  bool smooth;

  smooth = ak_meshPrimitiveSmoothShading(prim);
  if (st->hasSmoothState && st->smoothState == smooth)
    return;

  st->hasSmoothState = true;
  st->smoothState    = smooth;
  wobj_w_lit(&st->w, smooth ? "s on\n" : "s off\n");
}

static
void
wobj_write_triangles(WOBJExpState    * __restrict st,
                     AkMeshPrimitive * __restrict prim,
                     AkInput         * __restrict posInput,
                     AkInput         * __restrict texInput,
                     AkInput         * __restrict normInput,
                     uint32_t                     vBase,
                     uint32_t                     vtBase,
                     uint32_t                     vnBase,
                     uint32_t                     vCount,
                     uint32_t                     vtCount,
                     uint32_t                     vnCount) {
  AkTriangleMode mode;
  uint32_t       vertexCount;
  uint32_t       i;

  vertexCount = io_primitive_vertex_count(prim);
  mode        = ((AkTriangles *)prim)->mode;

  if (mode == 0)
    mode = AK_TRIANGLES;

  wobj_write_smooth_state(st, prim);

  if (mode == AK_TRIANGLE_STRIP) {
    for (i = 0; i + 2u < vertexCount; i++) {
      uint32_t tri[3];

      if (i & 1u) {
        tri[0] = i + 1u;
        tri[1] = i;
        tri[2] = i + 2u;
      } else {
        tri[0] = i;
        tri[1] = i + 1u;
        tri[2] = i + 2u;
      }
      wobj_write_face_vertices(st, prim, posInput, texInput, normInput,
                               tri, 3u, vBase, vtBase, vnBase,
                               vCount, vtCount, vnCount);
    }
    return;
  }

  if (mode == AK_TRIANGLE_FAN) {
    for (i = 1; i + 1u < vertexCount; i++) {
      uint32_t tri[3];

      tri[0] = 0u;
      tri[1] = i;
      tri[2] = i + 1u;
      wobj_write_face_vertices(st, prim, posInput, texInput, normInput,
                               tri, 3u, vBase, vtBase, vnBase,
                               vCount, vtCount, vnCount);
    }
    return;
  }

  for (i = 0; i + 2u < vertexCount; i += 3u) {
    uint32_t tri[3];

    tri[0] = i;
    tri[1] = i + 1u;
    tri[2] = i + 2u;
    wobj_write_face_vertices(st, prim, posInput, texInput, normInput,
                             tri, 3u, vBase, vtBase, vnBase,
                             vCount, vtCount, vnCount);
  }
}

static
void
wobj_write_lines(WOBJExpState    * __restrict st,
                 AkMeshPrimitive * __restrict prim,
                 AkInput         * __restrict posInput,
                 uint32_t                     vBase,
                 uint32_t                     vCount) {
  AkLineMode mode;
  uint32_t   vertexCount;
  uint32_t   i;

  vertexCount = io_primitive_vertex_count(prim);
  mode        = ((AkLines *)prim)->mode;
  if (mode == 0)
    mode = AK_LINES;

  if (mode == AK_LINE_STRIP || mode == AK_LINE_LOOP) {
    wobj_w_lit(&st->w, "l");
    for (i = 0; i < vertexCount; i++) {
      wobj_w_ch(&st->w, ' ');
      wobj_w_uint(&st->w,
                  wobj_input_obj_index(prim, posInput, i, vBase, vCount));
    }
    if (mode == AK_LINE_LOOP && vertexCount > 0) {
      wobj_w_ch(&st->w, ' ');
      wobj_w_uint(&st->w,
                  wobj_input_obj_index(prim, posInput, 0u, vBase, vCount));
    }
    wobj_w_ch(&st->w, '\n');
    return;
  }

  for (i = 0; i + 1u < vertexCount; i += 2u) {
    wobj_w_lit(&st->w, "l ");
    wobj_w_uint(&st->w,
                wobj_input_obj_index(prim, posInput, i, vBase, vCount));
    wobj_w_ch(&st->w, ' ');
    wobj_w_uint(&st->w,
                wobj_input_obj_index(prim, posInput, i + 1u, vBase, vCount));
    wobj_w_ch(&st->w, '\n');
  }
}

static
void
wobj_write_points(WOBJExpState    * __restrict st,
                  AkMeshPrimitive * __restrict prim,
                  AkInput         * __restrict posInput,
                  uint32_t                     vBase,
                  uint32_t                     vCount) {
  uint32_t vertexCount;
  uint32_t i;

  vertexCount = io_primitive_vertex_count(prim);
  if (vertexCount == 0)
    return;

  wobj_w_lit(&st->w, "p");
  for (i = 0; i < vertexCount; i++) {
    wobj_w_ch(&st->w, ' ');
    wobj_w_uint(&st->w,
                wobj_input_obj_index(prim, posInput, i, vBase, vCount));
  }
  wobj_w_ch(&st->w, '\n');
}

static
void
wobj_write_polygons(WOBJExpState    * __restrict st,
                    AkMeshPrimitive * __restrict prim,
                    AkInput         * __restrict posInput,
                    AkInput         * __restrict texInput,
                    AkInput         * __restrict normInput,
                    uint32_t                     vBase,
                    uint32_t                     vtBase,
                    uint32_t                     vnBase,
                    uint32_t                     vCount,
                    uint32_t                     vtCount,
                    uint32_t                     vnCount) {
  AkPolygon *poly;
  size_t     cursor;
  size_t     i;

  poly = (AkPolygon *)prim;
  if (!poly->vcount || poly->vcount->count == 0)
    return;

  wobj_write_smooth_state(st, prim);

  cursor = 0;
  for (i = 0; i < poly->vcount->count; i++) {
    uint32_t local[64];
    uint32_t vc;
    uint32_t j;

    vc = poly->vcount->items[i];
    if (vc < 3u) {
      cursor += vc;
      continue;
    }

    if (vc <= AK_ARRAY_LEN(local)) {
      for (j = 0; j < vc; j++)
        local[j] = (uint32_t)(cursor + j);
      wobj_write_face_vertices(st, prim, posInput, texInput, normInput,
                               local, vc, vBase, vtBase, vnBase,
                               vCount, vtCount, vnCount);
    } else {
      uint32_t *heapLocal;

      heapLocal = malloc(sizeof(*heapLocal) * vc);
      if (!heapLocal) {
        st->w.result = AK_ERR;
        return;
      }
      for (j = 0; j < vc; j++)
        heapLocal[j] = (uint32_t)(cursor + j);
      wobj_write_face_vertices(st, prim, posInput, texInput, normInput,
                               heapLocal, vc, vBase, vtBase, vnBase,
                               vCount, vtCount, vnCount);
      free(heapLocal);
    }

    cursor += vc;
  }
}

static
bool
wobj_write_primitive_material(WOBJExpState      * __restrict st,
                              AkMeshPrimitive   * __restrict prim,
                              AkInstanceGeometry * __restrict inst) {
  AkResolvedMaterial resolved;
  uint32_t           matIdx;

  if (st->materialCount == 0)
    return true;

  memset(&resolved, 0, sizeof(resolved));
  if (!ak_materialResolve(prim, inst, UINT32_MAX, &resolved))
    return true;

  matIdx = wobj_material_index(st, resolved.material);
  if (matIdx == UINT32_MAX)
    return true;

  return wobj_use_material(st, matIdx);
}

static
bool
wobj_write_primitive(WOBJExpState      * __restrict st,
                     AkMeshPrimitive   * __restrict prim,
                     AkInstanceGeometry * __restrict inst,
                     mat4                            world) {
  AkInput      *posInput;
  AkInput      *colorInput;
  AkInput      *texInput;
  AkInput      *normInput;
  WOBJExpRows   posRows;
  WOBJExpRows   colorRows;
  WOBJExpRows   texRows;
  WOBJExpRows   normRows;
  uint32_t      vBase;
  uint32_t      vtBase;
  uint32_t      vnBase;
  bool          hasTexcoord;
  bool          hasNormal;
  bool          ok;

  posInput = wobj_find_input(prim, AK_INPUT_POSITION);
  if (!posInput || !posInput->accessor)
    return true;

  colorInput = wobj_find_vertex_color_input(prim, posInput);
  texInput   = wobj_find_texcoord_input(prim);
  normInput  = wobj_find_input(prim, AK_INPUT_NORMAL);

  memset(&colorRows, 0, sizeof(colorRows));
  memset(&texRows, 0, sizeof(texRows));
  memset(&normRows, 0, sizeof(normRows));

  if (!wobj_rows_init(&posRows, posInput->accessor))
    return false;

  hasTexcoord = texInput
                && texInput->accessor
                && wobj_rows_init(&texRows, texInput->accessor);
  hasNormal   = normInput
                && normInput->accessor
                && wobj_rows_init(&normRows, normInput->accessor);

  vBase  = st->vCount;
  vtBase = st->vtCount;
  vnBase = st->vnCount;

  if (colorInput && !wobj_rows_init(&colorRows, colorInput->accessor))
    colorInput = NULL;

  ok = wobj_write_positions(st, &posRows, colorInput ? &colorRows : NULL, world);
  if (ok && hasTexcoord)
    ok = wobj_write_texcoords(st, &texRows);
  if (ok && hasNormal)
    ok = wobj_write_normals(st, &normRows, world);
  if (ok) {
    ok = wobj_write_primitive_material(st, prim, inst);
  }
  if (ok) {
    switch (prim->type) {
      case AK_PRIMITIVE_TRIANGLES:
        wobj_write_triangles(st, prim, posInput,
                             hasTexcoord ? texInput : NULL,
                             hasNormal ? normInput : NULL,
                             vBase, vtBase, vnBase,
                             posRows.accessor->count,
                             hasTexcoord ? texRows.accessor->count : 0u,
                             hasNormal ? normRows.accessor->count : 0u);
        break;
      case AK_PRIMITIVE_POLYGONS:
        wobj_write_polygons(st, prim, posInput,
                            hasTexcoord ? texInput : NULL,
                            hasNormal ? normInput : NULL,
                            vBase, vtBase, vnBase,
                            posRows.accessor->count,
                            hasTexcoord ? texRows.accessor->count : 0u,
                            hasNormal ? normRows.accessor->count : 0u);
        break;
      case AK_PRIMITIVE_LINES:
        wobj_write_lines(st, prim, posInput, vBase, posRows.accessor->count);
        break;
      case AK_PRIMITIVE_POINTS:
        wobj_write_points(st, prim, posInput, vBase, posRows.accessor->count);
        break;
      default:
        break;
    }
  }

  wobj_rows_destroy(&normRows);
  wobj_rows_destroy(&texRows);
  wobj_rows_destroy(&colorRows);
  wobj_rows_destroy(&posRows);

  return ok && st->w.result == AK_OK;
}

static
void
wobj_write_object_name(WOBJExpState      * __restrict st,
                       AkNode            * __restrict node,
                       AkInstanceGeometry * __restrict inst,
                       AkGeometry        * __restrict geom,
                       AkMesh            * __restrict mesh) {
  const char *name;

  name = NULL;
  if (inst && inst->base.name)
    name = inst->base.name;
  else if (node && node->name)
    name = node->name;
  else if (geom && geom->name)
    name = geom->name;
  else if (mesh && mesh->name)
    name = mesh->name;

  wobj_w_lit(&st->w, "o ");
  if (name && *name) {
    wobj_w_name(&st->w, name);
  } else {
    wobj_w_lit(&st->w, "object_");
    wobj_w_uint(&st->w, st->objectCount);
  }
  wobj_w_ch(&st->w, '\n');
  st->objectCount++;
}

AK_HIDE
bool
wobj_write_mesh_instance(WOBJExpState      * __restrict st,
                         AkNode            * __restrict node,
                         AkInstanceGeometry * __restrict inst,
                         AkGeometry        * __restrict geom,
                         mat4                            world) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;

  if (!st || !geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return true;

  mesh = ak_objGet(geom->gdata);
  if (!mesh || !mesh->primitive)
    return true;

  wobj_write_object_name(st, node, inst, geom, mesh);
  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (!wobj_write_primitive(st, prim, inst, world))
      return false;
  }

  return st->w.result == AK_OK;
}
