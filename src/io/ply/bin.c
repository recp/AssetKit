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
#include "common.h"
#include "util.h"
#include "../common/util.h"
#include "../../data.h"
#include "../../endian.h"

#if defined(__aarch64__) && defined(__ARM_NEON) \
    && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  include <arm_neon.h>
#  define PLY_BIN_HAS_NEON_RGBA 1
#else
#  define PLY_BIN_HAS_NEON_RGBA 0
#endif

#define PLY_BIN_FAST_MAX_SLOTS 16
#define PLY_FACE_INLINE_CAPACITY 64u

typedef enum PLYBinFastKind {
  PLY_BIN_FAST_NONE = 0,
  PLY_BIN_FAST_FLOAT,
  PLY_BIN_FAST_UBYTE
} PLYBinFastKind;

AK_INLINE
void
ply_bin_srgb_rgba_row(const char  * __restrict src,
                      float       * __restrict dst,
                      const float * __restrict table,
                      float                    alphaScale) {
#if PLY_BIN_HAS_NEON_RGBA
  uint8x16_t  raw;
  uint32_t    rgba;
  float32x4_t color;

  raw  = vld1q_u8((const uint8_t *)(const void *)src);
  rgba = vgetq_lane_u32(vreinterpretq_u32_u8(raw), 3);

  /* The second store replaces the four raw color bytes copied by the first.
     This turns six scalar output stores into two 128-bit stores. */
  vst1q_u8((uint8_t *)(void *)dst, raw);
  color = vdupq_n_f32((float)(rgba >> 24) * alphaScale);
  color = vsetq_lane_f32(table[rgba & 0xffu], color, 0);
  color = vsetq_lane_f32(table[(rgba >> 8) & 0xffu], color, 1);
  color = vsetq_lane_f32(table[(rgba >> 16) & 0xffu], color, 2);
  vst1q_f32(dst + 3, color);
#else
  memcpy(dst, src, sizeof(float) * 3u);
  dst[3] = table[(uint8_t)src[12]];
  dst[4] = table[(uint8_t)src[13]];
  dst[5] = table[(uint8_t)src[14]];
  dst[6] = (float)(uint8_t)src[15] * alphaScale;
#endif
}

AK_INLINE
float
ply_bin_read_f32(const char * __restrict p, bool le) {
  uint32_t bits;
  float    value;

  memcpy(&bits, p, sizeof(bits));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  if (!le)
    bits = bswapu32(bits);
#else
  if (le)
    bits = bswapu32(bits);
#endif
  memcpy(&value, &bits, sizeof(value));

  return value;
}

static
bool
ply_bin_vertex_fast_layout(PLYElement    * __restrict elem,
                           size_t                      offsets[PLY_BIN_FAST_MAX_SLOTS],
                           PLYBinFastKind              kinds[PLY_BIN_FAST_MAX_SLOTS],
                           size_t        * __restrict  inputStride) {
  PLYProperty *prop;
  uint32_t     i;
  size_t       off;

  if (!elem || elem->knownCount == 0 || elem->knownCount > PLY_BIN_FAST_MAX_SLOTS)
    return false;

  for (i = 0; i < elem->knownCount; i++) {
    offsets[i] = 0;
    kinds[i]   = PLY_BIN_FAST_NONE;
  }

  off  = 0;
  prop = elem->property;
  while (prop) {
    AkTypeDesc *typeDesc;
    size_t      size;

    if (prop->islist)
      return false;

    typeDesc = prop->typeDesc;
    if (!typeDesc)
      return false;

    size = typeDesc->size;
    if (!prop->ignore) {
      if (prop->slot >= elem->knownCount)
        return false;

      switch (typeDesc->typeId) {
        case AKT_FLOAT:
          kinds[prop->slot] = PLY_BIN_FAST_FLOAT;
          break;
        case AKT_UBYTE:
          kinds[prop->slot] = PLY_BIN_FAST_UBYTE;
          break;
        default:
          return false;
      }
      offsets[prop->slot] = off;
    }

    off += size;
    prop = prop->next;
  }

  for (i = 0; i < elem->knownCount; i++) {
    if (kinds[i] == PLY_BIN_FAST_NONE)
      return false;
  }

  *inputStride = off;
  return off > 0;
}

static
bool
ply_bin_vertex_fast(char        ** __restrict src,
                    char         * __restrict end,
                    PLYElement   * __restrict elem,
                    PLYState     * __restrict pst,
                    float        * __restrict dst,
                    uint32_t                   count,
                    uint32_t                   stride,
                    bool                       le) {
  PLYBinFastKind kinds[PLY_BIN_FAST_MAX_SLOTS];
  size_t         offsets[PLY_BIN_FAST_MAX_SLOTS];
  size_t         inputStride, i, j;
  char          *p;
  bool           nativeEndian;

  if (stride != elem->knownCount)
    return false;
  if (!ply_bin_vertex_fast_layout(elem, offsets, kinds, &inputStride))
    return false;
  if ((uint64_t)count * inputStride > (uint64_t)(end - *src))
    return false;

  p = *src;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  nativeEndian = le;
#else
  nativeEndian = !le;
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  if (le && inputStride == (size_t)stride * sizeof(float)) {
#else
  if (!le && inputStride == (size_t)stride * sizeof(float)) {
#endif
    bool allFloatContiguous;

    allFloatContiguous = true;
    for (j = 0; j < stride; j++) {
      if (kinds[j] != PLY_BIN_FAST_FLOAT || offsets[j] != j * sizeof(float)) {
        allFloatContiguous = false;
        break;
      }
    }

    if (allFloatContiguous) {
      memcpy(dst, p, (size_t)count * inputStride);
      if (pst->normalizeColors || pst->alphaBlendBitsValid) {
        for (i = 0; i < count; i++)
          ply_normalize_color_row(pst, dst + i * stride, (uint32_t)i);
      }
      *src = p + (size_t)count * inputStride;
      return true;
    }
  }

  if ((stride == 6 || stride == 7)
      && inputStride == 12u + (size_t)(stride - 3u)
      && kinds[0] == PLY_BIN_FAST_FLOAT
      && kinds[1] == PLY_BIN_FAST_FLOAT
      && kinds[2] == PLY_BIN_FAST_FLOAT
      && offsets[0] == 0
      && offsets[1] == 4
      && offsets[2] == 8) {
    bool packedColor;

    packedColor = true;
    for (j = 3; j < stride; j++) {
      if (kinds[j] != PLY_BIN_FAST_UBYTE || offsets[j] != 12u + (j - 3u)) {
        packedColor = false;
        break;
      }
    }

    if (packedColor) {
      bool packedColorNormalization;

      packedColorNormalization = pst->normalizeColors
                                 && pst->colorSlot == 3u
                                 && pst->colorComponentCount == stride - 3u;

      if (packedColorNormalization && pst->colorLookup8 && nativeEndian) {
        const float * __restrict table;

        table = ak_srgb8_to_linear_table;
        if (stride == 7u) {
          const float alphaScale = 1.0f / 255.0f;

#if defined(__clang__)
#  pragma clang loop unroll_count(8)
#elif defined(__GNUC__)
#  pragma GCC unroll 8
#endif
          for (i = 0u; i < count; i++) {
            ply_bin_srgb_rgba_row(p, dst, table, alphaScale);
            ply_record_alpha_row(pst, dst, (uint32_t)i);
            p   += 16u;
            dst += 7u;
          }
        } else {
#if defined(__clang__)
#  pragma clang loop unroll_count(8)
#elif defined(__GNUC__)
#  pragma GCC unroll 8
#endif
          for (i = 0; i < count; i++) {
            memcpy(dst, p, sizeof(float) * 3u);
            dst[3] = table[(uint8_t)p[12]];
            dst[4] = table[(uint8_t)p[13]];
            dst[5] = table[(uint8_t)p[14]];
            p   += 15u;
            dst += 6u;
          }
        }
        *src = p;
        return true;
      }

      if (packedColorNormalization && !pst->colorSrgb && nativeEndian) {
        const float scale = pst->colorScale;

        for (i = 0; i < count; i++) {
          memcpy(dst, p, sizeof(float) * 3u);
          for (j = 3; j < stride; j++)
            dst[j] = (float)(uint8_t)p[offsets[j]] * scale;
          ply_record_alpha_row(pst, dst, (uint32_t)i);
          p   += inputStride;
          dst += stride;
        }
        *src = p;
        return true;
      }

      for (i = 0; i < count; i++) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        if (le) {
#else
        if (!le) {
#endif
          memcpy(dst, p, sizeof(float) * 3u);
        } else {
          dst[0] = ply_bin_read_f32(p + 0, le);
          dst[1] = ply_bin_read_f32(p + 4, le);
          dst[2] = ply_bin_read_f32(p + 8, le);
        }
        for (j = 3; j < stride; j++)
          dst[j] = (float)*(const uint8_t *)(const void *)(p + offsets[j]);
        ply_normalize_color_row(pst, dst, (uint32_t)i);
        p   += inputStride;
        dst += stride;
      }
      *src = p;
      return true;
    }
  }

  for (i = 0; i < count; i++) {
    for (j = 0; j < stride; j++) {
      switch (kinds[j]) {
        case PLY_BIN_FAST_FLOAT:
          dst[j] = ply_bin_read_f32(p + offsets[j], le);
          break;
        case PLY_BIN_FAST_UBYTE:
          dst[j] = (float)*(const uint8_t *)(const void *)(p + offsets[j]);
          break;
        default:
          break;
      }
    }
    ply_normalize_color_row(pst, dst, (uint32_t)i);
    p   += inputStride;
    dst += stride;
  }

  *src = p;
  return true;
}

static
bool
ply_bin_skip_property(char        ** __restrict src,
                      char         * __restrict end,
                      PLYProperty  * __restrict prop,
                      bool                       le) {
  char   *p;
  AkUInt  count;
  size_t  itemSize;

  p = *src;

  if (!prop->islist) {
    if (!prop->typeDesc || p + prop->typeDesc->size > end)
      return false;

    *src = p + prop->typeDesc->size;
    return true;
  }

  if (!prop->listCountTypeDesc || !prop->typeDesc)
    return false;

  if (p + prop->listCountTypeDesc->size > end)
    return false;

  ply_val(p, prop->listCountTypeDesc, le, AkUInt, count, 0);

  itemSize = prop->typeDesc->size;
  if ((uint64_t)count * itemSize > (uint64_t)(end - p))
    return false;

  *src = p + (size_t)count * itemSize;

  return true;
}

AK_HIDE
void
ply_bin(char * __restrict src, PLYState * __restrict pst, bool le) {
  char        *p;
  float       *b;
  PLYElement  *elem;
  PLYProperty *prop;
  AkBuffer    *buff;
  char        *e;
  AkUInt       faceInline[PLY_FACE_INLINE_CAPACITY];
  AkUInt      *faceHeap;
  size_t       faceHeapCapacity;
  uint32_t     i, stride, vertcount;
  
  p         = src;
  elem      = pst->element;
  vertcount = pst->vertcount;
  e         = pst->end;
  faceHeap         = NULL;
  faceHeapCapacity = 0;

  while (elem) {
    if (elem->type == PLY_ELEM_VERTEX) {
      AkUInt elemc;
      
      elemc  = elem->count;
      buff   = elem->buff;
      b      = buff->data; /* TODO: all vertices are floats for now */
      stride = elem->knownCount;
      i      = 0;

      /* stop */
      if (!elem->buff || elem->buff->length == 0)
        goto fns;

      if (!ply_bin_vertex_fast(&p, e, elem, pst, b, elemc, stride, le)) {
        while (i < elemc) {
          prop = elem->property;
          while (prop) {
            if (!prop->ignore) {
              if (!prop->typeDesc || p + prop->typeDesc->size > e)
                goto fns;

              ply_val(p, prop->typeDesc, le, float, b[prop->slot], 0.0f);

            } else if (!ply_bin_skip_property(&p, e, prop, le)) {
              goto fns;
            }
            prop = prop->next;
          }

          ply_normalize_color_row(pst, b, i);
          b += stride;
          i++;
        }
      }
    } else if (elem->type == PLY_ELEM_FACE) {
      AkUInt *f, fc, j, count, valid, elemc;

      pst->dc_ind = ply_index_data_new_estimated(pst, (size_t)elem->count * 3u);
      pst->faceAlphaBlendCount = 0u;
      elemc       = elem->count;
      f           = faceInline;
      i           = 0;
      count       = 0;

      while (i++ < elemc) {
        prop = elem->property;
        
        /* iterate thorough list and other properties */
        while (prop) {
          if (prop->semantic == PLY_PROP_VERTEX_INDICES && prop->islist) {
            if (!prop->listCountTypeDesc || !prop->typeDesc)
              goto fns;

            if ((p + prop->listCountTypeDesc->size) > e)
              goto fns;

            ply_val(p, prop->listCountTypeDesc, le, AkUInt, fc, 0);

            if (fc == 3) {
              AkUInt f0, f1, f2;

              if ((p + prop->typeDesc->size * 3) > e)
                goto fns;

              ply_val(p, prop->typeDesc, le, uint32_t, f0, 0);
              ply_val(p, prop->typeDesc, le, uint32_t, f1, 0);
              ply_val(p, prop->typeDesc, le, uint32_t, f2, 0);

              if (f0 < vertcount && f1 < vertcount && f2 < vertcount)
                PLY_INDEX_APPEND_TRI(pst, f0, f1, f2, count);
            } else if (fc > 3) {
              if (fc <= PLY_FACE_INLINE_CAPACITY) {
                f = faceInline;
              } else {
                if ((size_t)fc > faceHeapCapacity) {
                  AkUInt *grown;

                  if ((size_t)fc > ((size_t)-1) / sizeof(*f))
                    goto fns;

                  grown = realloc(faceHeap, sizeof(*f) * (size_t)fc);
                  if (!grown)
                    goto fns;

                  faceHeap = grown;
                  faceHeapCapacity = fc;
                }
                f = faceHeap;
              }

              valid = 0;

              /* copy data */
              for (j = 0; j < fc; j++) {
                if ((p + prop->typeDesc->size) > e)
                  goto fns;

                ply_val(p, prop->typeDesc, le, uint32_t, f[j], 0);

                valid += f[j] < vertcount;
              }
              
              /* check valid loop */
              if (valid == fc) {
                PLY_INDEX_APPEND_FACE(pst, f, fc, count);
              }
            } else if (fc > 0) {
              for (j = 0; j < fc; j++)
                p += prop->typeDesc->size;
            }
            
          } else {
            if (!ply_bin_skip_property(&p, e, prop, le))
              goto fns;
          }

          prop = prop->next;
        }
      }

      pst->count = count;
    } else if (elem->type == PLY_ELEM_TRISTRIPS) {
      AkUInt elemc, fc, j, count, vertcount;

      elemc       = elem->count;
      i           = 0;
      count       = 0;
      vertcount   = pst->vertcount;

      while (i++ < elemc) {
        prop = elem->property;

        while (prop) {
          if (prop->semantic == PLY_PROP_VERTEX_INDICES && prop->islist) {
            PLYTriSeen seen;
            AkUInt prev0, prev1, stripLen;
            size_t itemSize;

            if (!prop->listCountTypeDesc || !prop->typeDesc)
              goto fns;

            if ((p + prop->listCountTypeDesc->size) > e)
              goto fns;

            ply_val(p, prop->listCountTypeDesc, le, AkUInt, fc, 0);

            itemSize = prop->typeDesc->size;
            if ((uint64_t)fc * itemSize > (uint64_t)(e - p))
              goto fns;

            if (!pst->dc_ind) {
              pst->dc_ind = ply_index_data_new_estimated(
                pst,
                fc > 2 ? ((size_t)fc - 2u) * 3u : 0);
              pst->faceAlphaBlendCount = 0u;
            }
            ply_tri_seen_init(&seen, pst, fc);
            prev0 = prev1 = 0;
            stripLen = 0;
            for (j = 0; j < fc; j++) {
              AkInt value;
              AkUInt index;

              value = ply_val_akint(&p, prop->typeDesc, le, -1);

              if (value < 0 || (AkUInt)value >= vertcount) {
                stripLen = 0;
                continue;
              }

              index = (AkUInt)value;
              if (stripLen == 0) {
                prev0 = index;
                stripLen = 1;
              } else if (stripLen == 1) {
                prev1 = index;
                stripLen = 2;
              } else {
                PLY_INDEX_APPEND_STRIP_TRI_SEEN(pst,
                                                prev0,
                                                prev1,
                                                index,
                                                stripLen,
                                                count,
                                                &seen);
                prev0 = prev1;
                prev1 = index;
                stripLen++;
              }
            }
          } else {
            if (!ply_bin_skip_property(&p, e, prop, le))
              goto fns;
          }

          prop = prop->next;
        }
      }

      pst->count = count;
    } else if (elem->type == PLY_ELEM_EDGE) {
      AkUInt elemc, vertcount;

      elemc     = elem->count;
      i         = 0;
      vertcount = pst->vertcount;

      while (i++ < elemc) {
        AkUInt v0, v1;
        bool   hasV0, hasV1;

        v0 = v1 = 0;
        hasV0 = hasV1 = false;
        prop = elem->property;

        while (prop) {
          if (!prop->islist
              && (prop->semantic == PLY_PROP_VERTEX1
                  || prop->semantic == PLY_PROP_VERTEX2)) {
            AkUInt value;

            if (!prop->typeDesc || p + prop->typeDesc->size > e)
              goto fns;

            value = ply_val_akuint(&p, prop->typeDesc, le, UINT32_MAX);

            if (prop->semantic == PLY_PROP_VERTEX1) {
              v0 = value;
              hasV0 = true;
            } else {
              v1 = value;
              hasV1 = true;
            }
          } else {
            if (!ply_bin_skip_property(&p, e, prop, le))
              goto fns;
          }

          prop = prop->next;
        }

        if (hasV0 && hasV1 && v0 < vertcount && v1 < vertcount) {
          ply_edge_append(pst, v0, v1);
        }
      }
    } else {
      /* skip unsupported elements */
      AkUInt elemc;

      elemc = elem->count;
      i     = 0;
      while (i++ < elemc) {
        prop = elem->property;
        while (prop) {
          if (!ply_bin_skip_property(&p, e, prop, le))
            goto fns;
          prop = prop->next;
        }
      }
    }
    elem = elem->next;
  }
  
fns:
  free(faceHeap);
  ply_finish(pst);
}
