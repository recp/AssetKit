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
#include "../common/export_space.h"
#include "../common/primitive.h"
#include "../common/text_number.h"
#include "../../thread.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLY_EXP_FILE_BUFFER_SIZE (1024u * 1024u)
#define PLY_EXP_MAX_NODE_DEPTH   512u

/*
 * Texture baking is deliberately bounded.  The exporter streams generated
 * vertices/faces and never retains a tessellated mesh, but an unbounded UV
 * footprint could still create an impractically large file.  These limits
 * cap only texture-bake expansion; source geometry is never dropped.
 */
#define PLY_EXP_BAKE_MAX_EDGE_SEGMENTS 128u
#define PLY_EXP_BAKE_MAX_EXTRA_FACES   1000000ull
#define PLY_EXP_BAKE_SAMPLES_PER_TEXEL 2.0f
#define PLY_EXP_BAKE_SAMPLES_PER_METRE 16.0f
#define PLY_EXP_BAKE_IMAGE_METRIC_CAP  64u
#define PLY_EXP_BAKE_PARALLEL_MIN_VERTICES 512u
#define PLY_EXP_BAKE_PARALLEL_MAX_TASKS 8u

typedef enum PLYExpPass {
  PLY_EXP_PASS_DISCOVER = 0,
  PLY_EXP_PASS_PLAN     = 1,
  PLY_EXP_PASS_COUNT    = 2,
  PLY_EXP_PASS_VERTICES = 3,
  PLY_EXP_PASS_FACES    = 4,
  PLY_EXP_PASS_EDGES    = 5
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

typedef struct PLYExpImageMetric {
  AkImageData *image;
  float        detail;
} PLYExpImageMetric;

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
  bool         bakeTextures;
  bool         hasNormals;
  bool         hasUV;
  bool         hasColors;
  bool         hasAlpha;
  bool         hasTextureColors;
  uint64_t     bakeDesiredExtraFaces;
  uint64_t     bakeExtraRemaining;
  double       bakeExtraScale;
  PLYExpImageMetric imageMetric[PLY_EXP_BAKE_IMAGE_METRIC_CAP];
  uint32_t     imageMetricCount;
  unsigned char *bakeVertexBuffer;
  size_t        bakeVertexBufferCapacity;
} PLYExpState;

typedef struct PLYExpPrim {
  AkMeshPrimitive *prim;
  AkInput    *posInput;
  AkInput    *normalInput;
  AkInput    *texInput;
  AkInput    *sampleTexInput;
  AkInput    *colorInput;
  PLYExpRows  posRows;
  PLYExpRows  normalRows;
  PLYExpRows  texRows;
  PLYExpRows  sampleTexRows;
  PLYExpRows  colorRows;
  mat4        world;
  mat4        normalMatrix;
  AkColor     materialColor;
  AkTextureRef *baseColorTexture;
  AkImageData  *baseColorImage;
  float       textureDetail;
  uint32_t    reusableVertexCount;
  bool        hasNormalRows;
  bool        hasTexRows;
  bool        hasSampleTexRows;
  bool        hasColorRows;
  bool        hasMaterialColor;
  bool        flipTextureV;
  bool        bakeTexture;
  bool        mirrored;
  bool        reuseVertices;
} PLYExpPrim;

static
AkMaterialInput*
ply_resolved_base_color(AkMeshPrimitive     * __restrict prim,
                        AkInstanceGeometry  * __restrict inst,
                        AkResolvedMaterial  * __restrict resolved,
                        AkMaterialProperty ** __restrict propertyOut) {
  AkMaterialProperty *property;

  memset(resolved, 0, sizeof(*resolved));
  if (!ak_materialResolve(prim, inst, UINT32_MAX, resolved)) {
    if (propertyOut)
      *propertyOut = NULL;
    return NULL;
  }

  property = ak_resolvedMaterialProperty(resolved);
  if (propertyOut)
    *propertyOut = property;

  return property && property->baseColor
         ? property->baseColor
         : resolved->surface ? resolved->surface->baseColor : NULL;
}

static
bool
ply_material_input_color(const AkMaterialInput * __restrict input,
                         AkColor               * __restrict color) {
  if (!input || !color)
    return false;

  color->rgba.R = 1.0f;
  color->rgba.G = 1.0f;
  color->rgba.B = 1.0f;
  color->rgba.A = 1.0f;

  switch (input->valueType) {
    case AK_MATERIAL_VALUE_COLOR:
      *color = input->color;
      return true;
    case AK_MATERIAL_VALUE_FLOAT4:
      memcpy(color->vec, input->value, sizeof(color->vec));
      return true;
    case AK_MATERIAL_VALUE_FLOAT3:
      color->rgba.R = input->value[0];
      color->rgba.G = input->value[1];
      color->rgba.B = input->value[2];
      return true;
    case AK_MATERIAL_VALUE_FLOAT2:
      color->rgba.R = input->value[0];
      color->rgba.G = input->value[1];
      return true;
    case AK_MATERIAL_VALUE_FLOAT:
      color->rgba.R = input->value[0];
      color->rgba.G = input->value[0];
      color->rgba.B = input->value[0];
      return true;
    default:
      return false;
  }
}

static
bool
ply_resolved_material_color(AkMeshPrimitive    * __restrict prim,
                            AkInstanceGeometry * __restrict inst,
                            AkColor            * __restrict color) {
  AkResolvedMaterial resolved;
  AkMaterialProperty *property;
  AkMaterialInput    *input;

  input = ply_resolved_base_color(prim, inst, &resolved, &property);
  if (!resolved.material && !resolved.surface && !property)
    return false;

  if (!ply_material_input_color(input, color)) {
    if (!property)
      return false;
    *color = property->displayColor;
  }

  color->rgba.A *= ak_materialOpacityFactor(resolved.surface);
  return true;
}

static
AkTextureRef*
ply_resolved_base_color_texture(AkMeshPrimitive     * __restrict prim,
                                AkInstanceGeometry  * __restrict inst) {
  AkResolvedMaterial resolved;
  AkMaterialInput   *input;

  input = ply_resolved_base_color(prim, inst, &resolved, NULL);
  return ak_materialInputTexture(input);
}

static
bool
ply_image_data_valid(AkImageData * __restrict data) {
  size_t pixels;

  if (!data || !data->data || data->width == 0 || data->height == 0
      || data->comp < 1 || data->comp > 4)
    return false;

  if ((size_t)data->width > SIZE_MAX / (size_t)data->height)
    return false;
  pixels = (size_t)data->width * (size_t)data->height;
  return pixels <= SIZE_MAX / (size_t)data->comp;
}

static
float
ply_image_detail_compute(AkImageData * __restrict image) {
  const uint32_t grid = 64u;
  double         sum[3] = {0.0, 0.0, 0.0};
  double         squareSum[3] = {0.0, 0.0, 0.0};
  uint64_t       sampleCount;
  uint32_t       x;
  uint32_t       y;
  uint32_t       channel;
  double         maxVariance;
  float          detail;

  if (!ply_image_data_valid(image))
    return 1.0f;

  sampleCount = 0u;
  for (y = 0; y < grid; y++) {
    uint32_t iy;

    iy = image->height > 1u
         ? (uint32_t)(((uint64_t)y * (image->height - 1u)) / (grid - 1u))
         : 0u;
    for (x = 0; x < grid; x++) {
      const uint8_t *pixel;
      uint32_t       ix;

      ix = image->width > 1u
           ? (uint32_t)(((uint64_t)x * (image->width - 1u)) / (grid - 1u))
           : 0u;
      pixel = (const uint8_t *)image->data
              + (((size_t)iy * image->width + ix) * (uint32_t)image->comp);
      for (channel = 0; channel < 3u; channel++) {
        uint32_t sourceChannel;
        double   value;

        sourceChannel = image->comp < 3 ? 0u : channel;
        value = (double)pixel[sourceChannel] * (1.0 / 255.0);
        sum[channel]       += value;
        squareSum[channel] += value * value;
      }
      sampleCount++;
    }
  }

  maxVariance = 0.0;
  for (channel = 0; channel < 3u; channel++) {
    double mean;
    double variance;

    mean     = sum[channel] / (double)sampleCount;
    variance = squareSum[channel] / (double)sampleCount - mean * mean;
    if (variance > maxVariance)
      maxVariance = variance;
  }

  if (maxVariance <= 1e-8)
    return 0.0f;

  /* Contrast controls how much of the shared bake budget a texture needs.
     sqrt keeps low-contrast relief visible while avoiding dense subdivision
     for near-uniform metal/concrete maps. */
  detail = sqrtf(sqrtf((float)fmax(maxVariance, 0.0)) / 0.03f);
  if (detail < 0.2f)
    detail = 0.2f;
  else if (detail > 1.0f)
    detail = 1.0f;
  return detail;
}

static
float
ply_image_detail(PLYExpState * __restrict st,
                 AkImageData * __restrict image) {
  uint32_t i;
  float    detail;

  for (i = 0; i < st->imageMetricCount; i++) {
    if (st->imageMetric[i].image == image)
      return st->imageMetric[i].detail;
  }

  detail = ply_image_detail_compute(image);
  if (st->imageMetricCount < PLY_EXP_BAKE_IMAGE_METRIC_CAP) {
    i = st->imageMetricCount++;
    st->imageMetric[i].image  = image;
    st->imageMetric[i].detail = detail;
  }
  return detail;
}

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
  char *dst;
  char *end;

  if (sizeof(w->buffer) - w->len < 16u)
    ply_w_flush(w);

  dst = (char *)w->buffer + w->len;
  end = ak_io_text_format_uint32(dst, value);
  w->len += (size_t)(end - dst);
}

static
void
ply_w_u8_ascii(PLYExpWriter * __restrict w, uint8_t value) {
  char *dst;
  char *end;

  if (sizeof(w->buffer) - w->len < 4u)
    ply_w_flush(w);

  dst = (char *)w->buffer + w->len;
  end = ak_io_text_format_uint32(dst, value);
  w->len += (size_t)(end - dst);
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
ply_find_texture_texcoord_input(AkMeshPrimitive     * __restrict prim,
                                AkInstanceGeometry  * __restrict inst,
                                AkTextureRef        * __restrict texref) {
  AkInput *input;
  AkInput *fallback;
  int      slot;

  /*
   * COLLADA binds texture-coordinate semantics on the instance, not on the
   * material itself.  Resolve that binding before looking up an input set so
   * a textured PLY samples the same UVs as the source material.
   */
  slot = ak_materialTextureSlot(prim, inst, texref);

  fallback = NULL;
  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if ((input->semantic != AK_INPUT_TEXCOORD && input->semantic != AK_INPUT_UV)
        || !input->accessor
        || input->accessor->componentCount < 2u)
      continue;
    if ((int)input->set == slot)
      return input;
    if (!fallback)
      fallback = input;
  }
  return fallback;
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
ply_primitive_base_color_texture(AkMeshPrimitive     * __restrict prim,
                                 AkInstanceGeometry  * __restrict inst,
                                 AkTextureRef        ** __restrict texrefOut,
                                 AkImageData         ** __restrict imageOut,
                                 AkInput             ** __restrict texcoordOut) {
  AkTextureRef *texref;
  AkTexture    *texture;
  AkImage      *image;
  AkInput      *texcoord;

  texref = ply_resolved_base_color_texture(prim, inst);
  texture = texref ? texref->texture : NULL;
  image = texture ? texture->image : NULL;
  if (!image)
    return false;

  texcoord = ply_find_texture_texcoord_input(prim, inst, texref);
  if (!ply_input_valid(texcoord, 2u))
    return false;

  if (!image->data)
    ak_imageLoad(image);
  if (!ply_image_data_valid(image->data))
    return false;

  if (texrefOut)
    *texrefOut = texref;
  if (imageOut)
    *imageOut = image->data;
  if (texcoordOut)
    *texcoordOut = texcoord;
  return true;
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
ply_primitive_reusable_vertex_count(PLYExpState       * __restrict st,
                                    AkMeshPrimitive   * __restrict prim,
                                    AkInstanceGeometry * __restrict inst,
                                    uint32_t          * __restrict countOut) {
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

    if (st->bakeTextures
        && st->hasTextureColors
        && ply_primitive_base_color_texture(prim,
                                            inst,
                                            NULL,
                                            NULL,
                                            &texInput)
        && !ply_input_uses_position_index(prim,
                                          posInput,
                                          texInput,
                                          vertexCount))
      return false;
  }

  *countOut = vertexCount;
  return true;
}

static
void
ply_note_optional_inputs(PLYExpState      * __restrict st,
                         AkMeshPrimitive  * __restrict prim,
                         AkInstanceGeometry * __restrict inst) {
  AkImageData  *imageData;
  AkInput      *input;
  AkTextureRef *texref;
  AkColor       materialColor;

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

    if (ply_resolved_material_color(prim, inst, &materialColor)) {
      st->hasColors = true;
      if (materialColor.rgba.A < 0.999f)
        st->hasAlpha = true;
    }

    if (st->bakeTextures
        && ply_primitive_base_color_texture(prim,
                                            inst,
                                            &texref,
                                            &imageData,
                                            NULL)) {
      st->hasColors        = true;
      st->hasTextureColors = true;
      if ((imageData->comp == 2 || imageData->comp == 4)
          && (texref->channels == AK_TEXTURE_CHANNEL_NONE
              || (texref->channels & AK_TEXTURE_CHANNEL_A) != 0))
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
uint8_t
ply_color_u8(float value, bool srgb) {
  if (!isfinite(value))
    return 0u;

  if (srgb)
    return ak_linear_to_srgb8_fast(value);

  if (value < 0.0f)
    value = 0.0f;
  else if (value > 1.0f)
    value = 1.0f;

  return (uint8_t)(value * 255.0f + 0.5f);
}

static
float
ply_wrap_coord(float value, AkWrapMode mode, bool * __restrict border) {
  float period;

  *border = false;
  switch (mode) {
    case AK_WRAP_MODE_CLAMP:
      return value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
    case AK_WRAP_MODE_BORDER:
      if (value < 0.0f || value > 1.0f) {
        *border = true;
        return 0.0f;
      }
      return value;
    case AK_WRAP_MODE_MIRROR:
      period = fmodf(value, 2.0f);
      if (period < 0.0f)
        period += 2.0f;
      return period <= 1.0f ? period : 2.0f - period;
    case AK_WRAP_MODE_MIRROR_ONCE:
      value = fabsf(value);
      return value > 1.0f ? 1.0f : value;
    case AK_WRAP_MODE_WRAP:
    default:
      value -= floorf(value);
      return value;
  }
}

static
void
ply_texture_border(AkSampler * __restrict sampler, float out[4]) {
  if (sampler && sampler->borderColor) {
    memcpy(out, sampler->borderColor->vec, sizeof(sampler->borderColor->vec));
    return;
  }

  out[0] = 0.0f;
  out[1] = 0.0f;
  out[2] = 0.0f;
  out[3] = 0.0f;
}

static
void
ply_texture_texel(PLYExpPrim * __restrict pc,
                  int32_t                  x,
                  int32_t                  y,
                  float                    out[4]) {
  AkImageData *image;
  const uint8_t *pixel;
  uint32_t comp;
  bool srgb;

  image = pc->baseColorImage;
  comp  = (uint32_t)image->comp;
  pixel = (const uint8_t *)image->data
          + (((size_t)y * image->width + (uint32_t)x) * comp);
  srgb  = pc->baseColorTexture->colorSpace
          != AK_TEXTURE_COLORSPACE_LINEAR;

  switch (comp) {
    case 1:
      out[0] = out[1] = out[2] = srgb
                                     ? ak_srgb8_to_linearf_fast(pixel[0])
                                     : (float)pixel[0] * (1.0f / 255.0f);
      out[3] = 1.0f;
      break;
    case 2:
      out[0] = out[1] = out[2] = srgb
                                     ? ak_srgb8_to_linearf_fast(pixel[0])
                                     : (float)pixel[0] * (1.0f / 255.0f);
      out[3] = (float)pixel[1] * (1.0f / 255.0f);
      break;
    default:
      out[0] = srgb ? ak_srgb8_to_linearf_fast(pixel[0])
                    : (float)pixel[0] * (1.0f / 255.0f);
      out[1] = srgb ? ak_srgb8_to_linearf_fast(pixel[1])
                    : (float)pixel[1] * (1.0f / 255.0f);
      out[2] = srgb ? ak_srgb8_to_linearf_fast(pixel[2])
                    : (float)pixel[2] * (1.0f / 255.0f);
      out[3] = comp == 4 ? (float)pixel[3] * (1.0f / 255.0f) : 1.0f;
      break;
  }

  if (pc->baseColorTexture->channels != AK_TEXTURE_CHANNEL_NONE
      && (pc->baseColorTexture->channels & AK_TEXTURE_CHANNEL_A) == 0)
    out[3] = 1.0f;
}

static
int32_t
ply_texture_tap_index(int32_t     index,
                      uint32_t    size,
                      AkWrapMode  mode,
                      bool       * __restrict border) {
  int32_t last;

  *border = false;
  last = (int32_t)size - 1;
  if (index >= 0 && index <= last)
    return index;

  switch (mode) {
    case AK_WRAP_MODE_BORDER:
      *border = true;
      return 0;
    case AK_WRAP_MODE_WRAP:
      /* The primary coordinate has already been wrapped into [0, 1), so a
         bilinear neighbor can be at most one texel beyond either edge. */
      return index < 0 ? last : 0;
    case AK_WRAP_MODE_CLAMP:
    case AK_WRAP_MODE_MIRROR:
    case AK_WRAP_MODE_MIRROR_ONCE:
    default:
      return index < 0 ? 0 : last;
  }
}

static
void
ply_texture_transform_uv(PLYExpPrim * __restrict pc,
                         const float             in[2],
                         float                   out[2]) {
  AkTextureTransform *transform;
  float               tx;
  float               ty;
  float               c;
  float               s;

  /* Malformed source UVs must never reach floorf/fmodf or an integer texel
     conversion. Resolve each component independently so one finite authored
     coordinate remains useful when its pair is invalid. */
  out[0] = isfinite(in[0]) ? in[0] : 0.0f;
  out[1] = isfinite(in[1]) ? in[1] : 0.0f;
  transform = pc->baseColorTexture ? pc->baseColorTexture->transform : NULL;
  if (transform) {
    tx     = out[0] * transform->scale[0];
    ty     = out[1] * transform->scale[1];
    c      = cosf(transform->rotation);
    s      = sinf(transform->rotation);
    out[0] = c * tx - s * ty + transform->offset[0];
    out[1] = s * tx + c * ty + transform->offset[1];
  }

  if (pc->flipTextureV)
    out[1] = 1.0f - out[1];

  /* A malformed/overflowing texture transform is handled by the same stable
     origin fallback before wrap/filter logic. */
  if (!isfinite(out[0]))
    out[0] = 0.0f;
  if (!isfinite(out[1]))
    out[1] = 0.0f;
}

static
void
ply_vertex_sample_texcoord(PLYExpPrim * __restrict pc,
                           uint32_t                vertexIndex,
                           bool                    direct,
                           float                   out[2]) {
  const float *row;
  uint32_t     texIndex;

  out[0] = 0.0f;
  out[1] = 0.0f;
  if (!pc->hasSampleTexRows)
    return;

  texIndex = direct
             ? vertexIndex
             : io_primitive_input_index(pc->prim,
                                        pc->sampleTexInput,
                                        vertexIndex);
  row      = ply_rows_get(&pc->sampleTexRows, texIndex);
  out[0]   = ply_row_component(row,
                               pc->sampleTexRows.componentCount,
                               0u,
                               0.0f);
  out[1]   = ply_row_component(row,
                               pc->sampleTexRows.componentCount,
                               1u,
                               0.0f);
}

static
void
ply_texture_sample_uv(PLYExpPrim * __restrict pc,
                      const float             inputUV[2],
                      float                   out[4]) {
  AkImageData       *image;
  AkSampler         *sampler;
  AkWrapMode         wrapS;
  AkWrapMode         wrapT;
  float              u;
  float              v;
  float              x;
  float              y;
  float              tx;
  float              ty;
  float              transformedUV[2];
  float              a[4];
  float              b[4];
  float              d[4];
  float              e[4];
  int32_t            x0;
  int32_t            x1;
  int32_t            y0;
  int32_t            y1;
  int32_t            tapX0;
  int32_t            tapX1;
  int32_t            tapY0;
  int32_t            tapY1;
  bool               borderS;
  bool               borderT;
  bool               borderX0;
  bool               borderX1;
  bool               borderY0;
  bool               borderY1;
  uint32_t           i;

  out[0] = out[1] = out[2] = out[3] = 1.0f;
  if (!pc->hasSampleTexRows || !pc->baseColorImage || !pc->baseColorTexture)
    return;

  ply_texture_transform_uv(pc, inputUV, transformedUV);
  u = transformedUV[0];
  v = transformedUV[1];

  sampler = pc->baseColorTexture->texture
            ? pc->baseColorTexture->texture->sampler : NULL;
  wrapS   = sampler && sampler->wrapS ? sampler->wrapS : AK_WRAP_MODE_WRAP;
  wrapT   = sampler && sampler->wrapT ? sampler->wrapT : AK_WRAP_MODE_WRAP;
  u       = ply_wrap_coord(u, wrapS, &borderS);
  v       = ply_wrap_coord(v, wrapT, &borderT);
  if (borderS || borderT) {
    ply_texture_border(sampler, out);
    return;
  }

  image = pc->baseColorImage;
  if (sampler && sampler->magfilter == AK_MAGFILTER_NEAREST) {
    x0 = (int32_t)floorf(u * (float)image->width);
    y0 = (int32_t)floorf(v * (float)image->height);
    if (x0 >= (int32_t)image->width)
      x0 = (int32_t)image->width - 1;
    if (y0 >= (int32_t)image->height)
      y0 = (int32_t)image->height - 1;
    ply_texture_texel(pc, x0, y0, out);
    return;
  }

  x  = u * (float)image->width - 0.5f;
  y  = v * (float)image->height - 0.5f;
  x0 = (int32_t)floorf(x);
  y0 = (int32_t)floorf(y);
  tx = x - (float)x0;
  ty = y - (float)y0;
  x1 = x0 + 1;
  y1 = y0 + 1;

  /* The primary coordinate is already resolved. Resolve only the two integer
     taps per axis instead of round-tripping all four texels through float
     coordinates, wrap math and floorf. */
  tapX0 = ply_texture_tap_index(x0, image->width,  wrapS, &borderX0);
  tapX1 = ply_texture_tap_index(x1, image->width,  wrapS, &borderX1);
  tapY0 = ply_texture_tap_index(y0, image->height, wrapT, &borderY0);
  tapY1 = ply_texture_tap_index(y1, image->height, wrapT, &borderY1);

  if (borderX0 || borderY0)
    ply_texture_border(sampler, a);
  else
    ply_texture_texel(pc, tapX0, tapY0, a);

  if (borderX1 || borderY0)
    ply_texture_border(sampler, b);
  else
    ply_texture_texel(pc, tapX1, tapY0, b);

  if (borderX0 || borderY1)
    ply_texture_border(sampler, d);
  else
    ply_texture_texel(pc, tapX0, tapY1, d);

  if (borderX1 || borderY1)
    ply_texture_border(sampler, e);
  else
    ply_texture_texel(pc, tapX1, tapY1, e);

  for (i = 0; i < 4u; i++) {
    float top;
    float bottom;

    top    = a[i] + (b[i] - a[i]) * tx;
    bottom = d[i] + (e[i] - d[i]) * tx;
    out[i] = top + (bottom - top) * ty;
  }
}

static
void
ply_texture_sample_area(PLYExpPrim * __restrict pc,
                        const float             inputUV[2],
                        const float             du[2],
                        const float             dv[2],
                        float                   out[4]) {
  float center[2];
  float edgeU[2];
  float edgeV[2];
  float transformedCenter[2];
  float transformedU[2];
  float transformedV[2];
  float footprintU;
  float footprintV;
  uint32_t grid;
  uint32_t x;
  uint32_t y;
  uint32_t channel;

  center[0] = inputUV[0];
  center[1] = inputUV[1];
  edgeU[0]  = inputUV[0] + du[0];
  edgeU[1]  = inputUV[1] + du[1];
  edgeV[0]  = inputUV[0] + dv[0];
  edgeV[1]  = inputUV[1] + dv[1];
  ply_texture_transform_uv(pc, center, transformedCenter);
  ply_texture_transform_uv(pc, edgeU, edgeU);
  ply_texture_transform_uv(pc, edgeV, edgeV);

  transformedU[0] = edgeU[0] - transformedCenter[0];
  transformedU[1] = edgeU[1] - transformedCenter[1];
  transformedV[0] = edgeV[0] - transformedCenter[0];
  transformedV[1] = edgeV[1] - transformedCenter[1];
  footprintU = hypotf(transformedU[0] * pc->baseColorImage->width,
                      transformedU[1] * pc->baseColorImage->height);
  footprintV = hypotf(transformedV[0] * pc->baseColorImage->width,
                      transformedV[1] * pc->baseColorImage->height);
  if (fmaxf(footprintU, footprintV) <= 1.25f) {
    ply_texture_sample_uv(pc, inputUV, out);
    return;
  }

  grid = fmaxf(footprintU, footprintV) > 3.0f ? 4u : 2u;
  out[0] = out[1] = out[2] = out[3] = 0.0f;
  for (y = 0; y < grid; y++) {
    float fy;

    fy = ((float)y + 0.5f) / (float)grid - 0.5f;
    for (x = 0; x < grid; x++) {
      float fx;
      float uv[2];
      float sampled[4];

      fx    = ((float)x + 0.5f) / (float)grid - 0.5f;
      uv[0] = inputUV[0] + du[0] * fx + dv[0] * fy;
      uv[1] = inputUV[1] + du[1] * fx + dv[1] * fy;
      ply_texture_sample_uv(pc, uv, sampled);
      for (channel = 0; channel < 4u; channel++)
        out[channel] += sampled[channel];
    }
  }

  for (channel = 0; channel < 4u; channel++)
    out[channel] *= 1.0f / (float)(grid * grid);
}

static
void
ply_texture_sample(PLYExpPrim * __restrict pc,
                   uint32_t                vertexIndex,
                   bool                    direct,
                   float                   out[4]) {
  float uv[2];

  ply_vertex_sample_texcoord(pc, vertexIndex, direct, uv);
  ply_texture_sample_uv(pc, uv, out);
}

static
void
ply_vertex_color_factor(PLYExpPrim * __restrict pc,
                        uint32_t                vertexIndex,
                        bool                    direct,
                        float                   out[4]) {
  const float *row;
  uint32_t     colorIndex;

  out[0] = out[1] = out[2] = out[3] = 1.0f;
  if (!pc->hasColorRows)
    return;

  colorIndex = direct
               ? vertexIndex
               : io_primitive_input_index(pc->prim, pc->colorInput, vertexIndex);
  row = ply_rows_get(&pc->colorRows, colorIndex);

  out[0] = ply_color_component(&pc->colorRows, row, 0u, 1.0f);
  out[1] = ply_color_component(&pc->colorRows, row, 1u, 1.0f);
  out[2] = ply_color_component(&pc->colorRows, row, 2u, 1.0f);
  out[3] = ply_color_component(&pc->colorRows, row, 3u, 1.0f);
}

static
void
ply_vertex_color(PLYExpState * __restrict st,
                 PLYExpPrim  * __restrict pc,
                 uint32_t                  vertexIndex,
                 bool                      direct,
                 float                     out[4]) {
  float factor[4];

  out[0] = 1.0f;
  out[1] = 1.0f;
  out[2] = 1.0f;
  out[3] = 1.0f;

  if (pc->hasMaterialColor)
    memcpy(out, pc->materialColor.vec, sizeof(pc->materialColor.vec));

  if (pc->hasSampleTexRows) {
    float sampled[4];

    ply_texture_sample(pc, vertexIndex, direct, sampled);
    out[0] *= sampled[0];
    out[1] *= sampled[1];
    out[2] *= sampled[2];
    out[3] *= sampled[3];
  }

  ply_vertex_color_factor(pc, vertexIndex, direct, factor);
  out[0] *= factor[0];
  out[1] *= factor[1];
  out[2] *= factor[2];
  if (st->hasAlpha)
    out[3] *= factor[3];
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
  float   color[4];

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
      if (st->colorSrgb) {
        ply_w_u8_ascii(&st->w, ply_color_u8(color[0], true));
        ply_w_ch(&st->w, ' ');
        ply_w_u8_ascii(&st->w, ply_color_u8(color[1], true));
        ply_w_ch(&st->w, ' ');
        ply_w_u8_ascii(&st->w, ply_color_u8(color[2], true));
        if (st->hasAlpha) {
          ply_w_ch(&st->w, ' ');
          ply_w_u8_ascii(&st->w, ply_color_u8(color[3], false));
        }
      } else {
        ply_w_float_ascii(&st->w, color[0]);
        ply_w_ch(&st->w, ' ');
        ply_w_float_ascii(&st->w, color[1]);
        ply_w_ch(&st->w, ' ');
        ply_w_float_ascii(&st->w, color[2]);
        if (st->hasAlpha) {
          ply_w_ch(&st->w, ' ');
          ply_w_float_ascii(&st->w, color[3]);
        }
      }
    }

    ply_w_ch(&st->w, '\n');
    return;
  }

  {
    unsigned char  out[56];
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
      if (st->colorSrgb) {
        *p++ = ply_color_u8(color[0], true);
        *p++ = ply_color_u8(color[1], true);
        *p++ = ply_color_u8(color[2], true);
        if (st->hasAlpha)
          *p++ = ply_color_u8(color[3], false);
      } else if (!ply_store_f32le(&p, color[0])
                 || !ply_store_f32le(&p, color[1])
                 || !ply_store_f32le(&p, color[2])
                 || (st->hasAlpha && !ply_store_f32le(&p, color[3]))) {
        st->w.result = AK_ERR;
        return;
      }
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

typedef struct PLYExpBakeCorner {
  vec3  position;
  vec3  normal;
  float uv[2];
  float sampleUV[2];
  float colorFactor[4];
} PLYExpBakeCorner;

typedef struct PLYExpBakeVertex {
  vec3  position;
  vec3  normal;
  float uv[2];
  float color[4];
} PLYExpBakeVertex;

static
float
ply_bake_mix3(float a, float b, float c, float wa, float wb, float wc) {
  return a * wa + b * wb + c * wc;
}

static
void
ply_bake_vertex_values(PLYExpPrim              * __restrict pc,
                       const PLYExpBakeCorner    corner[3],
                       float                     wa,
                       float                     wb,
                       float                     wc,
                       const float               sampleDu[2],
                       const float               sampleDv[2],
                       PLYExpBakeVertex         * __restrict vertex) {
  float sampleUV[2];
  float sampled[4];
  uint32_t i;

  for (i = 0; i < 3u; i++) {
    vertex->position[i] = ply_bake_mix3(corner[0].position[i],
                                        corner[1].position[i],
                                        corner[2].position[i],
                                        wa,
                                        wb,
                                        wc);
    vertex->normal[i] = ply_bake_mix3(corner[0].normal[i],
                                      corner[1].normal[i],
                                      corner[2].normal[i],
                                      wa,
                                      wb,
                                      wc);
  }
  io_vec3_normalize_or_zero(vertex->normal);

  for (i = 0; i < 2u; i++) {
    vertex->uv[i] = ply_bake_mix3(corner[0].uv[i],
                                  corner[1].uv[i],
                                  corner[2].uv[i],
                                  wa,
                                  wb,
                                  wc);
    sampleUV[i] = ply_bake_mix3(corner[0].sampleUV[i],
                                corner[1].sampleUV[i],
                                corner[2].sampleUV[i],
                                wa,
                                wb,
                                wc);
  }

  vertex->color[0] = vertex->color[1] = 1.0f;
  vertex->color[2] = vertex->color[3] = 1.0f;
  if (pc->hasMaterialColor)
    memcpy(vertex->color,
           pc->materialColor.vec,
           sizeof(pc->materialColor.vec));

  ply_texture_sample_area(pc, sampleUV, sampleDu, sampleDv, sampled);
  for (i = 0; i < 4u; i++) {
    float factor;

    factor = ply_bake_mix3(corner[0].colorFactor[i],
                           corner[1].colorFactor[i],
                           corner[2].colorFactor[i],
                           wa,
                           wb,
                           wc);
    vertex->color[i] *= sampled[i] * factor;
  }
}

static
bool
ply_store_bake_vertex_binary(const PLYExpState      * __restrict st,
                             const PLYExpBakeVertex * __restrict vertex,
                             unsigned char          * __restrict out,
                             size_t                 * __restrict outLen) {
  unsigned char *p;

  p = out;
  if (!ply_store_f32le(&p, vertex->position[0])
      || !ply_store_f32le(&p, vertex->position[1])
      || !ply_store_f32le(&p, vertex->position[2]))
    return false;

  if (st->hasNormals
      && (!ply_store_f32le(&p, vertex->normal[0])
          || !ply_store_f32le(&p, vertex->normal[1])
          || !ply_store_f32le(&p, vertex->normal[2])))
    return false;

  if (st->hasUV
      && (!ply_store_f32le(&p, vertex->uv[0])
          || !ply_store_f32le(&p, vertex->uv[1])))
    return false;

  if (st->hasColors) {
    if (st->colorSrgb) {
      *p++ = ply_color_u8(vertex->color[0], true);
      *p++ = ply_color_u8(vertex->color[1], true);
      *p++ = ply_color_u8(vertex->color[2], true);
      if (st->hasAlpha)
        *p++ = ply_color_u8(vertex->color[3], false);
    } else if (!ply_store_f32le(&p, vertex->color[0])
               || !ply_store_f32le(&p, vertex->color[1])
               || !ply_store_f32le(&p, vertex->color[2])
               || (st->hasAlpha
                   && !ply_store_f32le(&p, vertex->color[3]))) {
      return false;
    }
  }

  *outLen = (size_t)(p - out);
  return true;
}

static
void
ply_write_bake_vertex_record(PLYExpState             * __restrict st,
                             PLYExpPrim              * __restrict pc,
                             const PLYExpBakeCorner    corner[3],
                             float                     wa,
                             float                     wb,
                             float                     wc,
                             const float               sampleDu[2],
                             const float               sampleDv[2]) {
  PLYExpBakeVertex vertex;

  ply_bake_vertex_values(pc,
                         corner,
                         wa,
                         wb,
                         wc,
                         sampleDu,
                         sampleDv,
                         &vertex);

  if (st->w.ascii) {
    ply_w_float_ascii(&st->w, vertex.position[0]);
    ply_w_ch(&st->w, ' ');
    ply_w_float_ascii(&st->w, vertex.position[1]);
    ply_w_ch(&st->w, ' ');
    ply_w_float_ascii(&st->w, vertex.position[2]);

    if (st->hasNormals) {
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, vertex.normal[0]);
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, vertex.normal[1]);
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, vertex.normal[2]);
    }

    if (st->hasUV) {
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, vertex.uv[0]);
      ply_w_ch(&st->w, ' ');
      ply_w_float_ascii(&st->w, vertex.uv[1]);
    }

    if (st->hasColors) {
      ply_w_ch(&st->w, ' ');
      if (st->colorSrgb) {
        ply_w_u8_ascii(&st->w, ply_color_u8(vertex.color[0], true));
        ply_w_ch(&st->w, ' ');
        ply_w_u8_ascii(&st->w, ply_color_u8(vertex.color[1], true));
        ply_w_ch(&st->w, ' ');
        ply_w_u8_ascii(&st->w, ply_color_u8(vertex.color[2], true));
        if (st->hasAlpha) {
          ply_w_ch(&st->w, ' ');
          ply_w_u8_ascii(&st->w, ply_color_u8(vertex.color[3], false));
        }
      } else {
        ply_w_float_ascii(&st->w, vertex.color[0]);
        ply_w_ch(&st->w, ' ');
        ply_w_float_ascii(&st->w, vertex.color[1]);
        ply_w_ch(&st->w, ' ');
        ply_w_float_ascii(&st->w, vertex.color[2]);
        if (st->hasAlpha) {
          ply_w_ch(&st->w, ' ');
          ply_w_float_ascii(&st->w, vertex.color[3]);
        }
      }
    }

    ply_w_ch(&st->w, '\n');
    return;
  }

  {
    unsigned char  out[56];
    size_t         outLen;

    if (!ply_store_bake_vertex_binary(st, &vertex, out, &outLen)) {
      st->w.result = AK_ERR;
      return;
    }

    ply_w_raw_small(&st->w, out, outLen);
  }
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
uint32_t
ply_bake_grid_vertex_count(uint32_t segments) {
  return (segments + 1u) * (segments + 2u) / 2u;
}

static
uint32_t
ply_bake_grid_row_start(uint32_t segments, uint32_t row) {
  return row * (segments + 1u) - row * (row - 1u) / 2u;
}

static
uint32_t
ply_bake_desired_segments(PLYExpPrim     * __restrict pc,
                          const uint32_t * __restrict indices) {
  float    transformed[3][2];
  vec3     position[3];
  float    uv[2];
  float    longestTexel;
  float    longestWorld;
  float    texelTarget;
  float    worldTarget;
  uint32_t texelSegments;
  uint32_t worldSegments;
  uint32_t segments;
  uint32_t i;

  if (!pc || !pc->bakeTexture || !pc->baseColorImage)
    return 1u;

  for (i = 0; i < 3u; i++) {
    ply_vertex_sample_texcoord(pc, indices[i], false, uv);
    ply_texture_transform_uv(pc, uv, transformed[i]);
    ply_vertex_position(pc, indices[i], false, position[i]);
  }

  longestTexel = 0.0f;
  longestWorld = 0.0f;
  for (i = 0; i < 3u; i++) {
    uint32_t j;
    float    dx;
    float    dy;
    float    texelLength;
    float    worldLength;

    j  = (i + 1u) % 3u;
    dx = (transformed[j][0] - transformed[i][0])
         * (float)pc->baseColorImage->width;
    dy = (transformed[j][1] - transformed[i][1])
         * (float)pc->baseColorImage->height;
    texelLength = hypotf(dx, dy);
    worldLength = glm_vec3_distance(position[i], position[j]);
    if (isfinite(texelLength) && texelLength > longestTexel)
      longestTexel = texelLength;
    if (isfinite(worldLength) && worldLength > longestWorld)
      longestWorld = worldLength;
  }

  if (longestTexel <= 0.5f || longestWorld <= 0.0f)
    return 1u;

  texelTarget = ceilf(longestTexel
                      * PLY_EXP_BAKE_SAMPLES_PER_TEXEL
                      * pc->textureDetail);
  worldTarget = ceilf(longestWorld
                      * PLY_EXP_BAKE_SAMPLES_PER_METRE
                      * pc->textureDetail);
  if (!isfinite(texelTarget) || texelTarget > PLY_EXP_BAKE_MAX_EDGE_SEGMENTS)
    texelTarget = PLY_EXP_BAKE_MAX_EDGE_SEGMENTS;
  if (!isfinite(worldTarget) || worldTarget > PLY_EXP_BAKE_MAX_EDGE_SEGMENTS)
    worldTarget = PLY_EXP_BAKE_MAX_EDGE_SEGMENTS;
  texelSegments = (uint32_t)texelTarget;
  worldSegments = (uint32_t)worldTarget;
  segments      = texelSegments < worldSegments
                  ? texelSegments : worldSegments;
  if (segments > PLY_EXP_BAKE_MAX_EDGE_SEGMENTS)
    segments = PLY_EXP_BAKE_MAX_EDGE_SEGMENTS;
  return segments > 1u ? segments : 1u;
}

static
uint32_t
ply_bake_segments(PLYExpState    * __restrict st,
                  PLYExpPrim     * __restrict pc,
                  const uint32_t * __restrict indices) {
  uint32_t desired;
  uint32_t chosen;
  uint64_t desiredExtra;
  uint64_t scaledExtra;
  uint64_t square;

  desired      = ply_bake_desired_segments(pc, indices);
  desiredExtra = (uint64_t)desired * desired - 1u;
  chosen       = desired;
  if (desiredExtra > 0u && st->bakeExtraScale < 1.0) {
    scaledExtra = (uint64_t)floor((double)desiredExtra
                                  * st->bakeExtraScale);
    square      = scaledExtra + 1u;
    chosen      = (uint32_t)sqrt((double)square);
    if (chosen < 1u)
      chosen = 1u;
    while ((uint64_t)(chosen + 1u) * (chosen + 1u) <= square)
      chosen++;
    while ((uint64_t)chosen * chosen > square)
      chosen--;
  }

  /* Floating-point budget scaling is deterministic, but retain an integer
     final guard so roundoff can never exceed the advertised hard cap. */
  scaledExtra = (uint64_t)chosen * chosen - 1u;
  if (scaledExtra > st->bakeExtraRemaining) {
    square = st->bakeExtraRemaining + 1u;
    chosen = (uint32_t)sqrt((double)square);
    while ((uint64_t)(chosen + 1u) * (chosen + 1u) <= square)
      chosen++;
    while ((uint64_t)chosen * chosen > square)
      chosen--;
    scaledExtra = (uint64_t)chosen * chosen - 1u;
  }
  st->bakeExtraRemaining -= scaledExtra;
  return chosen;
}

static
void
ply_bake_note_desired(PLYExpState * __restrict st, uint32_t segments) {
  uint64_t extra;

  extra = (uint64_t)segments * segments - 1u;
  if (UINT64_MAX - st->bakeDesiredExtraFaces < extra)
    st->bakeDesiredExtraFaces = UINT64_MAX;
  else
    st->bakeDesiredExtraFaces += extra;
}

static
void
ply_bake_fill_corners(PLYExpState          * __restrict st,
                      PLYExpPrim           * __restrict pc,
                      const uint32_t       * __restrict indices,
                      PLYExpBakeCorner       corner[3]) {
  vec3     fallbackNormal;
  uint32_t i;

  (void)st;
  ply_face_fallback_normal(pc, indices, 3u, pc->mirrored, fallbackNormal);
  for (i = 0; i < 3u; i++) {
    uint32_t vertexIndex;

    ply_ordered_index(indices, 3u, i, pc->mirrored, &vertexIndex);
    ply_vertex_position(pc, vertexIndex, false, corner[i].position);
    ply_vertex_normal(pc,
                      vertexIndex,
                      false,
                      fallbackNormal,
                      corner[i].normal);
    ply_vertex_texcoord(pc, vertexIndex, false, corner[i].uv);
    ply_vertex_sample_texcoord(pc,
                               vertexIndex,
                               false,
                               corner[i].sampleUV);
    ply_vertex_color_factor(pc,
                            vertexIndex,
                            false,
                            corner[i].colorFactor);
  }
}

typedef struct PLYExpBakeRowWorker {
  const PLYExpState      *st;
  PLYExpPrim             *pc;
  const PLYExpBakeCorner *corner;
  const float            *sampleDu;
  const float            *sampleDv;
  unsigned char          *out;
  size_t                  recordStride;
  float                   invSegments;
  uint32_t                segments;
  uint32_t                rowBegin;
  uint32_t                rowEnd;
  bool                    ok;
} PLYExpBakeRowWorker;

static
size_t
ply_bake_binary_vertex_stride(const PLYExpState * __restrict st) {
  size_t stride;

  stride = 3u * sizeof(float);
  if (st->hasNormals)
    stride += 3u * sizeof(float);
  if (st->hasUV)
    stride += 2u * sizeof(float);
  if (st->hasColors) {
    if (st->colorSrgb)
      stride += st->hasAlpha ? 4u : 3u;
    else
      stride += (st->hasAlpha ? 4u : 3u) * sizeof(float);
  }
  return stride;
}

static
void
ply_bake_row_worker(void *userdata) {
  PLYExpBakeRowWorker *worker;
  uint32_t             row;
  uint32_t             column;

  worker     = userdata;
  worker->ok = true;
  for (row = worker->rowBegin; row < worker->rowEnd; row++) {
    uint32_t rowStart;

    rowStart = ply_bake_grid_row_start(worker->segments, row);
    for (column = 0; column <= worker->segments - row; column++) {
      PLYExpBakeVertex vertex;
      unsigned char   *out;
      size_t           outLen;
      float            wa;
      float            wb;
      float            wc;

      wb = (float)column * worker->invSegments;
      wc = (float)row * worker->invSegments;
      wa = 1.0f - wb - wc;
      ply_bake_vertex_values(worker->pc,
                             worker->corner,
                             wa,
                             wb,
                             wc,
                             worker->sampleDu,
                             worker->sampleDv,
                             &vertex);

      out = worker->out
            + (size_t)(rowStart + column) * worker->recordStride;
      if (!ply_store_bake_vertex_binary(worker->st,
                                        &vertex,
                                        out,
                                        &outLen)
          || outLen != worker->recordStride) {
        worker->ok = false;
        return;
      }
    }
  }
}

static
bool
ply_bake_write_vertices_parallel(PLYExpState             * __restrict st,
                                 PLYExpPrim              * __restrict pc,
                                 const PLYExpBakeCorner    corner[3],
                                 uint32_t                  segments,
                                 float                     invSegments,
                                 const float               sampleDu[2],
                                 const float               sampleDv[2]) {
  PLYExpBakeRowWorker workers[PLY_EXP_BAKE_PARALLEL_MAX_TASKS];
  AkThreadTask        tasks[PLY_EXP_BAKE_PARALLEL_MAX_TASKS];
  unsigned char      *buffer;
  size_t              bytes;
  size_t              recordStride;
  uint32_t            taskCount;
  uint32_t            vertexCount;
  uint32_t            row;
  uint32_t            i;

  vertexCount = ply_bake_grid_vertex_count(segments);
  if (st->w.ascii || vertexCount < PLY_EXP_BAKE_PARALLEL_MIN_VERTICES)
    return false;

  recordStride = ply_bake_binary_vertex_stride(st);
  if (recordStride == 0u || vertexCount > SIZE_MAX / recordStride)
    return false;
  bytes = (size_t)vertexCount * recordStride;
  if (st->bakeVertexBufferCapacity < bytes) {
    buffer = realloc(st->bakeVertexBuffer, bytes);
    if (!buffer)
      return false;
    st->bakeVertexBuffer         = buffer;
    st->bakeVertexBufferCapacity = bytes;
  }

  taskCount = ak_thread_cpu_count();
  if (taskCount > PLY_EXP_BAKE_PARALLEL_MAX_TASKS)
    taskCount = PLY_EXP_BAKE_PARALLEL_MAX_TASKS;
  if (taskCount > segments + 1u)
    taskCount = segments + 1u;

  row = 0u;
  for (i = 0; i < taskCount; i++) {
    uint32_t rowEnd;

    if (i + 1u == taskCount) {
      rowEnd = segments + 1u;
    } else {
      uint32_t target;

      target = (uint32_t)(((uint64_t)vertexCount * (i + 1u)) / taskCount);
      rowEnd = row + 1u;
      while (rowEnd < segments + 1u
             && ply_bake_grid_row_start(segments, rowEnd) < target)
        rowEnd++;

      if (segments + 1u - rowEnd < taskCount - i - 1u)
        rowEnd = segments + 1u - (taskCount - i - 1u);
    }

    workers[i].st           = st;
    workers[i].pc           = pc;
    workers[i].corner       = corner;
    workers[i].sampleDu     = sampleDu;
    workers[i].sampleDv     = sampleDv;
    workers[i].out          = st->bakeVertexBuffer;
    workers[i].recordStride = recordStride;
    workers[i].invSegments  = invSegments;
    workers[i].segments     = segments;
    workers[i].rowBegin     = row;
    workers[i].rowEnd       = rowEnd;
    workers[i].ok           = false;
    tasks[i].func           = ply_bake_row_worker;
    tasks[i].userdata       = &workers[i];
    row                     = rowEnd;
  }

  if (!ak_thread_run_tasks(tasks, taskCount))
    return false;

  for (i = 0; i < taskCount; i++) {
    if (!workers[i].ok) {
      st->w.result = AK_ERR;
      return true;
    }
  }

  ply_w_raw(&st->w, st->bakeVertexBuffer, bytes);
  return true;
}

static
void
ply_bake_write_vertices(PLYExpState    * __restrict st,
                        PLYExpPrim     * __restrict pc,
                        const uint32_t * __restrict indices,
                        uint32_t                    segments) {
  PLYExpBakeCorner corner[3];
  float            invSegments;
  float            sampleDu[2];
  float            sampleDv[2];
  uint32_t         row;

  ply_bake_fill_corners(st, pc, indices, corner);
  invSegments = 1.0f / (float)segments;
  sampleDu[0] = (corner[1].sampleUV[0] - corner[0].sampleUV[0])
                * invSegments;
  sampleDu[1] = (corner[1].sampleUV[1] - corner[0].sampleUV[1])
                * invSegments;
  sampleDv[0] = (corner[2].sampleUV[0] - corner[0].sampleUV[0])
                * invSegments;
  sampleDv[1] = (corner[2].sampleUV[1] - corner[0].sampleUV[1])
                * invSegments;

  if (ply_bake_write_vertices_parallel(st,
                                       pc,
                                       corner,
                                       segments,
                                       invSegments,
                                       sampleDu,
                                       sampleDv))
    return;

  for (row = 0; row <= segments; row++) {
    uint32_t column;

    for (column = 0; column <= segments - row; column++) {
      float wb;
      float wc;
      float wa;

      wb = (float)column * invSegments;
      wc = (float)row * invSegments;
      wa = 1.0f - wb - wc;
      ply_write_bake_vertex_record(st,
                                   pc,
                                   corner,
                                   wa,
                                   wb,
                                   wc,
                                   sampleDu,
                                   sampleDv);
    }
  }
}

static
void
ply_bake_write_ref(PLYExpState * __restrict st,
                   uint32_t                  a,
                   uint32_t                  b,
                   uint32_t                  c) {
  if (st->w.ascii) {
    ply_w_lit(&st->w, "3 ");
    ply_w_u32_ascii(&st->w, a);
    ply_w_ch(&st->w, ' ');
    ply_w_u32_ascii(&st->w, b);
    ply_w_ch(&st->w, ' ');
    ply_w_u32_ascii(&st->w, c);
    ply_w_ch(&st->w, '\n');
  } else {
    unsigned char out[1u + 3u * sizeof(uint32_t)];

    out[0] = 3u;
    io_store_u32le(out + 1u, a);
    io_store_u32le(out + 5u, b);
    io_store_u32le(out + 9u, c);
    ply_w_raw_small(&st->w, out, sizeof(out));
  }
}

static
void
ply_bake_write_faces(PLYExpState * __restrict st, uint32_t segments) {
  uint32_t base;
  uint32_t row;

  base = st->vertexCursor;
  for (row = 0; row < segments; row++) {
    uint32_t top;
    uint32_t bottom;
    uint32_t column;

    top    = ply_bake_grid_row_start(segments, row);
    bottom = ply_bake_grid_row_start(segments, row + 1u);
    for (column = 0; column < segments - row; column++) {
      ply_bake_write_ref(st,
                         base + top + column,
                         base + top + column + 1u,
                         base + bottom + column);
      if (column + 1u < segments - row) {
        ply_bake_write_ref(st,
                           base + top + column + 1u,
                           base + bottom + column + 1u,
                           base + bottom + column);
      }
    }
  }

  st->vertexCursor += ply_bake_grid_vertex_count(segments);
}

static
bool
ply_process_baked_triangle(PLYExpState    * __restrict st,
                           PLYExpPrim     * __restrict pc,
                           const uint32_t * __restrict indices) {
  uint32_t segments;
  uint32_t vertexCount;
  uint32_t faceCount;

  if (st->pass == PLY_EXP_PASS_PLAN) {
    ply_bake_note_desired(st, ply_bake_desired_segments(pc, indices));
    return true;
  }

  segments  = ply_bake_segments(st, pc, indices);
  vertexCount = ply_bake_grid_vertex_count(segments);
  faceCount = segments * segments;
  switch (st->pass) {
    case PLY_EXP_PASS_COUNT:
      return ply_count_add(&st->vertexCount, vertexCount)
             && ply_count_add(&st->faceCount, faceCount);
    case PLY_EXP_PASS_VERTICES:
      ply_bake_write_vertices(st, pc, indices, segments);
      return st->w.result == AK_OK;
    case PLY_EXP_PASS_FACES:
      ply_bake_write_faces(st, segments);
      return st->w.result == AK_OK;
    case PLY_EXP_PASS_EDGES:
      st->vertexCursor += vertexCount;
      return true;
    default:
      return false;
  }
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
               AkInstanceGeometry * __restrict inst,
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
                                                          inst,
                                                          &pc->reusableVertexCount);

  glm_mat4_copy(world, pc->world);
  glm_mat4_inv((vec4 *)world, pc->normalMatrix);
  glm_mat4_transpose(pc->normalMatrix);
  pc->mirrored = mirrored;
  pc->flipTextureV = st->doc && st->doc->inf && st->doc->inf->flipImage;
  if (st->hasColors) {
    pc->hasMaterialColor = ply_resolved_material_color(prim,
                                                        inst,
                                                        &pc->materialColor);
    if (st->bakeTextures
        && st->hasTextureColors
        && ply_primitive_base_color_texture(prim,
                                            inst,
                                            &pc->baseColorTexture,
                                            &pc->baseColorImage,
                                            &pc->sampleTexInput)) {
      pc->hasSampleTexRows = ply_rows_init(&pc->sampleTexRows,
                                            pc->sampleTexInput->accessor);
      if (!pc->hasSampleTexRows)
        goto fail;
      pc->textureDetail = ply_image_detail(st, pc->baseColorImage);
      pc->bakeTexture = true;
      pc->reuseVertices = false;
    }
  }

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
  ply_rows_destroy(&pc->sampleTexRows);
  ply_rows_destroy(&pc->colorRows);
  ply_rows_destroy(&pc->texRows);
  ply_rows_destroy(&pc->normalRows);
  ply_rows_destroy(&pc->posRows);
  return false;
}

static
void
ply_prim_end(PLYExpPrim * __restrict pc) {
  ply_rows_destroy(&pc->sampleTexRows);
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

  if (pc && pc->bakeTexture && count == 3u)
    return ply_process_baked_triangle(st, pc, indices);

  switch (st->pass) {
    case PLY_EXP_PASS_PLAN:
      return true;
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
    case PLY_EXP_PASS_EDGES:
      if (!pc || !pc->reuseVertices)
        st->vertexCursor += count;
      return true;
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
    case PLY_EXP_PASS_PLAN:
      return true;
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
      if (!pc || !pc->reuseVertices)
        st->vertexCursor += 2u;
      return true;
    case PLY_EXP_PASS_EDGES:
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
    case PLY_EXP_PASS_PLAN:
      return true;
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
    case PLY_EXP_PASS_EDGES:
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

      if (st->triangulated || (pc && pc->bakeTexture)) {
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

      if (st->triangulated || (pc && pc->bakeTexture)) {
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
                    AkInstanceGeometry * __restrict inst,
                    mat4                         world,
                    bool                         mirrored) {
  PLYExpPrim pc;
  bool       ok;
  bool       pcInitialized;

  if (!prim)
    return true;

  if (prim->type != AK_PRIMITIVE_TRIANGLES
      && prim->type != AK_PRIMITIVE_POLYGONS
      && prim->type != AK_PRIMITIVE_LINES
      && prim->type != AK_PRIMITIVE_POINTS)
    return true;

  if (st->pass == PLY_EXP_PASS_DISCOVER) {
    AkInput *posInput;

    posInput = io_primitive_find_input(prim, AK_INPUT_POSITION);
    if (ply_input_valid(posInput, 3u))
      ply_note_optional_inputs(st, prim, inst);
    return true;
  }

  pcInitialized = false;
  if (st->pass == PLY_EXP_PASS_COUNT && !st->bakeTextures) {
    AkInput *posInput;

    posInput = io_primitive_find_input(prim, AK_INPUT_POSITION);
    if (!ply_input_valid(posInput, 3u))
      return true;

    memset(&pc, 0, sizeof(pc));
    pc.prim = prim;
    pc.posInput = posInput;
    pc.reuseVertices = ply_primitive_reusable_vertex_count(st,
                                                           prim,
                                                           inst,
                                                           &pc.reusableVertexCount);
    if (pc.reuseVertices
        && !ply_count_add(&st->vertexCount, pc.reusableVertexCount))
      return false;
  } else if (!ply_prim_begin(st, &pc, prim, inst, world, mirrored)) {
    return false;
  } else {
    pcInitialized = true;
    if (st->pass == PLY_EXP_PASS_COUNT
        && pc.reuseVertices
        && !ply_count_add(&st->vertexCount, pc.reusableVertexCount)) {
      ply_prim_end(&pc);
      return false;
    }
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

  if (pcInitialized)
    ply_prim_end(&pc);

  if (ok
      && (st->pass == PLY_EXP_PASS_FACES || st->pass == PLY_EXP_PASS_EDGES)
      && pc.reuseVertices)
    st->vertexCursor += pc.reusableVertexCount;

  return ok && st->w.result == AK_OK;
}

static
bool
ply_write_mesh_instance(PLYExpState * __restrict st,
                        AkGeometry  * __restrict geom,
                        AkInstanceGeometry * __restrict inst,
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
    if (!ply_write_primitive(st, prim, inst, world, mirrored))
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

    if (!ply_write_mesh_instance(st, geom, inst, world))
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
  mat4 root;

  io_export_canonical_root(st->doc, root);
  if (st->doc->scene && st->doc->scene->node)
    return ply_write_node(st, st->doc->scene->node, root, 0u);

  return true;
}

static
bool
ply_write_library_fallback(PLYExpState * __restrict st) {
  AkGeometry *geom;
  mat4        root;

  if (st->passObjectCount > 0)
    return true;

  io_export_canonical_root(st->doc, root);
  for (geom = st->doc->lib.geometries.first; geom; geom = geom->next) {
    if (!ply_write_mesh_instance(st, geom, NULL, root))
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

  if (st->bakeTextures) {
    if (st->hasTextureColors)
      ply_w_lit(&st->w, "comment texture_bake enabled\n");
    else
      ply_w_lit(&st->w, "comment texture_bake enabled_no_textures\n");

    if (st->bakeDesiredExtraFaces > PLY_EXP_BAKE_MAX_EXTRA_FACES) {
      ply_w_lit(&st->w,
                "comment texture_bake_budget_limited max_extra_faces ");
      ply_w_u32_ascii(&st->w, (uint32_t)PLY_EXP_BAKE_MAX_EXTRA_FACES);
      ply_w_ch(&st->w, '\n');
    }
  }

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
    if (st->colorSrgb) {
      ply_w_lit(&st->w,
                "property uchar red\n"
                "property uchar green\n"
                "property uchar blue\n");
      if (st->hasAlpha)
        ply_w_lit(&st->w, "property uchar alpha\n");
    } else {
      ply_w_lit(&st->w,
                "property float red\n"
                "property float green\n"
                "property float blue\n");
      if (st->hasAlpha)
        ply_w_lit(&st->w, "property float alpha\n");
    }
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
  st.bakeTextures = st.wantColors
                    && ak_opt_get(AK_OPT_PLY_EXPORT_BAKE_TEXTURES) != 0;
  st.bakeExtraScale = 1.0;

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

  /*
   * The output vertex layout is global. Discover it before counting so an
   * early primitive cannot be counted as reusable before a later primitive
   * enables normals, UVs, colors, or alpha for the whole file.
   */
  st.pass = PLY_EXP_PASS_DISCOVER;
  st.passObjectCount = 0;
  if (!ply_write_scene(&st) || !ply_write_library_fallback(&st)) {
    result = AK_ERR;
    goto done;
  }

  if (st.bakeTextures && st.hasTextureColors) {
    st.pass = PLY_EXP_PASS_PLAN;
    st.passObjectCount = 0;
    if (!ply_write_scene(&st) || !ply_write_library_fallback(&st)) {
      result = AK_ERR;
      goto done;
    }

    if (st.bakeDesiredExtraFaces > PLY_EXP_BAKE_MAX_EXTRA_FACES) {
      st.bakeExtraScale = (double)PLY_EXP_BAKE_MAX_EXTRA_FACES
                          / (double)st.bakeDesiredExtraFaces;
    }
  }

  st.pass = PLY_EXP_PASS_COUNT;
  st.bakeExtraRemaining = PLY_EXP_BAKE_MAX_EXTRA_FACES;
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
  st.bakeExtraRemaining = PLY_EXP_BAKE_MAX_EXTRA_FACES;
  st.vertexCursor = 0;
  st.passObjectCount = 0;
  if (!ply_write_scene(&st) || !ply_write_library_fallback(&st)) {
    result = AK_ERR;
    goto done;
  }

  st.pass         = PLY_EXP_PASS_FACES;
  st.bakeExtraRemaining = PLY_EXP_BAKE_MAX_EXTRA_FACES;
  st.vertexCursor = 0;
  st.passObjectCount = 0;
  if (!ply_write_scene(&st) || !ply_write_library_fallback(&st)) {
    result = AK_ERR;
    goto done;
  }

  if (st.edgeCount > 0) {
    st.pass         = PLY_EXP_PASS_EDGES;
    st.bakeExtraRemaining = PLY_EXP_BAKE_MAX_EXTRA_FACES;
    st.vertexCursor = 0;
    st.passObjectCount = 0;
    if (!ply_write_scene(&st) || !ply_write_library_fallback(&st)) {
      result = AK_ERR;
      goto done;
    }
  }

  if (!ply_writer_end(&st.w))
    result = AK_ERR;

done:
  free(st.bakeVertexBuffer);
  if (fclose(file) != 0 && result == AK_OK)
    result = AK_ERR;

  if (result != AK_OK)
    remove(filepath);

  return result;
}
