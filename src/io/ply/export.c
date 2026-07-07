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

#include "ply.h"
#include "../common/binary.h"
#include "../common/primitive.h"
#include "../common/text_number.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLY_EXP_FILE_BUFFER_SIZE (1024u * 1024u)
#define PLY_EXP_MAX_NODE_DEPTH   512u

typedef enum PLYExpPass {
  PLY_EXP_PASS_COUNT    = 0,
  PLY_EXP_PASS_VERTICES = 1,
  PLY_EXP_PASS_FACES    = 2
} PLYExpPass;

typedef IOFloatRows PLYExpRows;

#define ply_rows_init    io_float_rows_init
#define ply_rows_destroy io_float_rows_destroy
#define ply_rows_get     io_float_rows_get
#define ply_row_component io_float_row_component

typedef struct PLYExpWriter {
  FILE         *file;
  size_t        len;
  AkResult      result;
  bool          ascii;
  unsigned char buffer[64u * 1024u];
} PLYExpWriter;

typedef struct PLYExpState {
  AkDoc        *doc;
  PLYExpWriter w;
  PLYExpPass   pass;
  uint32_t     passObjectCount;
  uint32_t     vertexCount;
  uint32_t     faceCount;
  uint32_t     edgeCount;
  uint32_t     vertexCursor;
  bool         wantNormals;
  bool         wantUV;
  bool         wantColors;
  bool         colorSrgb;
  bool         triangulated;
  bool         hasNormals;
  bool         hasUV;
  bool         hasColors;
  bool         hasAlpha;
} PLYExpState;

typedef struct PLYExpPrim {
  AkMeshPrimitive *prim;
  AkInput    *posInput;
  AkInput    *normalInput;
  AkInput    *texInput;
  AkInput    *colorInput;
  PLYExpRows  posRows;
  PLYExpRows  normalRows;
  PLYExpRows  texRows;
  PLYExpRows  colorRows;
  mat4        world;
  mat4        normalMatrix;
  uint32_t    reusableVertexCount;
  bool        hasNormalRows;
  bool        hasTexRows;
  bool        hasColorRows;
  bool        mirrored;
  bool        reuseVertices;
} PLYExpPrim;

static
void
ply_w_flush(PLYExpWriter * __restrict w) {
  if (w->len == 0)
    return;

  if (w->result == AK_OK
      && fwrite(w->buffer, 1, w->len, w->file) != w->len)
    w->result = AK_ERR;

  w->len = 0;
}

static
void
ply_w_raw(PLYExpWriter * __restrict w,
          const void   * __restrict data,
          size_t                    len) {
  const unsigned char *src;

  src = data;
  while (len > 0) {
    size_t avail;
    size_t n;

    avail = sizeof(w->buffer) - w->len;
    if (avail == 0) {
      ply_w_flush(w);
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
ply_w_raw_small(PLYExpWriter * __restrict w,
                const void   * __restrict data,
                size_t                    len) {
  if (sizeof(w->buffer) - w->len < len)
    ply_w_flush(w);

  memcpy(w->buffer + w->len, data, len);
  w->len += len;
}

static
void
ply_w_ch(PLYExpWriter * __restrict w, char ch) {
  if (w->len == sizeof(w->buffer))
    ply_w_flush(w);

  w->buffer[w->len++] = (unsigned char)ch;
}

#define ply_w_lit(W, LIT) ply_w_raw((W), "" LIT, sizeof("" LIT) - 1u)

static
void
ply_w_float_ascii(PLYExpWriter * __restrict w, float value) {
  char  *dst;
  size_t avail;
  size_t outLen;

  avail = sizeof(w->buffer) - w->len;
  if (avail < 48u) {
    ply_w_flush(w);
    avail = sizeof(w->buffer) - w->len;
  }

  dst = (char *)w->buffer + w->len;
  if (!ak_io_text_format_float6(dst, avail, value, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  w->len += outLen;
}

static
void
ply_w_u32_ascii(PLYExpWriter * __restrict w, uint32_t value) {
  char *end;
  char  buf[16];

  end = ak_io_text_format_uint64(buf, value);
  ply_w_raw(w, buf, (size_t)(end - buf));
}

static
void
ply_w_u8_ascii(PLYExpWriter * __restrict w, uint8_t value) {
  char *end;
  char  buf[4];

  end = ak_io_text_format_uint64(buf, value);
  ply_w_raw(w, buf, (size_t)(end - buf));
}

static
void
ply_write_u8(PLYExpWriter * __restrict w, uint8_t value) {
  ply_w_raw(w, &value, sizeof(value));
}

static
void
ply_write_u32le(PLYExpWriter * __restrict w, uint32_t value) {
  unsigned char out[4];

  io_store_u32le(out, value);
  ply_w_raw(w, out, sizeof(out));
}

static
bool
ply_store_f32le(unsigned char ** __restrict cursor, float value) {
  if (!isfinite(value))
    return false;

  io_store_f32le(*cursor, value);
  *cursor += sizeof(float);
  return true;
}

static
bool
ply_count_add(uint32_t * __restrict value, uint32_t add) {
  if (UINT32_MAX - *value < add)
    return false;
  *value += add;
  return true;
}

static
bool
ply_count_face(PLYExpState * __restrict st, uint32_t vertexCount) {
  if (vertexCount > UINT8_MAX)
    return false;
  return ply_count_add(&st->vertexCount, vertexCount)
         && ply_count_add(&st->faceCount, 1u);
}

static
bool
ply_count_edge(PLYExpState * __restrict st) {
  return ply_count_add(&st->vertexCount, 2u)
         && ply_count_add(&st->edgeCount, 1u);
}

static
bool
ply_count_point(PLYExpState * __restrict st) {
  return ply_count_add(&st->vertexCount, 1u);
}

static
AkInput*
ply_find_texcoord_input(AkMeshPrimitive * __restrict prim) {
  return io_primitive_find_set_input(prim,
                                     AK_INPUT_TEXCOORD,
                                     AK_INPUT_UV,
                                     2u);
}

static
AkInput*
ply_find_color_input(AkMeshPrimitive * __restrict prim) {
  return io_primitive_find_set_input(prim,
                                     AK_INPUT_COLOR,
                                     AK_INPUT_COLOR,
                                     3u);
}

static
bool
ply_input_valid(AkInput * __restrict input, uint32_t minComponents) {
  return input
         && input->accessor
         && input->accessor->count > 0
         && input->accessor->componentCount >= minComponents;
}

static
bool
ply_input_uses_position_index(AkMeshPrimitive * __restrict prim,
                              AkInput         * __restrict posInput,
                              AkInput         * __restrict input,
                              uint32_t                     vertexCount) {
  uint32_t stride;

  if (!input)
    return true;
  if (!input->accessor || input->accessor->count < vertexCount)
    return false;

  if (!prim || !prim->indices)
    return true;

  stride = prim->indexStride ? prim->indexStride : 1u;
  if (stride <= 1u)
    return true;

  return input->indexOffset == posInput->indexOffset;
}

static
bool
ply_primitive_reusable_vertex_count(PLYExpState     * __restrict st,
                                    AkMeshPrimitive * __restrict prim,
                                    uint32_t        * __restrict countOut) {
  AkInput *posInput;
  AkInput *normalInput;
  AkInput *texInput;
  AkInput *colorInput;
  uint32_t vertexCount;

  posInput = io_primitive_find_input(prim, AK_INPUT_POSITION);
  if (!ply_input_valid(posInput, 3u))
    return false;

  vertexCount = posInput->accessor->count;
  if (vertexCount == 0)
    return false;

  if (st->hasNormals) {
    normalInput = io_primitive_find_input(prim, AK_INPUT_NORMAL);
    if (!ply_input_valid(normalInput, 3u)
        || !ply_input_uses_position_index(prim, posInput, normalInput, vertexCount))
      return false;
  }

  if (st->hasUV) {
    texInput = ply_find_texcoord_input(prim);
    if (texInput
        && !ply_input_uses_position_index(prim, posInput, texInput, vertexCount))
      return false;
  }

  if (st->hasColors) {
    colorInput = ply_find_color_input(prim);
    if (colorInput
        && !ply_input_uses_position_index(prim, posInput, colorInput, vertexCount))
      return false;
  }

  *countOut = vertexCount;
  return true;
}

static
void
ply_note_optional_inputs(PLYExpState    * __restrict st,
                         AkMeshPrimitive * __restrict prim) {
  AkInput *input;

  if (st->wantNormals) {
    input = io_primitive_find_input(prim, AK_INPUT_NORMAL);
    if (ply_input_valid(input, 3u))
      st->hasNormals = true;
  }

  if (st->wantUV) {
    input = ply_find_texcoord_input(prim);
    if (ply_input_valid(input, 2u))
      st->hasUV = true;
  }

  if (st->wantColors) {
    input = ply_find_color_input(prim);
    if (ply_input_valid(input, 3u)) {
      st->hasColors = true;
      if (input->accessor->componentCount > 3u)
        st->hasAlpha = true;
    }
  }
}

static
void
ply_vertex_position(PLYExpPrim * __restrict pc,
                    uint32_t                vertexIndex,
                    bool                    direct,
                    vec3                    out) {
  const float *row;
  uint32_t     posIndex;
  vec3         in;

  posIndex = direct
             ? vertexIndex
             : io_primitive_input_index(pc->prim, pc->posInput, vertexIndex);
  row      = ply_rows_get(&pc->posRows, posIndex);

  in[0] = ply_row_component(row, pc->posRows.componentCount, 0u, 0.0f);
  in[1] = ply_row_component(row, pc->posRows.componentCount, 1u, 0.0f);
  in[2] = ply_row_component(row, pc->posRows.componentCount, 2u, 0.0f);
  glm_mat4_mulv3(pc->world, in, 1.0f, out);
}

static
void
ply_vertex_normal(PLYExpPrim * __restrict pc,
                  uint32_t                vertexIndex,
                  bool                    direct,
                  vec3                    fallback,
                  vec3                    out) {
  const float *row;
  uint32_t     normIndex;
  vec3         in;

  if (!pc->hasNormalRows) {
    glm_vec3_copy(fallback, out);
    return;
  }

  normIndex = direct
              ? vertexIndex
              : io_primitive_input_index(pc->prim, pc->normalInput, vertexIndex);
  row       = ply_rows_get(&pc->normalRows, normIndex);

  in[0] = ply_row_component(row, pc->normalRows.componentCount, 0u, 0.0f);
  in[1] = ply_row_component(row, pc->normalRows.componentCount, 1u, 0.0f);
  in[2] = ply_row_component(row, pc->normalRows.componentCount, 2u, 1.0f);
  glm_mat4_mulv3(pc->normalMatrix, in, 0.0f, out);
  io_vec3_normalize_or_zero(out);
}

static
void
ply_vertex_texcoord(PLYExpPrim * __restrict pc,
                    uint32_t                vertexIndex,
                    bool                    direct,
                    float                   out[2]) {
  const float *row;
  uint32_t     texIndex;

  out[0] = 0.0f;
  out[1] = 0.0f;
  if (!pc->hasTexRows)
    return;

  texIndex = direct
             ? vertexIndex
             : io_primitive_input_index(pc->prim, pc->texInput, vertexIndex);
  row      = ply_rows_get(&pc->texRows, texIndex);
  out[0]   = ply_row_component(row, pc->texRows.componentCount, 0u, 0.0f);
  out[1]   = ply_row_component(row, pc->texRows.componentCount, 1u, 0.0f);
}

static
float
ply_color_component(PLYExpRows * __restrict rows,
                    const float * __restrict row,
                    uint32_t                 component,
                    float                    fallback) {
  float value;

  value = ply_row_component(row, rows->componentCount, component, fallback);

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
float
ply_linear_to_srgb(float value) {
  if (value <= 0.0031308f)
    return value * 12.92f;

  return 1.055f * powf(value, 1.0f / 2.4f) - 0.055f;
}

static
uint8_t
ply_color_u8(float value, bool srgb) {
  if (srgb)
    value = ply_linear_to_srgb(value);

  if (value < 0.0f)
    value = 0.0f;
  else if (value > 1.0f)
    value = 1.0f;

  return (uint8_t)(value * 255.0f + 0.5f);
}

static
void
ply_vertex_color(PLYExpState * __restrict st,
                 PLYExpPrim  * __restrict pc,
                 uint32_t                  vertexIndex,
                 bool                      direct,
                 uint8_t                   out[4]) {
  const float *row;
  uint32_t     colorIndex;

  out[0] = 255u;
  out[1] = 255u;
  out[2] = 255u;
  out[3] = 255u;
  if (!pc->hasColorRows)
    return;

  colorIndex = direct
               ? vertexIndex
               : io_primitive_input_index(pc->prim, pc->colorInput, vertexIndex);
  row = ply_rows_get(&pc->colorRows, colorIndex);

  out[0] = ply_color_u8(ply_color_component(&pc->colorRows, row, 0u, 1.0f),
                        st->colorSrgb);
  out[1] = ply_color_u8(ply_color_component(&pc->colorRows, row, 1u, 1.0f),
                        st->colorSrgb);
  out[2] = ply_color_u8(ply_color_component(&pc->colorRows, row, 2u, 1.0f),
                        st->colorSrgb);
  if (st->hasAlpha) {
    out[3] = ply_color_u8(ply_color_component(&pc->colorRows, row, 3u, 1.0f),
                          false);
  }
}

static
void
ply_write_vertex_record(PLYExpState * __restrict st,
                        PLYExpPrim  * __restrict pc,
                        uint32_t                  vertexIndex,
                        bool                      direct,
                        vec3                      fallbackNormal) {
  vec3    pos;
  vec3    normal;
  float   uv[2];
  uint8_t color[4];

  ply_vertex_position(pc, vertexIndex, direct, pos);

  if (st->w.ascii) {
    ply_w_float_ascii(&st->w, pos[0]);
    ply_w_ch(&st->w, ' ');
    ply_w_float_ascii(&st->w, pos[1]);
    ply_w_ch(&st->w, ' ');
    ply_w_float_ascii(&st->w, pos[2]);

    if (st->hasNormals) {
      ply_vertex_normal(pc, vertexIndex, direct, fallbackNormal, normal);
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, normal[0]);
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, normal[1]);
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, normal[2]);
    }

    if (st->hasUV) {
      ply_vertex_texcoord(pc, vertexIndex, direct, uv);
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, uv[0]);
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, uv[1]);
    }

    if (st->hasColors) {
      ply_vertex_color(st, pc, vertexIndex, direct, color);
      ply_w_ch(&st->w, ' ');
      ply_w_u8_ascii(&st->w, color[0]);
      ply_w_ch(&st->w, ' ');
      ply_w_u8_ascii(&st->w, color[1]);
      ply_w_ch(&st->w, ' ');
      ply_w_u8_ascii(&st->w, color[2]);
      if (st->hasAlpha) {
        ply_w_ch(&st->w, ' ');
        ply_w_u8_ascii(&st->w, color[3]);
      }
    }

    ply_w_ch(&st->w, '\n');
    return;
  }

  {
    unsigned char  out[40];
    unsigned char *p = out;

    if (!ply_store_f32le(&p, pos[0])
        || !ply_store_f32le(&p, pos[1])
        || !ply_store_f32le(&p, pos[2])) {
      st->w.result = AK_ERR;
      return;
    }

    if (st->hasNormals) {
      ply_vertex_normal(pc, vertexIndex, direct, fallbackNormal, normal);
      if (!ply_store_f32le(&p, normal[0])
          || !ply_store_f32le(&p, normal[1])
          || !ply_store_f32le(&p, normal[2])) {
        st->w.result = AK_ERR;
        return;
      }
    }

    if (st->hasUV) {
      ply_vertex_texcoord(pc, vertexIndex, direct, uv);
      if (!ply_store_f32le(&p, uv[0])
          || !ply_store_f32le(&p, uv[1])) {
        st->w.result = AK_ERR;
        return;
      }
    }

    if (st->hasColors) {
      ply_vertex_color(st, pc, vertexIndex, direct, color);
      *p++ = color[0];
      *p++ = color[1];
      *p++ = color[2];
      if (st->hasAlpha)
        *p++ = color[3];
    }

    ply_w_raw_small(&st->w, out, (size_t)(p - out));
  }
}

static
void
ply_ordered_index(const uint32_t * __restrict indices,
                  uint32_t                    count,
                  uint32_t                    outIndex,
                  bool                        mirrored,
                  uint32_t                  * out) {
  *out = mirrored ? indices[count - 1u - outIndex] : indices[outIndex];
}

static
void
ply_face_fallback_normal(PLYExpPrim       * __restrict pc,
                         const uint32_t   * __restrict indices,
                         uint32_t                      count,
                         bool                          mirrored,
                         vec3                          out) {
  uint32_t i0;
  uint32_t i1;
  uint32_t i2;
  vec3     a;
  vec3     b;
  vec3     c;

  if (count < 3u) {
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 1.0f;
    return;
  }

  ply_ordered_index(indices, count, 0u, mirrored, &i0);
  ply_ordered_index(indices, count, 1u, mirrored, &i1);
  ply_ordered_index(indices, count, 2u, mirrored, &i2);
  ply_vertex_position(pc, i0, false, a);
  ply_vertex_position(pc, i1, false, b);
  ply_vertex_position(pc, i2, false, c);
  io_triangle_normal(a, b, c, out);
}

static
void
ply_write_face_vertices(PLYExpState    * __restrict st,
                        PLYExpPrim     * __restrict pc,
                        const uint32_t * __restrict indices,
                        uint32_t                    count,
                        bool                        mirrored) {
  vec3     fallbackNormal;
  uint32_t i;

  ply_face_fallback_normal(pc, indices, count, mirrored, fallbackNormal);
  for (i = 0; i < count; i++) {
    uint32_t vertexIndex;

    ply_ordered_index(indices, count, i, mirrored, &vertexIndex);
    ply_write_vertex_record(st, pc, vertexIndex, false, fallbackNormal);
  }
}

static
void
ply_write_face_ref(PLYExpState * __restrict st, uint32_t count) {
  uint32_t i;

  if (st->w.ascii) {
    ply_w_u8_ascii(&st->w, (uint8_t)count);
    for (i = 0; i < count; i++) {
      ply_w_ch(&st->w, ' ');
      ply_w_u32_ascii(&st->w, st->vertexCursor + i);
    }
    ply_w_ch(&st->w, '\n');
  } else {
    if (count == 3u) {
      unsigned char out[1u + 3u * sizeof(uint32_t)];

      out[0] = 3u;
      io_store_u32le(out + 1u, st->vertexCursor);
      io_store_u32le(out + 5u, st->vertexCursor + 1u);
      io_store_u32le(out + 9u, st->vertexCursor + 2u);
      ply_w_raw_small(&st->w, out, sizeof(out));
    } else {
      ply_write_u8(&st->w, (uint8_t)count);
      for (i = 0; i < count; i++)
        ply_write_u32le(&st->w, st->vertexCursor + i);
    }
  }

  st->vertexCursor += count;
}

static
uint32_t
ply_reused_ref_index(PLYExpPrim * __restrict pc, uint32_t tupleIndex) {
  AkUInt index;

  index = io_primitive_input_index(pc->prim, pc->posInput, tupleIndex);
  if (index >= pc->reusableVertexCount)
    index = 0;
  return (uint32_t)index;
}

static
void
ply_write_reused_face_ref(PLYExpState    * __restrict st,
                          PLYExpPrim     * __restrict pc,
                          const uint32_t * __restrict indices,
                          uint32_t                    count) {
  uint32_t i;

  if (st->w.ascii) {
    ply_w_u8_ascii(&st->w, (uint8_t)count);
    for (i = 0; i < count; i++) {
      uint32_t tupleIndex;
      uint32_t refIndex;

      ply_ordered_index(indices, count, i, pc->mirrored, &tupleIndex);
      refIndex = ply_reused_ref_index(pc, tupleIndex);
      ply_w_ch(&st->w, ' ');
      ply_w_u32_ascii(&st->w, st->vertexCursor + refIndex);
    }
    ply_w_ch(&st->w, '\n');
  } else {
    if (count == 3u) {
      unsigned char out[1u + 3u * sizeof(uint32_t)];
      uint32_t      tuple0;
      uint32_t      tuple1;
      uint32_t      tuple2;

      out[0] = 3u;
      if (pc->mirrored) {
        tuple0 = indices[2];
        tuple1 = indices[1];
        tuple2 = indices[0];
      } else {
        tuple0 = indices[0];
        tuple1 = indices[1];
        tuple2 = indices[2];
      }

      io_store_u32le(out + 1u,
                     st->vertexCursor + ply_reused_ref_index(pc, tuple0));
      io_store_u32le(out + 5u,
                     st->vertexCursor + ply_reused_ref_index(pc, tuple1));
      io_store_u32le(out + 9u,
                     st->vertexCursor + ply_reused_ref_index(pc, tuple2));

      ply_w_raw_small(&st->w, out, sizeof(out));
    } else {
      ply_write_u8(&st->w, (uint8_t)count);
      for (i = 0; i < count; i++) {
        uint32_t tupleIndex;
        uint32_t refIndex;

        ply_ordered_index(indices, count, i, pc->mirrored, &tupleIndex);
        refIndex = ply_reused_ref_index(pc, tupleIndex);
        ply_write_u32le(&st->w, st->vertexCursor + refIndex);
      }
    }
  }
}

static
void
ply_write_edge_ref(PLYExpState * __restrict st) {
  if (st->w.ascii) {
    ply_w_u32_ascii(&st->w, st->vertexCursor);
    ply_w_ch(&st->w, ' ');
    ply_w_u32_ascii(&st->w, st->vertexCursor + 1u);
    ply_w_ch(&st->w, '\n');
  } else {
    ply_write_u32le(&st->w, st->vertexCursor);
    ply_write_u32le(&st->w, st->vertexCursor + 1u);
  }

  st->vertexCursor += 2u;
}

static
void
ply_write_reused_edge_ref(PLYExpState * __restrict st,
                          PLYExpPrim  * __restrict pc,
                          uint32_t                  i0,
                          uint32_t                  i1) {
  uint32_t ref0;
  uint32_t ref1;

  ref0 = st->vertexCursor + ply_reused_ref_index(pc, i0);
  ref1 = st->vertexCursor + ply_reused_ref_index(pc, i1);

  if (st->w.ascii) {
    ply_w_u32_ascii(&st->w, ref0);
    ply_w_ch(&st->w, ' ');
    ply_w_u32_ascii(&st->w, ref1);
    ply_w_ch(&st->w, '\n');
  } else {
    ply_write_u32le(&st->w, ref0);
    ply_write_u32le(&st->w, ref1);
  }
}

static
void
ply_write_reused_vertices(PLYExpState * __restrict st,
                          PLYExpPrim  * __restrict pc) {
  vec3     fallbackNormal = {0.0f, 0.0f, 1.0f};
  uint32_t i;

  for (i = 0; i < pc->reusableVertexCount; i++)
    ply_write_vertex_record(st, pc, i, true, fallbackNormal);
}

static
void
ply_write_edge_vertices(PLYExpState * __restrict st,
                        PLYExpPrim  * __restrict pc,
                        uint32_t                  i0,
                        uint32_t                  i1) {
  vec3 fallbackNormal = {0.0f, 0.0f, 1.0f};

  ply_write_vertex_record(st, pc, i0, false, fallbackNormal);
  ply_write_vertex_record(st, pc, i1, false, fallbackNormal);
}

static
void
ply_write_point_vertex(PLYExpState * __restrict st,
                       PLYExpPrim  * __restrict pc,
                       uint32_t                  i) {
  vec3 fallbackNormal = {0.0f, 0.0f, 1.0f};

  ply_write_vertex_record(st, pc, i, false, fallbackNormal);
}

static
bool
ply_prim_begin(PLYExpState    * __restrict st,
               PLYExpPrim     * __restrict pc,
               AkMeshPrimitive * __restrict prim,
               mat4                         world,
               bool                         mirrored) {
  memset(pc, 0, sizeof(*pc));

  pc->posInput = io_primitive_find_input(prim, AK_INPUT_POSITION);
  if (!ply_input_valid(pc->posInput, 3u))
    return false;

  pc->prim = prim;
  if (!ply_rows_init(&pc->posRows, pc->posInput->accessor))
    return false;
  pc->reuseVertices = ply_primitive_reusable_vertex_count(st,
                                                          prim,
                                                          &pc->reusableVertexCount);

  glm_mat4_copy(world, pc->world);
  glm_mat4_inv((vec4 *)world, pc->normalMatrix);
  glm_mat4_transpose(pc->normalMatrix);
  pc->mirrored = mirrored;

  if (st->hasNormals) {
    pc->normalInput = io_primitive_find_input(prim, AK_INPUT_NORMAL);
    if (ply_input_valid(pc->normalInput, 3u)) {
      pc->hasNormalRows = ply_rows_init(&pc->normalRows,
                                        pc->normalInput->accessor);
      if (!pc->hasNormalRows)
        goto fail;
    }
  }

  if (st->hasUV) {
    pc->texInput = ply_find_texcoord_input(prim);
    if (ply_input_valid(pc->texInput, 2u)) {
      pc->hasTexRows = ply_rows_init(&pc->texRows, pc->texInput->accessor);
      if (!pc->hasTexRows)
        goto fail;
    }
  }

  if (st->hasColors) {
    pc->colorInput = ply_find_color_input(prim);
    if (ply_input_valid(pc->colorInput, 3u)) {
      pc->hasColorRows = ply_rows_init(&pc->colorRows,
                                       pc->colorInput->accessor);
      if (!pc->hasColorRows)
        goto fail;
    }
  }

  return true;

fail:
  ply_rows_destroy(&pc->colorRows);
  ply_rows_destroy(&pc->texRows);
  ply_rows_destroy(&pc->normalRows);
  ply_rows_destroy(&pc->posRows);
  return false;
}

static
void
ply_prim_end(PLYExpPrim * __restrict pc) {
  ply_rows_destroy(&pc->colorRows);
  ply_rows_destroy(&pc->texRows);
  ply_rows_destroy(&pc->normalRows);
  ply_rows_destroy(&pc->posRows);
}

static
bool
ply_process_face(PLYExpState    * __restrict st,
                 PLYExpPrim     * __restrict pc,
                 const uint32_t * __restrict indices,
                 uint32_t                    count) {
  if (count < 3u)
    return true;
  if (count > UINT8_MAX)
    return false;

  switch (st->pass) {
    case PLY_EXP_PASS_COUNT:
      if (pc && pc->reuseVertices)
        return ply_count_add(&st->faceCount, 1u);
      return ply_count_face(st, count);
    case PLY_EXP_PASS_VERTICES:
      if (pc && pc->reuseVertices)
        return true;
      ply_write_face_vertices(st, pc, indices, count, pc->mirrored);
      return st->w.result == AK_OK;
    case PLY_EXP_PASS_FACES:
      if (pc && pc->reuseVertices) {
        ply_write_reused_face_ref(st, pc, indices, count);
        return st->w.result == AK_OK;
      }
      ply_write_face_ref(st, count);
      return st->w.result == AK_OK;
    default:
      return false;
  }
}

static
bool
ply_process_edge(PLYExpState * __restrict st,
                 PLYExpPrim  * __restrict pc,
                 uint32_t                  i0,
                 uint32_t                  i1) {
  switch (st->pass) {
    case PLY_EXP_PASS_COUNT:
      if (pc && pc->reuseVertices)
        return ply_count_add(&st->edgeCount, 1u);
      return ply_count_edge(st);
    case PLY_EXP_PASS_VERTICES:
      if (pc && pc->reuseVertices)
        return true;
      ply_write_edge_vertices(st, pc, i0, i1);
      return st->w.result == AK_OK;
    case PLY_EXP_PASS_FACES:
      if (pc && pc->reuseVertices) {
        ply_write_reused_edge_ref(st, pc, i0, i1);
        return st->w.result == AK_OK;
      }
      ply_write_edge_ref(st);
      return st->w.result == AK_OK;
    default:
      return false;
  }
}

static
bool
ply_process_point(PLYExpState * __restrict st,
                  PLYExpPrim  * __restrict pc,
                  uint32_t                  i) {
  switch (st->pass) {
    case PLY_EXP_PASS_COUNT:
      if (pc && pc->reuseVertices)
        return true;
      return ply_count_point(st);
    case PLY_EXP_PASS_VERTICES:
      if (pc && pc->reuseVertices)
        return true;
      ply_write_point_vertex(st, pc, i);
      return st->w.result == AK_OK;
    case PLY_EXP_PASS_FACES:
      if (!pc || !pc->reuseVertices)
        st->vertexCursor++;
      return true;
    default:
      return false;
  }
}

static
bool
ply_write_triangles_primitive(PLYExpState    * __restrict st,
                              PLYExpPrim     * __restrict pc,
                              AkMeshPrimitive * __restrict prim) {
  IOTriangleIter iter;
  uint32_t       tri[3];

  if (!io_triangle_iter_init(&iter, prim))
    return true;

  while (io_triangle_iter_next(&iter, tri)) {
    if (!ply_process_face(st, pc, tri, 3u))
      return false;
  }

  return true;
}

static
bool
ply_write_polygon_primitive(PLYExpState    * __restrict st,
                            PLYExpPrim     * __restrict pc,
                            AkMeshPrimitive * __restrict prim) {
  AkPolygon *poly;
  size_t     cursor;
  size_t     i;

  poly = (AkPolygon *)prim;
  if (!poly->vcount || poly->vcount->count == 0)
    return true;

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

      if (st->triangulated) {
        for (j = 1u; j + 1u < vc; j++) {
          uint32_t tri[3] = {local[0], local[j], local[j + 1u]};
          if (!ply_process_face(st, pc, tri, 3u))
            return false;
        }
      } else if (!ply_process_face(st, pc, local, vc)) {
        return false;
      }
    } else {
      uint32_t *heapLocal;

      heapLocal = malloc(sizeof(*heapLocal) * vc);
      if (!heapLocal)
        return false;

      for (j = 0; j < vc; j++)
        heapLocal[j] = (uint32_t)(cursor + j);

      if (st->triangulated) {
        for (j = 1u; j + 1u < vc; j++) {
          uint32_t tri[3] = {heapLocal[0], heapLocal[j], heapLocal[j + 1u]};
          if (!ply_process_face(st, pc, tri, 3u)) {
            free(heapLocal);
            return false;
          }
        }
      } else if (!ply_process_face(st, pc, heapLocal, vc)) {
        free(heapLocal);
        return false;
      }

      free(heapLocal);
    }

    cursor += vc;
  }

  return true;
}

static
bool
ply_write_lines_primitive(PLYExpState    * __restrict st,
                          PLYExpPrim     * __restrict pc,
                          AkMeshPrimitive * __restrict prim) {
  AkLineMode mode;
  uint32_t   vertexCount;
  uint32_t   i;

  vertexCount = io_primitive_vertex_count(prim);
  mode        = ((AkLines *)prim)->mode;
  if (mode == 0)
    mode = AK_LINES;

  if (mode == AK_LINE_STRIP || mode == AK_LINE_LOOP) {
    for (i = 0; i + 1u < vertexCount; i++) {
      if (!ply_process_edge(st, pc, i, i + 1u))
        return false;
    }
    if (mode == AK_LINE_LOOP && vertexCount > 1u)
      return ply_process_edge(st, pc, vertexCount - 1u, 0u);
    return true;
  }

  for (i = 0; i + 1u < vertexCount; i += 2u) {
    if (!ply_process_edge(st, pc, i, i + 1u))
      return false;
  }

  return true;
}

static
bool
ply_write_points_primitive(PLYExpState    * __restrict st,
                           PLYExpPrim     * __restrict pc,
                           AkMeshPrimitive * __restrict prim) {
  uint32_t vertexCount;
  uint32_t i;

  vertexCount = io_primitive_vertex_count(prim);
  for (i = 0; i < vertexCount; i++) {
    if (!ply_process_point(st, pc, i))
      return false;
  }

  return true;
}

static
bool
ply_write_primitive(PLYExpState    * __restrict st,
                    AkMeshPrimitive * __restrict prim,
                    mat4                         world,
                    bool                         mirrored) {
  PLYExpPrim pc;
  bool       ok;

  if (!prim)
    return true;

  if (prim->type != AK_PRIMITIVE_TRIANGLES
      && prim->type != AK_PRIMITIVE_POLYGONS
      && prim->type != AK_PRIMITIVE_LINES
      && prim->type != AK_PRIMITIVE_POINTS)
    return true;

  if (st->pass == PLY_EXP_PASS_COUNT) {
    AkInput *posInput;

    posInput = io_primitive_find_input(prim, AK_INPUT_POSITION);
    if (!ply_input_valid(posInput, 3u))
      return true;

    ply_note_optional_inputs(st, prim);
    memset(&pc, 0, sizeof(pc));
    pc.prim = prim;
    pc.posInput = posInput;
    pc.reuseVertices = ply_primitive_reusable_vertex_count(st,
                                                           prim,
                                                           &pc.reusableVertexCount);
    if (pc.reuseVertices
        && !ply_count_add(&st->vertexCount, pc.reusableVertexCount))
      return false;
  } else if (!ply_prim_begin(st, &pc, prim, world, mirrored)) {
    return false;
  }

  if (st->pass == PLY_EXP_PASS_VERTICES && pc.reuseVertices) {
    ply_write_reused_vertices(st, &pc);
    ok = st->w.result == AK_OK;
    ply_prim_end(&pc);
    return ok;
  }

  switch (prim->type) {
    case AK_PRIMITIVE_TRIANGLES:
      ok = ply_write_triangles_primitive(st, &pc, prim);
      break;
    case AK_PRIMITIVE_POLYGONS:
      ok = ply_write_polygon_primitive(st, &pc, prim);
      break;
    case AK_PRIMITIVE_LINES:
      ok = ply_write_lines_primitive(st, &pc, prim);
      break;
    case AK_PRIMITIVE_POINTS:
      ok = ply_write_points_primitive(st, &pc, prim);
      break;
    default:
      ok = true;
      break;
  }

  if (st->pass != PLY_EXP_PASS_COUNT)
    ply_prim_end(&pc);

  if (ok && st->pass == PLY_EXP_PASS_FACES && pc.reuseVertices)
    st->vertexCursor += pc.reusableVertexCount;

  return ok && st->w.result == AK_OK;
}

static
bool
ply_write_mesh_instance(PLYExpState * __restrict st,
                        AkGeometry  * __restrict geom,
                        mat4                      world) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  bool             mirrored;

  if (!st || !geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return true;

  mesh = ak_objGet(geom->gdata);
  if (!mesh || !mesh->primitive)
    return true;

  mirrored = glm_mat4_det(world) < 0.0f;
  st->passObjectCount++;

  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (!ply_write_primitive(st, prim, world, mirrored))
      return false;
  }

  return st->w.result == AK_OK;
}

static
AkGeometry*
ply_instance_geometry(AkInstanceGeometry * __restrict inst) {
  void *obj;

  if (!inst)
    return NULL;

  obj = ak_instanceObject(&inst->base);
  return obj;
}

static
bool
ply_write_node(PLYExpState * __restrict st,
               AkNode      * __restrict node,
               mat4                      parentWorld,
               uint32_t                  depth) {
  AkInstanceBase *base;
  AkNode         *child;
  AkInstanceNode *nodeRef;
  AkMatrix        localMatrix;
  mat4            world;

  if (!node)                          return true;
  if (depth > PLY_EXP_MAX_NODE_DEPTH) return false;

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
    geom = ply_instance_geometry(inst);

    if (!ply_write_mesh_instance(st, geom, world))
      return false;
  }

  for (child = node->chld; child; child = child->next) {
    if (!ply_write_node(st, child, world, depth + 1u))
      return false;
  }

  for (nodeRef = node->node; nodeRef; nodeRef = nodeRef->next) {
    AkNode *target;

    target = ak_instanceNodeTarget(nodeRef);
    if (target && !ply_write_node(st, target, world, depth + 1u))
      return false;
  }

  return true;
}

static
bool
ply_write_scene(PLYExpState * __restrict st) {
  mat4 identity;

  glm_mat4_identity(identity);
  if (st->doc->scene && st->doc->scene->node)
    return ply_write_node(st, st->doc->scene->node, identity, 0u);

  return true;
}

static
bool
ply_write_library_fallback(PLYExpState * __restrict st) {
  AkGeometry *geom;
  mat4        identity;

  if (st->passObjectCount > 0)
    return true;

  glm_mat4_identity(identity);
  for (geom = st->doc->lib.geometries.first; geom; geom = geom->next) {
    if (!ply_write_mesh_instance(st, geom, identity))
      return false;
  }

  return true;
}

static
bool
ply_writer_begin(PLYExpWriter * __restrict w,
                 FILE         * __restrict file,
                 bool                       ascii) {
  memset(w, 0, sizeof(*w));
  w->file   = file;
  w->result = AK_OK;
  w->ascii  = ascii;

  return true;
}

static
void
ply_write_header(PLYExpState * __restrict st) {
  ply_w_lit(&st->w, "ply\nformat ");
  if (st->w.ascii)
    ply_w_lit(&st->w, "ascii");
  else
    ply_w_lit(&st->w, "binary_little_endian");
  ply_w_lit(&st->w, " 1.0\ncomment Generated by AssetKit\n");

  ply_w_lit(&st->w, "element vertex ");
  ply_w_u32_ascii(&st->w, st->vertexCount);
  ply_w_lit(&st->w, "\nproperty float x\nproperty float y\nproperty float z\n");

  if (st->hasNormals) {
    ply_w_lit(&st->w,
              "property float nx\n"
              "property float ny\n"
              "property float nz\n");
  }

  if (st->hasUV)
    ply_w_lit(&st->w, "property float s\nproperty float t\n");

  if (st->hasColors) {
    ply_w_lit(&st->w,
              "property uchar red\n"
              "property uchar green\n"
              "property uchar blue\n");
    if (st->hasAlpha)
      ply_w_lit(&st->w, "property uchar alpha\n");
  }

  ply_w_lit(&st->w, "element face ");
  ply_w_u32_ascii(&st->w, st->faceCount);
  ply_w_lit(&st->w, "\nproperty list uchar uint vertex_indices\n");

  if (st->edgeCount > 0) {
    ply_w_lit(&st->w, "element edge ");
    ply_w_u32_ascii(&st->w, st->edgeCount);
    ply_w_lit(&st->w, "\nproperty uint vertex1\nproperty uint vertex2\n");
  }

  ply_w_lit(&st->w, "end_header\n");
}

static
bool
ply_writer_end(PLYExpWriter * __restrict w) {
  ply_w_flush(w);
  return w->result == AK_OK;
}

AK_HIDE
AkResult
ply_export(AkDoc * __restrict doc, const char * __restrict filepath) {
  PLYExpState st;
  FILE       *file;
  AkResult    result;
  uintptr_t   format;
  uintptr_t   colorMode;

  if (!doc || !filepath)
    return AK_ERR;

  memset(&st, 0, sizeof(st));
  st.doc          = doc;
  st.wantNormals  = ak_opt_get(AK_OPT_PLY_EXPORT_NORMALS) != 0;
  st.wantUV       = ak_opt_get(AK_OPT_PLY_EXPORT_UV) != 0;
  st.triangulated = ak_opt_get(AK_OPT_PLY_EXPORT_TRIANGULATED) != 0;

  colorMode       = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  st.wantColors   = colorMode != AK_PLY_EXPORT_COLOR_NONE;
  st.colorSrgb    = colorMode == AK_PLY_EXPORT_COLOR_SRGB;

  format = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);

  file = fopen(filepath, "wb");
  if (!file)
    return AK_EBADF;
  (void)setvbuf(file, NULL, _IOFBF, PLY_EXP_FILE_BUFFER_SIZE);

  result = AK_OK;
  if (!ply_writer_begin(&st.w, file, format == AK_PLY_EXPORT_ASCII)) {
    result = AK_ERR;
    goto done;
  }

  st.pass = PLY_EXP_PASS_COUNT;
  st.passObjectCount = 0;
  if (!ply_write_scene(&st) || !ply_write_library_fallback(&st)) {
    result = AK_ERR;
    goto done;
  }

  ply_write_header(&st);
  if (st.w.result != AK_OK) {
    result = AK_ERR;
    goto done;
  }

  st.pass         = PLY_EXP_PASS_VERTICES;
  st.vertexCursor = 0;
  st.passObjectCount = 0;
  if (!ply_write_scene(&st) || !ply_write_library_fallback(&st)) {
    result = AK_ERR;
    goto done;
  }

  st.pass         = PLY_EXP_PASS_FACES;
  st.vertexCursor = 0;
  st.passObjectCount = 0;
  if (!ply_write_scene(&st) || !ply_write_library_fallback(&st)) {
    result = AK_ERR;
    goto done;
  }

  if (!ply_writer_end(&st.w))
    result = AK_ERR;

done:
  if (fclose(file) != 0 && result == AK_OK)
    result = AK_ERR;

  if (result != AK_OK)
    remove(filepath);

  return result;
}
