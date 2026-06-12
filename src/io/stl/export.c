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

#include "stl.h"
#include "../common/primitive.h"
#include "../common/text_number.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STL_EXP_FILE_BUFFER_SIZE   (1024u * 1024u)
#define STL_EXP_HEADER_SIZE        80u
#define STL_EXP_TRIANGLE_SIZE      50u
#define STL_EXP_MAX_NODE_DEPTH     512u

typedef struct STLExpRows {
  AkAccessor *accessor;
  float      *scratch;
  uint32_t    componentCount;
  bool        direct;
} STLExpRows;

typedef struct STLExpWriter {
  FILE          *file;
  size_t         len;
  uint32_t       triangleCount;
  AkResult       result;
  bool           ascii;
  unsigned char  buffer[64u * 1024u];
} STLExpWriter;

typedef struct STLExpState {
  AkDoc        *doc;
  STLExpWriter w;
  uint32_t      objectCount;
} STLExpState;

static
void
stl_w_flush(STLExpWriter * __restrict w) {
  if (w->len == 0)
    return;

  if (w->result == AK_OK
      && fwrite(w->buffer, 1, w->len, w->file) != w->len)
    w->result = AK_ERR;

  w->len = 0;
}

static
void
stl_w_raw(STLExpWriter * __restrict w,
          const void   * __restrict data,
          size_t                    len) {
  const unsigned char *src;

  src = data;
  while (len > 0) {
    size_t avail;
    size_t n;

    avail = sizeof(w->buffer) - w->len;
    if (avail == 0) {
      stl_w_flush(w);
      avail = sizeof(w->buffer);
    }

    if (w->len == 0 && len >= sizeof(w->buffer)) {
      if (w->result == AK_OK && fwrite(src, 1, len, w->file) != len)
        w->result = AK_ERR;
      return;
    }

    n = len < avail ? len : avail;
    memcpy(w->buffer + w->len, src, n);
    w->len += n;
    src    += n;
    len    -= n;
  }
}

static
void
stl_w_ch(STLExpWriter * __restrict w, char ch) {
  if (w->len == sizeof(w->buffer))
    stl_w_flush(w);

  w->buffer[w->len++] = (unsigned char)ch;
}

static
void
stl_w_lit(STLExpWriter * __restrict w, const char * __restrict lit) {
  stl_w_raw(w, lit, strlen(lit));
}

static
void
stl_w_float(STLExpWriter * __restrict w, float val) {
  char   buf[48];
  int    len;
  size_t outLen;

  if (!isfinite(val)) {
    w->result = AK_ERR;
    return;
  }
  if (ak_io_text_format_fixed_float(buf, sizeof(buf), val, 6u, &outLen)) {
    stl_w_raw(w, buf, outLen);
    return;
  }

  len = snprintf(buf, sizeof(buf), "%.6g", (double)val);
  if (len <= 0 || (size_t)len >= sizeof(buf)) {
    w->result = AK_ERR;
    return;
  }

  outLen = (size_t)len;
  if (!ak_io_text_normalize_number(buf, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  stl_w_raw(w, buf, outLen);
}

static
void
stl_write_u16le(STLExpWriter * __restrict w, uint16_t value) {
  unsigned char out[2];

  out[0] = (unsigned char)(value & 0xffu);
  out[1] = (unsigned char)((value >> 8u) & 0xffu);
  stl_w_raw(w, out, sizeof(out));
}

static
void
stl_write_u32le(STLExpWriter * __restrict w, uint32_t value) {
  unsigned char out[4];

  out[0] = (unsigned char)(value & 0xffu);
  out[1] = (unsigned char)((value >> 8u) & 0xffu);
  out[2] = (unsigned char)((value >> 16u) & 0xffu);
  out[3] = (unsigned char)((value >> 24u) & 0xffu);
  stl_w_raw(w, out, sizeof(out));
}

static
void
stl_write_f32le(STLExpWriter * __restrict w, float value) {
  uint32_t bits;

  if (!isfinite(value)) {
    w->result = AK_ERR;
    return;
  }

  memcpy(&bits, &value, sizeof(bits));
  stl_write_u32le(w, bits);
}

static
bool
stl_rows_init(STLExpRows * __restrict rows,
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
stl_rows_destroy(STLExpRows * __restrict rows) {
  free(rows->scratch);
  rows->scratch = NULL;
}

static
const float*
stl_rows_get(STLExpRows * __restrict rows, uint32_t index) {
  if (index >= rows->accessor->count)
    index = 0;

  return rows->direct
         ? io_accessor_float_row(rows->accessor, index)
         : rows->scratch + (size_t)index * rows->componentCount;
}

static
float
stl_row_component(const float * __restrict row,
                  uint32_t                 componentCount,
                  uint32_t                 component,
                  float                    fallback) {
  return component < componentCount ? row[component] : fallback;
}

static
AkInput*
stl_find_position_input(AkMeshPrimitive * __restrict prim) {
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
stl_triangle_normal(vec3 a, vec3 b, vec3 c, vec3 normal) {
  vec3 ab;
  vec3 ac;
  float len;

  glm_vec3_sub(b, a, ab);
  glm_vec3_sub(c, a, ac);
  glm_vec3_cross(ab, ac, normal);
  len = glm_vec3_norm(normal);
  if (len > 0.0f && isfinite(len)) {
    glm_vec3_scale(normal, 1.0f / len, normal);
  } else {
    glm_vec3_zero(normal);
  }
}

static
bool
stl_writer_count_triangle(STLExpWriter * __restrict w) {
  if (w->triangleCount == UINT32_MAX) {
    w->result = AK_ERR;
    return false;
  }
  w->triangleCount++;
  return true;
}

static
void
stl_write_ascii_triangle(STLExpWriter * __restrict w,
                         vec3                      normal,
                         vec3                      a,
                         vec3                      b,
                         vec3                      c) {
  stl_w_lit(w, "facet normal ");
  stl_w_float(w, normal[0]);
  stl_w_ch(w, ' ');
  stl_w_float(w, normal[1]);
  stl_w_ch(w, ' ');
  stl_w_float(w, normal[2]);
  stl_w_lit(w, "\n outer loop\n  vertex ");
  stl_w_float(w, a[0]);
  stl_w_ch(w, ' ');
  stl_w_float(w, a[1]);
  stl_w_ch(w, ' ');
  stl_w_float(w, a[2]);
  stl_w_lit(w, "\n  vertex ");
  stl_w_float(w, b[0]);
  stl_w_ch(w, ' ');
  stl_w_float(w, b[1]);
  stl_w_ch(w, ' ');
  stl_w_float(w, b[2]);
  stl_w_lit(w, "\n  vertex ");
  stl_w_float(w, c[0]);
  stl_w_ch(w, ' ');
  stl_w_float(w, c[1]);
  stl_w_ch(w, ' ');
  stl_w_float(w, c[2]);
  stl_w_lit(w, "\n endloop\nendfacet\n");
}

static
void
stl_write_binary_triangle(STLExpWriter * __restrict w,
                          vec3                      normal,
                          vec3                      a,
                          vec3                      b,
                          vec3                      c) {
  uint32_t i;

  for (i = 0; i < 3u; i++)
    stl_write_f32le(w, normal[i]);
  for (i = 0; i < 3u; i++)
    stl_write_f32le(w, a[i]);
  for (i = 0; i < 3u; i++)
    stl_write_f32le(w, b[i]);
  for (i = 0; i < 3u; i++)
    stl_write_f32le(w, c[i]);
  stl_write_u16le(w, 0u);
}

static
void
stl_write_triangle(STLExpWriter * __restrict w,
                   vec3                      a,
                   vec3                      b,
                   vec3                      c) {
  vec3 normal;

  if (!stl_writer_count_triangle(w))
    return;

  stl_triangle_normal(a, b, c, normal);
  if (w->ascii)
    stl_write_ascii_triangle(w, normal, a, b, c);
  else
    stl_write_binary_triangle(w, normal, a, b, c);
}

static
void
stl_vertex_position(STLExpRows      * __restrict rows,
                    AkMeshPrimitive * __restrict prim,
                    AkInput         * __restrict posInput,
                    uint32_t                     vertexIndex,
                    mat4                         world,
                    vec3                         out) {
  const float *row;
  uint32_t     posIndex;
  vec3         in;

  posIndex = io_primitive_input_index(prim, posInput, vertexIndex);
  row      = stl_rows_get(rows, posIndex);

  in[0] = stl_row_component(row, rows->componentCount, 0u, 0.0f);
  in[1] = stl_row_component(row, rows->componentCount, 1u, 0.0f);
  in[2] = stl_row_component(row, rows->componentCount, 2u, 0.0f);
  glm_mat4_mulv3(world, in, 1.0f, out);
}

static
void
stl_write_triangle_indices(STLExpState    * __restrict st,
                           STLExpRows     * __restrict rows,
                           AkMeshPrimitive * __restrict prim,
                           AkInput         * __restrict posInput,
                           uint32_t                     i0,
                           uint32_t                     i1,
                           uint32_t                     i2,
                           mat4                         world,
                           bool                         mirrored) {
  vec3 a;
  vec3 b;
  vec3 c;

  if (mirrored) {
    uint32_t tmp;

    tmp = i0;
    i0  = i2;
    i2  = tmp;
  }

  stl_vertex_position(rows, prim, posInput, i0, world, a);
  stl_vertex_position(rows, prim, posInput, i1, world, b);
  stl_vertex_position(rows, prim, posInput, i2, world, c);
  stl_write_triangle(&st->w, a, b, c);
}

static
void
stl_write_triangles_primitive(STLExpState    * __restrict st,
                              STLExpRows     * __restrict rows,
                              AkMeshPrimitive * __restrict prim,
                              AkInput         * __restrict posInput,
                              mat4                         world,
                              bool                         mirrored) {
  AkTriangleMode mode;
  uint32_t       vertexCount;
  uint32_t       i;

  vertexCount = io_primitive_vertex_count(prim);
  mode        = ((AkTriangles *)prim)->mode;
  if (mode == 0)
    mode = AK_TRIANGLES;

  if (mode == AK_TRIANGLE_STRIP) {
    for (i = 0; i + 2u < vertexCount; i++) {
      if (i & 1u) {
        stl_write_triangle_indices(st, rows, prim, posInput,
                                   i + 1u, i, i + 2u, world, mirrored);
      } else {
        stl_write_triangle_indices(st, rows, prim, posInput,
                                   i, i + 1u, i + 2u, world, mirrored);
      }
    }
    return;
  }

  if (mode == AK_TRIANGLE_FAN) {
    for (i = 1; i + 1u < vertexCount; i++) {
      stl_write_triangle_indices(st, rows, prim, posInput,
                                 0u, i, i + 1u, world, mirrored);
    }
    return;
  }

  for (i = 0; i + 2u < vertexCount; i += 3u) {
    stl_write_triangle_indices(st, rows, prim, posInput,
                               i, i + 1u, i + 2u, world, mirrored);
  }
}

static
void
stl_write_polygon_primitive(STLExpState    * __restrict st,
                            STLExpRows     * __restrict rows,
                            AkMeshPrimitive * __restrict prim,
                            AkInput         * __restrict posInput,
                            mat4                         world,
                            bool                         mirrored) {
  AkPolygon *poly;
  size_t     cursor;
  size_t     i;

  poly = (AkPolygon *)prim;
  if (!poly->vcount || poly->vcount->count == 0)
    return;

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
      stl_write_triangle_indices(st, rows, prim, posInput,
                                 (uint32_t)cursor,
                                 (uint32_t)(cursor + j),
                                 (uint32_t)(cursor + j + 1u),
                                 world,
                                 mirrored);
    }
    cursor += vc;
  }
}

static
bool
stl_write_primitive(STLExpState    * __restrict st,
                    AkMeshPrimitive * __restrict prim,
                    mat4                         world,
                    bool                         mirrored) {
  AkInput    *posInput;
  STLExpRows  posRows;
  bool        ok;

  if (!prim)
    return true;
  if (prim->type != AK_PRIMITIVE_TRIANGLES
      && prim->type != AK_PRIMITIVE_POLYGONS)
    return true;

  posInput = stl_find_position_input(prim);
  if (!posInput || !posInput->accessor)
    return true;

  if (!stl_rows_init(&posRows, posInput->accessor))
    return false;

  switch (prim->type) {
    case AK_PRIMITIVE_TRIANGLES:
      stl_write_triangles_primitive(st, &posRows, prim, posInput, world, mirrored);
      break;
    case AK_PRIMITIVE_POLYGONS:
      stl_write_polygon_primitive(st, &posRows, prim, posInput, world, mirrored);
      break;
    default:
      break;
  }

  ok = st->w.result == AK_OK;
  stl_rows_destroy(&posRows);
  return ok;
}

static
bool
stl_write_mesh_instance(STLExpState       * __restrict st,
                        AkGeometry        * __restrict geom,
                        mat4                            world) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  bool             mirrored;

  if (!st || !geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return true;

  mesh = ak_objGet(geom->gdata);
  if (!mesh || !mesh->primitive)
    return true;

  mirrored = glm_mat4_det(world) < 0.0f;
  st->objectCount++;

  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (!stl_write_primitive(st, prim, world, mirrored))
      return false;
  }

  return st->w.result == AK_OK;
}

static
AkGeometry*
stl_instance_geometry(AkInstanceGeometry * __restrict inst) {
  void *obj;

  if (!inst)
    return NULL;

  obj = ak_instanceObject(&inst->base);
  return obj;
}

static
bool
stl_write_node(STLExpState * __restrict st,
               AkNode      * __restrict node,
               mat4                      parentWorld,
               uint32_t                  depth) {
  AkInstanceBase *base;
  AkNode         *child;
  AkInstanceNode *nodeRef;
  AkMatrix        localMatrix;
  mat4            world;

  if (!node)                           return true;
  if (depth > STL_EXP_MAX_NODE_DEPTH)  return false;

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
    geom = stl_instance_geometry(inst);

    if (!stl_write_mesh_instance(st, geom, world))
      return false;
  }

  for (child = node->chld; child; child = child->next) {
    if (!stl_write_node(st, child, world, depth + 1u))
      return false;
  }

  for (nodeRef = node->node; nodeRef; nodeRef = nodeRef->next) {
    AkNode *target;

    target = ak_instanceNodeTarget(nodeRef);
    if (target && !stl_write_node(st, target, world, depth + 1u))
      return false;
  }

  return true;
}

static
bool
stl_write_scene(STLExpState * __restrict st) {
  mat4 identity;

  glm_mat4_identity(identity);
  if (st->doc->scene && st->doc->scene->node)
    return stl_write_node(st, st->doc->scene->node, identity, 0u);

  return true;
}

static
bool
stl_write_library_fallback(STLExpState * __restrict st) {
  AkGeometry *geom;
  mat4        identity;

  if (st->objectCount > 0)
    return true;

  glm_mat4_identity(identity);
  for (geom = st->doc->lib.geometries.first; geom; geom = geom->next) {
    if (!stl_write_mesh_instance(st, geom, identity))
      return false;
  }

  return true;
}

static
bool
stl_writer_begin(STLExpWriter * __restrict w,
                 FILE         * __restrict file,
                 bool                       ascii) {
  unsigned char header[STL_EXP_HEADER_SIZE];

  memset(w, 0, sizeof(*w));
  w->file   = file;
  w->result = AK_OK;
  w->ascii  = ascii;

  if (ascii) {
    stl_w_lit(w, "solid assetkit\n");
    return true;
  }

  memset(header, 0, sizeof(header));
  memcpy(header, "Generated by AssetKit", 21u);
  stl_w_raw(w, header, sizeof(header));
  stl_write_u32le(w, 0u);
  return w->result == AK_OK;
}

static
bool
stl_writer_end(STLExpWriter * __restrict w) {
  long pos;

  if (w->ascii) {
    stl_w_lit(w, "endsolid assetkit\n");
    stl_w_flush(w);
    return w->result == AK_OK;
  }

  stl_w_flush(w);
  if (w->result != AK_OK)
    return false;

  pos = ftell(w->file);
  if (pos < 0)
    return false;

  if (fseek(w->file, (long)STL_EXP_HEADER_SIZE, SEEK_SET) != 0)
    return false;
  stl_write_u32le(w, w->triangleCount);
  stl_w_flush(w);
  if (w->result != AK_OK)
    return false;

  return fseek(w->file, pos, SEEK_SET) == 0;
}

AK_HIDE
AkResult
stl_export(AkDoc * __restrict doc, const char * __restrict filepath) {
  STLExpState st;
  FILE       *file;
  AkResult    result;
  uintptr_t   format;

  if (!doc || !filepath)
    return AK_ERR;

  memset(&st, 0, sizeof(st));
  st.doc = doc;

  format = ak_opt_get(AK_OPT_STL_EXPORT_FORMAT);

  file = fopen(filepath, "wb");
  if (!file)
    return AK_EBADF;
  (void)setvbuf(file, NULL, _IOFBF, STL_EXP_FILE_BUFFER_SIZE);

  result = AK_OK;
  if (!stl_writer_begin(&st.w, file, format == AK_STL_EXPORT_ASCII)
      || !stl_write_scene(&st)
      || !stl_write_library_fallback(&st)
      || !stl_writer_end(&st.w))
    result = AK_ERR;

  if (fclose(file) != 0 && result == AK_OK)
    result = AK_ERR;

  if (result != AK_OK)
    remove(filepath);

  return result;
}
