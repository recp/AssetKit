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
  AkDoc       *doc;
  AK3MFBuffer  resources;
  AK3MFBuffer  build;
  uint32_t     objectCount;
  uint32_t     nextObjectId;
  AkResult     result;
} AK3MFExportState;

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
void
ak_3mf_append_vertex(AK3MFBuffer * __restrict vertices, vec3 pos) {
  ak_3mf_buf_lit(vertices, "          <vertex x=\"");
  ak_3mf_buf_float(vertices, pos[0]);
  ak_3mf_buf_lit(vertices, "\" y=\"");
  ak_3mf_buf_float(vertices, pos[1]);
  ak_3mf_buf_lit(vertices, "\" z=\"");
  ak_3mf_buf_float(vertices, pos[2]);
  ak_3mf_buf_lit(vertices, "\"/>\n");
}

static
void
ak_3mf_append_triangle(AK3MFBuffer * __restrict triangles,
                       uint32_t                 i0,
                       uint32_t                 i1,
                       uint32_t                 i2) {
  ak_3mf_buf_lit(triangles, "          <triangle v1=\"");
  ak_3mf_buf_u32(triangles, i0);
  ak_3mf_buf_lit(triangles, "\" v2=\"");
  ak_3mf_buf_u32(triangles, i1);
  ak_3mf_buf_lit(triangles, "\" v3=\"");
  ak_3mf_buf_u32(triangles, i2);
  ak_3mf_buf_lit(triangles, "\"/>\n");
}

static
bool
ak_3mf_emit_triangle(AK3MFRows       * __restrict rows,
                     AkMeshPrimitive * __restrict prim,
                     AkInput         * __restrict posInput,
                     uint32_t                     i0,
                     uint32_t                     i1,
                     uint32_t                     i2,
                     AK3MFBuffer     * __restrict vertices,
                     AK3MFBuffer     * __restrict triangles,
                     uint32_t        * __restrict vertexCount,
                     uint32_t        * __restrict triangleCount) {
  vec3 a;
  vec3 b;
  vec3 c;

  if (*vertexCount > UINT32_MAX - 3u)
    return false;

  ak_3mf_vertex_position(rows, prim, posInput, i0, a);
  ak_3mf_vertex_position(rows, prim, posInput, i1, b);
  ak_3mf_vertex_position(rows, prim, posInput, i2, c);

  ak_3mf_append_vertex(vertices, a);
  ak_3mf_append_vertex(vertices, b);
  ak_3mf_append_vertex(vertices, c);
  ak_3mf_append_triangle(triangles,
                         *vertexCount,
                         *vertexCount + 1u,
                         *vertexCount + 2u);

  *vertexCount += 3u;
  (*triangleCount)++;
  return vertices->result == AK_OK && triangles->result == AK_OK;
}

static
bool
ak_3mf_emit_triangles_primitive(AK3MFRows       * __restrict rows,
                                AkMeshPrimitive * __restrict prim,
                                AkInput         * __restrict posInput,
                                AK3MFBuffer     * __restrict vertices,
                                AK3MFBuffer     * __restrict triangles,
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
        if (!ak_3mf_emit_triangle(rows, prim, posInput,
                                  i + 1u, i, i + 2u,
                                  vertices, triangles,
                                  vertexCount, triangleCount))
          return false;
      } else {
        if (!ak_3mf_emit_triangle(rows, prim, posInput,
                                  i, i + 1u, i + 2u,
                                  vertices, triangles,
                                  vertexCount, triangleCount))
          return false;
      }
    }
    return true;
  }

  if (mode == AK_TRIANGLE_FAN) {
    for (i = 1u; i + 1u < count; i++) {
      if (!ak_3mf_emit_triangle(rows, prim, posInput,
                                0u, i, i + 1u,
                                vertices, triangles,
                                vertexCount, triangleCount))
        return false;
    }
    return true;
  }

  for (i = 0; i + 2u < count; i += 3u) {
    if (!ak_3mf_emit_triangle(rows, prim, posInput,
                              i, i + 1u, i + 2u,
                              vertices, triangles,
                              vertexCount, triangleCount))
      return false;
  }

  return true;
}

static
bool
ak_3mf_emit_polygon_primitive(AK3MFRows       * __restrict rows,
                              AkMeshPrimitive * __restrict prim,
                              AkInput         * __restrict posInput,
                              AK3MFBuffer     * __restrict vertices,
                              AK3MFBuffer     * __restrict triangles,
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
      if (!ak_3mf_emit_triangle(rows, prim, posInput,
                                (uint32_t)cursor,
                                (uint32_t)(cursor + j),
                                (uint32_t)(cursor + j + 1u),
                                vertices, triangles,
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
  AK3MFBuffer triangles;
  AK3MFRows   rows;
  AkInput    *posInput;
  uint32_t    vertexCount;
  uint32_t    triangleCount;
  uint32_t    objectId;
  bool        ok;

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

  memset(&vertices, 0, sizeof(vertices));
  memset(&triangles, 0, sizeof(triangles));
  vertices.result  = AK_OK;
  triangles.result = AK_OK;
  vertexCount      = 0;
  triangleCount    = 0;

  if (prim->type == AK_PRIMITIVE_TRIANGLES) {
    ok = ak_3mf_emit_triangles_primitive(&rows,
                                         prim,
                                         posInput,
                                         &vertices,
                                         &triangles,
                                         &vertexCount,
                                         &triangleCount);
  } else {
    ok = ak_3mf_emit_polygon_primitive(&rows,
                                       prim,
                                       posInput,
                                       &vertices,
                                       &triangles,
                                       &vertexCount,
                                       &triangleCount);
  }

  ak_3mf_rows_destroy(&rows);
  if (!ok || vertices.result != AK_OK || triangles.result != AK_OK) {
    ak_3mf_buf_free(&vertices);
    ak_3mf_buf_free(&triangles);
    return false;
  }

  if (triangleCount == 0) {
    ak_3mf_buf_free(&vertices);
    ak_3mf_buf_free(&triangles);
    return true;
  }

  objectId = st->nextObjectId++;

  ak_3mf_buf_lit(&st->resources, "      <object id=\"");
  ak_3mf_buf_u32(&st->resources, objectId);
  ak_3mf_buf_lit(&st->resources, "\" type=\"model\">\n"
                                "        <mesh>\n"
                                "          <vertices>\n");
  ak_3mf_buf_raw(&st->resources, vertices.data, vertices.len);
  ak_3mf_buf_lit(&st->resources, "          </vertices>\n"
                                "          <triangles>\n");
  ak_3mf_buf_raw(&st->resources, triangles.data, triangles.len);
  ak_3mf_buf_lit(&st->resources, "          </triangles>\n"
                                "        </mesh>\n"
                                "      </object>\n");

  ak_3mf_buf_lit(&st->build, "      <item objectid=\"");
  ak_3mf_buf_u32(&st->build, objectId);
  ak_3mf_buf_lit(&st->build, "\" transform=\"");
  ak_3mf_append_transform(&st->build, world);
  ak_3mf_buf_lit(&st->build, "\"/>\n");

  st->objectCount++;

  ak_3mf_buf_free(&vertices);
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

  if (st->objectCount > 0)
    return true;

  glm_mat4_identity(identity);
  for (geom = st->doc->lib.geometries.first; geom; geom = geom->next) {
    if (!ak_3mf_write_mesh_instance(st, geom, identity))
      return false;
  }

  return true;
}

static
bool
ak_3mf_build_model_xml(AK3MFExportState * __restrict st,
                       AK3MFBuffer      * __restrict model) {
  memset(model, 0, sizeof(*model));
  model->result = AK_OK;

  ak_3mf_buf_lit(model, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<model unit=\"millimeter\" xml:lang=\"en-US\" "
                        "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">\n"
                        "  <resources>\n");
  ak_3mf_buf_raw(model, st->resources.data, st->resources.len);
  ak_3mf_buf_lit(model, "  </resources>\n"
                        "  <build>\n");
  ak_3mf_buf_raw(model, st->build.data, st->build.len);
  ak_3mf_buf_lit(model, "  </build>\n"
                        "</model>\n");

  return model->result == AK_OK;
}

AK_HIDE
AkResult
ak_3mf_export(AkDoc * __restrict doc, const char * __restrict filepath) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
    "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
    "  <Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n"
    "</Types>\n";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
    "  <Relationship Target=\"/3D/3dmodel.model\" Id=\"rel0\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>\n"
    "</Relationships>\n";
  AK3MFExportState st;
  AK3MFBuffer      model;
  AkZipWriteEntry  entries[3];
  AkResult         result;

  if (!doc || !filepath)
    return AK_ERR;

  memset(&st, 0, sizeof(st));
  st.doc              = doc;
  st.result           = AK_OK;
  st.resources.result = AK_OK;
  st.build.result     = AK_OK;
  st.nextObjectId     = 1u;

  if (!ak_3mf_write_scene(&st)
      || !ak_3mf_write_library_fallback(&st)
      || st.resources.result != AK_OK
      || st.build.result != AK_OK
      || !ak_3mf_build_model_xml(&st, &model)) {
    ak_3mf_buf_free(&st.resources);
    ak_3mf_buf_free(&st.build);
    return AK_ERR;
  }

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = model.data;
  entries[2].size = model.len;

  result = ak_zip_write_stored(filepath, entries, 3u);

  ak_3mf_buf_free(&model);
  ak_3mf_buf_free(&st.resources);
  ak_3mf_buf_free(&st.build);

  return result;
}
