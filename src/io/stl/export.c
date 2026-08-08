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
#include "../common/binary.h"
#include "../common/export_space.h"
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

typedef IOFloatRows STLExpRows;

#define stl_rows_init    io_float_rows_init
#define stl_rows_destroy io_float_rows_destroy
#define stl_rows_get     io_float_rows_get
#define stl_w_lit(W, LIT) stl_w_raw((W), (LIT), sizeof(LIT) - 1u)
#define stl_row_component io_float_row_component

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
stl_w_raw_small(STLExpWriter * __restrict w,
                const void   * __restrict data,
                size_t                    len) {
  if (sizeof(w->buffer) - w->len < len)
    stl_w_flush(w);

  memcpy(w->buffer + w->len, data, len);
  w->len += len;
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
stl_w_float(STLExpWriter * __restrict w, float val) {
  char  *dst;
  size_t avail;
  size_t outLen;

  avail = sizeof(w->buffer) - w->len;
  if (avail < 48u) {
    stl_w_flush(w);
    avail = sizeof(w->buffer) - w->len;
  }

  dst = (char *)w->buffer + w->len;
  if (!ak_io_text_format_float6(dst, avail, val, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  w->len += outLen;
}

static
bool
stl_vec3_finite(vec3 v) {
  return isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
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
  unsigned char out[STL_EXP_TRIANGLE_SIZE];

  if (!stl_vec3_finite(normal)
      || !stl_vec3_finite(a)
      || !stl_vec3_finite(b)
      || !stl_vec3_finite(c)) {
    w->result = AK_ERR;
    return;
  }

  io_store_f32le(out + 0,  normal[0]);
  io_store_f32le(out + 4,  normal[1]);
  io_store_f32le(out + 8,  normal[2]);
  io_store_f32le(out + 12, a[0]);
  io_store_f32le(out + 16, a[1]);
  io_store_f32le(out + 20, a[2]);
  io_store_f32le(out + 24, b[0]);
  io_store_f32le(out + 28, b[1]);
  io_store_f32le(out + 32, b[2]);
  io_store_f32le(out + 36, c[0]);
  io_store_f32le(out + 40, c[1]);
  io_store_f32le(out + 44, c[2]);
  out[48] = 0;
  out[49] = 0;
  stl_w_raw_small(w, out, sizeof(out));
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

  io_triangle_normal(a, b, c, normal);
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
  IOTriangleIter iter;
  uint32_t       tri[3];

  if (!io_triangle_iter_init(&iter, prim))
    return;

  while (io_triangle_iter_next(&iter, tri)) {
    stl_write_triangle_indices(st, rows, prim, posInput,
                               tri[0], tri[1], tri[2], world, mirrored);
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

  posInput = io_primitive_find_input(prim, AK_INPUT_POSITION);
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
  mat4 root;

  io_export_canonical_root(st->doc, root);
  if (st->doc->scene && st->doc->scene->node)
    return stl_write_node(st, st->doc->scene->node, root, 0u);

  return true;
}

static
bool
stl_write_library_fallback(STLExpState * __restrict st) {
  AkGeometry *geom;
  mat4        root;

  if (st->objectCount > 0)
    return true;

  io_export_canonical_root(st->doc, root);
  for (geom = st->doc->lib.geometries.first; geom; geom = geom->next) {
    if (!stl_write_mesh_instance(st, geom, root))
      return false;
  }

  return true;
}

static
bool
stl_writer_begin(STLExpWriter * __restrict w,
                 FILE         * __restrict file,
                 bool                       ascii) {
  unsigned char header[STL_EXP_HEADER_SIZE + sizeof(uint32_t)];

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
  io_store_u32le(header + STL_EXP_HEADER_SIZE, 0u);
  stl_w_raw(w, header, sizeof(header));
  return w->result == AK_OK;
}

static
bool
stl_writer_end(STLExpWriter * __restrict w) {
  unsigned char out[4];
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
  io_store_u32le(out, w->triangleCount);
  if (fwrite(out, 1, sizeof(out), w->file) != sizeof(out))
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
