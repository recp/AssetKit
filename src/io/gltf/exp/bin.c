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

#include "bin.h"

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define GLTF_EXP_FILE_BUFFER_SIZE (1024u * 1024u)

static
void
gltf_configure_file_buffer(FILE * __restrict file) {
  if (file)
    (void)setvbuf(file, NULL, _IOFBF, GLTF_EXP_FILE_BUFFER_SIZE);
}

static
bool
gltf_write_file_payload(FILE       * __restrict dst,
                        const char * __restrict path,
                        size_t                  expectedLen) {
  unsigned char buf[64 * 1024];
  FILE         *src;
  size_t        total;
  size_t        nread;
  bool          ok;

  if (!dst || !path || expectedLen == 0)
    return false;

  src = fopen(path, "rb");
  if (!src)
    return false;
  gltf_configure_file_buffer(src);

  ok    = true;
  total = 0;
  while ((nread = fread(buf, 1, sizeof(buf), src)) > 0) {
    if (expectedLen - total < nread
        || fwrite(buf, 1, nread, dst) != nread) {
      ok = false;
      break;
    }
    total += nread;
  }

  if (ferror(src) || total != expectedLen)
    ok = false;

  fclose(src);

  return ok;
}

static
char*
gltf_bin_path(const char * __restrict filepath) {
  const char *slash;
  const char *dot;
  char       *path;
  size_t      len;
  size_t      stemLen;

  len   = strlen(filepath);
  slash = strrchr(filepath, '/');
  dot   = strrchr(filepath, '.');

  if (dot && slash && dot < slash)
    dot = NULL;

  stemLen = dot ? (size_t)(dot - filepath) : len;
  if (stemLen > (size_t)-1 - 5u)
    return NULL;

  path    = malloc(stemLen + 5);
  if (!path)
    return NULL;

  memcpy(path, filepath, stemLen);
  memcpy(path + stemLen, ".bin", 5);

  return path;
}

static
bool
gltf_bin_uri(GLTFExpState * __restrict st) {
  static const char hex[] = "0123456789ABCDEF";
  const char       *base;
  const char       *slash;
  char             *uri;
  size_t            len;
  size_t            i;
  size_t            j;

  slash = strrchr(st->binPath, '/');
  base  = slash ? slash + 1 : st->binPath;
  len   = strlen(base);

  if (len > ((size_t)-1 - 1u) / 3u)
    return false;

  uri   = malloc(len * 3 + 1);
  if (!uri)
    return false;

  for (i = 0, j = 0; i < len; i++) {
    unsigned char c;

    c = (unsigned char)base[i];
    if ((c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '-' || c == '_' || c == '.' || c == '~') {
      uri[j++] = (char)c;
    } else {
      uri[j++] = '%';
      uri[j++] = hex[c >> 4];
      uri[j++] = hex[c & 0x0f];
    }
  }

  uri[j]     = '\0';
  st->binUri = uri;

  return st->binUri != NULL;
}

static
bool
gltf_write_zeroes(FILE * __restrict file, size_t count) {
  static const unsigned char zeroes[4] = {0, 0, 0, 0};

  return count == 0 || (file && fwrite(zeroes, 1, count, file) == count);
}

static
double
gltf_component_value(const unsigned char * __restrict src, AkTypeId type) {
  switch (type) {
    case AKT_BYTE: {
      int8_t val;
      memcpy(&val, src, sizeof(val));
      return val;
    }
    case AKT_UBYTE: {
      uint8_t val;
      memcpy(&val, src, sizeof(val));
      return val;
    }
    case AKT_SHORT: {
      int16_t val;
      memcpy(&val, src, sizeof(val));
      return val;
    }
    case AKT_USHORT: {
      uint16_t val;
      memcpy(&val, src, sizeof(val));
      return val;
    }
    case AKT_UINT: {
      uint32_t val;
      memcpy(&val, src, sizeof(val));
      return val;
    }
    case AKT_FLOAT: {
      float val;
      memcpy(&val, src, sizeof(val));
      return val;
    }
    default: break;
  }

  return 0.0;
}

static
bool
gltf_accessor_buffer_range_ok(AkAccessor * __restrict acc,
                              size_t                  fillSize,
                              size_t                  stride) {
  size_t lastOffset;
  size_t endOffset;

  if (!acc || !acc->buffer)
    return false;

  if (acc->byteOffset > acc->buffer->length)
    return false;

  if (acc->count == 0)
    return false;

  if ((size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;

  lastOffset = acc->byteOffset + (size_t)(acc->count - 1u) * stride;
  if (fillSize > (size_t)-1 - lastOffset)
    return false;

  endOffset = lastOffset + fillSize;

  return endOffset <= acc->buffer->length;
}

static
void
gltf_accessor_minmax_sample(GLTFExpAccessorOut * __restrict out,
                            const unsigned char * __restrict src,
                            size_t                           compByteSize,
                            uint32_t                         componentCount,
                            AkTypeId                         componentType,
                            bool                             first) {
  uint32_t i;

  for (i = 0; i < componentCount && i < AK_ARRAY_LEN(out->min); i++) {
    double val;

    val = gltf_component_value(src + (size_t)i * compByteSize, componentType);
    if (first || val < out->min[i])
      out->min[i] = val;
    if (first || val > out->max[i])
      out->max[i] = val;
  }
}

static
bool
gltf_write_normalized_vec3(FILE                * __restrict file,
                           const unsigned char * __restrict src,
                           size_t                           stride,
                           uint32_t                         count,
                           GLTFExpAccessorOut * __restrict out,
                           bool                             computeMinMax) {
  const float unitEpsilon = 1.0e-5f;
  uint32_t i;

  for (i = 0; i < count; i++) {
    const unsigned char *item;
    float                vec[3];
    float                len2;
    float                invLen;

    item = src + (size_t)i * stride;
    memcpy(&vec[0], item, sizeof(float));
    memcpy(&vec[1], item + sizeof(float), sizeof(float));
    memcpy(&vec[2], item + sizeof(float) * 2u, sizeof(float));

    len2 = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
    if (!isfinite(len2) || len2 <= 0.00000001f)
      return false;

    if (fabsf(len2 - 1.0f) > unitEpsilon) {
      invLen = 1.0f / sqrtf(len2);
      vec[0] *= invLen;
      vec[1] *= invLen;
      vec[2] *= invLen;
    }

    if (file && fwrite(vec, 1, sizeof(vec), file) != sizeof(vec))
      return false;

    if (computeMinMax) {
      uint32_t c;

      for (c = 0; c < 3; c++) {
        if (i == 0 || vec[c] < out->min[c])
          out->min[c] = vec[c];
        if (i == 0 || vec[c] > out->max[c])
          out->max[c] = vec[c];
      }
    }
  }

  if (computeMinMax) {
    out->minMaxCount = 3;
    out->hasMinMax   = count > 0;
  }

  return true;
}

static
bool
gltf_write_indices_as(FILE               * __restrict file,
                      const AkIndexArray * __restrict indices,
                      AkTypeId                        componentType) {
  unsigned char buf[64 * 1024];
  size_t        elemSize;
  size_t        capacity;
  size_t        offset;

  elemSize = ak_indexComponentSize(componentType);
  if (!indices || elemSize == 0)
    return false;

  if (indices->componentType == componentType) {
    size_t byteLength;

    if (indices->count > (size_t)-1 / elemSize)
      return false;

    byteLength = indices->count * elemSize;
    return !file || fwrite(indices->items, 1, byteLength, file) == byteLength;
  }

  capacity = sizeof(buf) / elemSize;
  if (capacity == 0)
    return false;

  for (offset = 0; offset < indices->count; offset += capacity) {
    size_t count;
    size_t i;

    count = indices->count - offset;
    if (count > capacity)
      count = capacity;

    switch (componentType) {
      case AKT_UBYTE: {
        uint8_t *dst;

        dst = (uint8_t *)buf;
        for (i = 0; i < count; i++)
          dst[i] = (uint8_t)ak_indexArrayGet(indices, offset + i);
        break;
      }
      case AKT_USHORT: {
        uint16_t *dst;

        dst = (uint16_t *)buf;
        for (i = 0; i < count; i++)
          dst[i] = (uint16_t)ak_indexArrayGet(indices, offset + i);
        break;
      }
      case AKT_UINT: {
        uint32_t *dst;

        dst = (uint32_t *)buf;
        for (i = 0; i < count; i++)
          dst[i] = (uint32_t)ak_indexArrayGet(indices, offset + i);
        break;
      }
      default:
        return false;
    }

    if (file && fwrite(buf, elemSize, count, file) != count)
      return false;
  }

  return true;
}

static
bool
gltf_write_accessor_data(FILE * __restrict file,
                         GLTFExpAccessorOut * __restrict out,
                         size_t * __restrict cursor) {
  size_t align;

  align = (*cursor) & 3u;
  if (align) {
    align = 4u - align;
    if (*cursor > SIZE_MAX - align)
      return false;
    if (file && !gltf_write_zeroes(file, align))
      return false;
    *cursor += align;
  }

  out->byteOffset = *cursor;

  if (out->kind == GLTF_EXP_ACCESSOR_ASSETKIT) {
    AkAccessor          *acc;
    const unsigned char *src;
    size_t               fillSize;
    size_t               stride;
    bool                 computeMinMax;

    acc      = out->accessor;
    fillSize = acc->fillByteSize;
    if (!fillSize)
      fillSize = (size_t)acc->bytesPerComponent * acc->componentCount;

    stride = acc->byteStride ? acc->byteStride : fillSize;

    if (!acc->buffer || !acc->buffer->data || fillSize == 0 || stride == 0)
      return false;

    if (!gltf_accessor_buffer_range_ok(acc, fillSize, stride))
      return false;

    src = (const unsigned char *)acc->buffer->data + acc->byteOffset;
    if ((size_t)acc->count > (size_t)-1 / fillSize)
      return false;

    out->byteLength = (size_t)acc->count * fillSize;
    computeMinMax   = out->minMaxRequired
                      && acc->componentCount > 0
                      && acc->componentCount <= AK_ARRAY_LEN(out->min)
                      && acc->bytesPerComponent > 0;
    if (out->minMaxRequired && !computeMinMax)
      return false;

    if (out->normalizeVec3) {
      if (acc->componentType != AKT_FLOAT
          || acc->componentCount != 3
          || acc->bytesPerComponent != sizeof(float)
          || fillSize != sizeof(float) * 3u)
        return false;
      if (!gltf_write_normalized_vec3(file,
                                      src,
                                      stride,
                                      acc->count,
                                      out,
                                      computeMinMax))
        return false;
    } else if (stride == fillSize && !computeMinMax) {
      if (file && fwrite(src, 1, out->byteLength, file) != out->byteLength)
        return false;
    } else {
      uint32_t i;

      for (i = 0; i < acc->count; i++) {
        const unsigned char *item;

        item = src + (size_t)i * stride;
        if (computeMinMax) {
          gltf_accessor_minmax_sample(out,
                                      item,
                                      acc->bytesPerComponent,
                                      acc->componentCount,
                                      acc->componentType,
                                      i == 0);
        }

        if (file && fwrite(item, 1, fillSize, file) != fillSize)
          return false;
      }
    }

    if (computeMinMax) {
      out->minMaxCount = acc->componentCount;
      out->hasMinMax   = acc->count > 0;
    }
  } else if (out->kind == GLTF_EXP_ACCESSOR_INDEX_ARRAY) {
    AkIndexArray *indices;
    AkTypeId      componentType;
    size_t        elemSize;

    indices       = out->indices;
    componentType = out->indexComponentType
                    ? out->indexComponentType
                    : indices->componentType;
    elemSize      = ak_indexComponentSize(componentType);
    if (elemSize == 0)
      return false;

    if (indices->count == 0 || indices->count > (size_t)-1 / elemSize)
      return false;

    out->byteLength = indices->count * elemSize;
    if (!gltf_write_indices_as(file, indices, componentType))
      return false;
  } else if (out->kind == GLTF_EXP_ACCESSOR_RAW_FILE_VIEW) {
    if (!out->rawPath || out->rawByteLength == 0)
      return false;

    out->byteLength = out->rawByteLength;
    if (file && !gltf_write_file_payload(file, out->rawPath, out->byteLength))
      return false;
  } else {
    if (!out->rawData || out->rawByteLength == 0)
      return false;

    out->byteLength = out->rawByteLength;
    if (file && fwrite(out->rawData, 1, out->byteLength, file) != out->byteLength)
      return false;
  }

  if (*cursor > SIZE_MAX - out->byteLength)
    return false;

  *cursor += out->byteLength;

  return true;
}

bool
gltf_write_bin_payload(GLTFExpState * __restrict st,
                       FILE         * __restrict file) {
  size_t i;
  size_t cursor;

  cursor = 0;
  for (i = 0; i < st->accessors.count; i++) {
    if (!gltf_write_accessor_data(file, &st->accessors.items[i], &cursor))
      return false;
  }

  st->binByteLength = cursor;

  return true;
}

AkResult
gltf_prepare_bin(GLTFExpState * __restrict st) {
  if (st->accessors.count == 0)
    return AK_OK;

  return gltf_write_bin_payload(st, NULL) ? AK_OK : AK_ERR;
}

AkResult
gltf_write_bin(GLTFExpState * __restrict st,
               const char   * __restrict filepath) {
  FILE  *file;

  if (st->accessors.count == 0)
    return AK_OK;

  st->binPath = gltf_bin_path(filepath);
  if (!st->binPath || !gltf_bin_uri(st))
    return AK_ERR;

  if (strcmp(st->binPath, filepath) == 0)
    return AK_ERR;

  file = fopen(st->binPath, "wb");
  if (!file)
    return AK_EBADF;
  gltf_configure_file_buffer(file);

  if (!gltf_write_bin_payload(st, file)) {
    fclose(file);
    remove(st->binPath);
    return AK_ERR;
  }

  if (fclose(file) != 0) {
    remove(st->binPath);
    return AK_ERR;
  }

  return AK_OK;
}
