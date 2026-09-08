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

#include "lgsc.h"
#include <libdeflate.h>
#include <math.h>

/* L-GSC v1.0: a plain header and two gzip members, each length-prefixed.
   Attribute order is INRIA: XYZ, DC[3], rest[45], opacity, scale[3], WXYZ.
   USE_GZIP and SEPARATE_GEOM_ATTR_CODING are not serialized by v1.0.
   The interoperable bitstream uses the reference decoder's defaults: both on. */
enum {
  LGSC_FIELDS    = 59,
  LGSC_HEADER    = 4 + 1 + 10 + LGSC_FIELDS * 8,
  LGSC_ROT_PACK  = 0x40,
  LGSC_SIGMOID   = 0x20,
  LGSC_SH_RGB    = 0x10,
  LGSC_POS_ROWS  = 0x08,
  LGSC_ATTR_ROWS = 0x04,
  LGSC_SH_YUV    = 0x02
};

typedef struct LGSCColumn {
  const uint8_t *data;
  size_t         stride;
  double         low;
  double         step;
  uint32_t       limit;
  uint8_t        width;
} LGSCColumn;

static inline
uint32_t
lgsc_u32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8
         | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static inline
float
lgsc_f32(const uint8_t *p) {
  uint32_t bits;
  float    value;

  bits = lgsc_u32(p);
  memcpy(&value, &bits, sizeof(value));
  return value;
}

static
bool
lgsc_segment(const uint8_t **cursor, size_t *remaining,
              const uint8_t **data, size_t *length) {
  if (*remaining < 4)
    return false;

  *length = lgsc_u32(*cursor);
  if (*length > *remaining - 4)
    return false;

  *data       = *cursor + 4;
  *cursor    += 4 + *length;
  *remaining -= 4 + *length;
  return true;
}

static
bool
lgsc_header(const uint8_t *header, size_t size, LGSCColumn columns[LGSC_FIELDS],
             uint32_t *count, uint8_t *flags, size_t *geomSize, size_t *attrSize) {
  static const uint8_t bitSlot[] = {0, 4, 5, 3, 1, 2};
  static const uint8_t groupSize[] = {3, 3, 45, 1, 3, 4};
  const uint8_t *params, *bounds;
  double         low, high;
  size_t         geomBytes, attrBytes;
  uint32_t       group, dim, field, bits, divisor;

  if (size != LGSC_HEADER || header[4] != 10)
    return false;

  *count = lgsc_u32(header);
  params = header + 5;
  bounds = params + 10;
  *flags = params[8];

  if (!*count || *count > INT32_MAX || (*flags & 0x81))
    return false;

  for (field = 0; field < 8; field++)
    if (!params[field] || params[field] > 24)
      return false;

  field     = 0;
  geomBytes = 0;
  attrBytes = 0;

  for (group = 0; group < 6; group++) {
    for (dim = 0; dim < groupSize[group]; dim++, field++, bounds += 8) {
      bits = params[group == 2 ? (dim < 9 ? 5 : dim < 24 ? 6 : 7) : bitSlot[group]];
      low  = lgsc_f32(bounds);
      high = lgsc_f32(bounds + 4);

      if (!isfinite(low) || !isfinite(high) || low > high)
        return false;

      divisor              = (1u << bits) - (group != 2);
      columns[field].low   = low;
      columns[field].step  = (high - low) / divisor;
      columns[field].limit = (1u << bits) - 1;
      columns[field].width = (uint8_t)((bits + 7) / 8);

      if (field == 58 && (*flags & LGSC_ROT_PACK)) {
        columns[field].low   = 0;
        columns[field].step  = 1;
        columns[field].limit = 3;
      }

      if (!group)
        geomBytes += columns[field].width;
      else
        attrBytes += columns[field].width;
    }
  }

  if (*count > SIZE_MAX / attrBytes || *count > SIZE_MAX / geomBytes)
    return false;

  *geomSize = (size_t)*count * geomBytes;
  *attrSize = (size_t)*count * attrBytes;
  return true;
}

static
void
lgsc_columns(LGSCColumn *columns, uint32_t dimensions, uint32_t count,
               const uint8_t *data, bool rows) {
  size_t   offset, stride;
  uint32_t dim;

  offset = 0;
  stride = 0;

  for (dim = 0; dim < dimensions; dim++)
    stride += columns[dim].width;

  for (dim = 0; dim < dimensions; dim++) {
    columns[dim].data   = data + offset;
    columns[dim].stride = rows ? stride : columns[dim].width;
    offset            += columns[dim].width * (rows ? 1 : (size_t)count);
  }
}

/* Select byte width outside the point loop. Each column has one affine
   dequantizer; no attribute lookup or allocation happens per point. */
static
bool
lgsc_column(const LGSCColumn *column, float *dest, size_t stride, uint32_t count) {
  const uint8_t *p;
  size_t         sourceStride;
  double         low, step;
  uint32_t       i, value, largest;

  p            = column->data;
  sourceStride = column->stride;
  low          = column->low;
  step         = column->step;
  largest      = 0;

#define LGSC_UNPACK(SYMBOL)                                                     \
  for (i = 0; i < count; i++, p += sourceStride, dest += stride) {               \
    value    = (SYMBOL);                                                       \
    largest |= value;                                                          \
    *dest    = (float)(low + value * step);                                     \
  }

  switch (column->width) {
    case 1: LGSC_UNPACK((uint32_t)p[0]); break;
    case 2: LGSC_UNPACK((uint32_t)p[0] | (uint32_t)p[1] << 8); break;
    case 3: LGSC_UNPACK((uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16); break;
    default: return false;
  }

#undef LGSC_UNPACK

  return largest <= column->limit;
}

static
bool
lgsc_group(LGSCColumn *columns, uint32_t dimensions, uint32_t count, float *dest) {
  uint32_t dim;

  for (dim = 0; dim < dimensions; dim++)
    if (!lgsc_column(&columns[dim], dest + dim, dimensions, count))
      return false;

  return true;
}

static
bool
lgsc_sh(LGSCColumn *columns, uint32_t count, uint32_t degree,
         uint8_t flags, bool rdf, float *dest) {
  LGSCColumn blockColumn;
  float      block[32][45], *row;
  float      y, u, v, value;
  uint32_t   first, rows, dim, i, coefficient, channel, index, width;

  width = degree * (degree + 2);

  /* Bounded tile keeps even dimension-major streams cache-local. It also
     handles the YUV + rearranged combination without a full-size float copy. */
  for (first = 0; first < count; first += rows) {
    rows = count - first < 32 ? count - first : 32;

    for (dim = 0; dim < 45; dim++) {
      blockColumn       = columns[dim];
      blockColumn.data += (size_t)first * blockColumn.stride;

      if (!lgsc_column(&blockColumn, &block[0][dim], 45, rows))
        return false;
    }

    for (i = 0; i < rows; i++) {
      row = block[i];

      if (flags & LGSC_SH_YUV) {
        for (dim = 0; dim < 15; dim++) {
          y             = row[dim];
          u             = row[dim + 15];
          v             = row[dim + 30];
          row[dim]      = (float)(y + 1.5748 * v);
          row[dim + 15] = (float)(y - 0.1873 * u - 0.4681 * v);
          row[dim + 30] = (float)(y + 1.8556 * u);
        }
      }

      for (coefficient = 0; coefficient < width; coefficient++) {
        for (channel = 0; channel < 3; channel++) {
          index = flags & LGSC_SH_RGB ? coefficient * 3 + channel : channel * 15 + coefficient;
          value = row[index];

          if (!isfinite(value))
            return false;

          /* INRIA RDF -> glTF LUF rotates 180 degrees around Z: (-1)^m. */
          if (rdf && ((coefficient + 1) & 1))
            value = -value;

          *dest++ = value;
        }
      }
    }
  }

  return true;
}

static
bool
lgsc_finish(float *positions, float *rotations, float *scales, float *opacity,
              uint32_t count, uint8_t flags, bool rdf) {
  double   norm;
  float    q[4], missing, value;
  uint32_t i, dim, largest, source;

  for (i = 0; i < count; i++, positions += 3, rotations += 4, scales += 3) {
    memcpy(q, rotations, sizeof(q));

    if (flags & LGSC_ROT_PACK) {
      largest = (uint32_t)q[3];
      missing = sqrtf(fmaxf(0, 1 - (q[0] * q[0] + q[1] * q[1] + q[2] * q[2])));
      source  = 3;

      for (dim = 4; dim-- > 0;)
        q[dim] = dim == largest ? missing : q[--source];
    }

    norm = (double)q[0] * q[0] + (double)q[1] * q[1]
           + (double)q[2] * q[2] + (double)q[3] * q[3];

    if (!(norm > 0) || !isfinite(norm))
      return false;

    norm         = 1 / sqrt(norm);
    rotations[0] = (float)(q[1] * norm);
    rotations[1] = (float)(q[2] * norm);
    rotations[2] = (float)(q[3] * norm);
    rotations[3] = (float)(q[0] * norm);

    if (rdf) {
      positions[0] = -positions[0];
      positions[1] = -positions[1];
      rotations[0] = -rotations[0];
      rotations[1] = -rotations[1];
    }

    for (dim = 0; dim < 3; dim++) {
      scales[dim] = expf(scales[dim]);

      if (!isfinite(scales[dim]))
        return false;
    }

    value = opacity[i];
    if (flags & LGSC_SIGMOID) {
      /* Reference decode logit followed by AssetKit sigmoid cancels out. */
      opacity[i] = fminf(1 - 1e-6f, fmaxf(1e-6f, value));
    } else if (value >= 0) {
      opacity[i] = 1 / (1 + expf(-value));
    } else {
      value      = expf(value);
      opacity[i] = value / (1 + value);
    }
  }

  return true;
}

AK_HIDE
bool
lgsc_decode(AkHeap *heap, void *parent, const uint8_t *bytes, size_t size,
            uint32_t degree, uint32_t expectedCount, bool rdf, AkLGSCData *out) {
  struct libdeflate_decompressor *decompressor;
  const uint8_t                 *header, *geometry, *attributes;
  uint8_t                       *scratch;
  float                         *values, *positions, *rotations, *scales, *opacity, *dc, *sh;
  AkBuffer                      *buffer;
  LGSCColumn                     columns[LGSC_FIELDS];
  size_t                         headerSize, geometrySize, attributesSize, geomBytes, attrBytes;
  size_t                         consumed, written, offset, groupBytes;
  uint32_t                       count, width, group, dim, field, i;
  uint8_t                        flags;
  bool                           ok;
  static const uint8_t groupSize[] = {3, 45, 1, 3, 4};

  if (!heap || !parent || !bytes || !out || degree > 3
      || !lgsc_segment(&bytes, &size, &header, &headerSize)
      || !lgsc_header(header, headerSize, columns, &count, &flags, &geomBytes, &attrBytes)
      || (expectedCount && count != expectedCount)
      || !lgsc_segment(&bytes, &size, &geometry, &geometrySize)
      || !lgsc_segment(&bytes, &size, &attributes, &attributesSize)
      || size || geometrySize < 18 || attributesSize < 18)
    return false;

  /* Check the gzip length hints before allocating; decompression still
     verifies the full sizes, stream consumption, and CRCs below. */
  if (lgsc_u32(geometry + geometrySize - 4) != (uint32_t)geomBytes
      || lgsc_u32(attributes + attributesSize - 4) != (uint32_t)attrBytes)
    return false;

  width = degree * (degree + 2);
  if (count > SIZE_MAX / sizeof(float) / (14 + width * 3))
    return false;

  decompressor = NULL;
  scratch      = NULL;
  buffer       = NULL;
  ok           = false;

  if (!(decompressor = libdeflate_alloc_decompressor())
      || !(scratch = ak_heap_alloc(heap, parent, attrBytes))
      || !(buffer = ak_heap_calloc(heap, parent, sizeof(*buffer))))
    goto cleanup;

  buffer->length = (size_t)count * (14 + width * 3) * sizeof(float);
  if (!(buffer->data = ak_heap_alloc(heap, buffer, buffer->length)))
    goto cleanup;

  values    = buffer->data;
  positions = values;
  rotations = positions + (size_t)count * 3;
  scales    = rotations + (size_t)count * 4;
  opacity   = scales + (size_t)count * 3;
  dc        = opacity + count;
  sh        = dc + (size_t)count * 3;

  if (libdeflate_gzip_decompress_ex(decompressor, geometry, geometrySize,
                                    scratch, geomBytes, &consumed, &written) != LIBDEFLATE_SUCCESS
      || consumed != geometrySize || written != geomBytes)
    goto cleanup;

  lgsc_columns(columns, 3, count, scratch, (flags & LGSC_POS_ROWS) != 0);
  if (!lgsc_group(columns, 3, count, positions))
    goto cleanup;

  if (libdeflate_gzip_decompress_ex(decompressor, attributes, attributesSize,
                                    scratch, attrBytes, &consumed, &written) != LIBDEFLATE_SUCCESS
      || consumed != attributesSize || written != attrBytes)
    goto cleanup;

  offset = 0;
  field  = 3;

  for (group = 0; group < 5; group++) {
    groupBytes = 0;

    for (dim = 0; dim < groupSize[group]; dim++)
      groupBytes += columns[field + dim].width;

    lgsc_columns(columns + field, groupSize[group], count, scratch + offset,
                   (flags & LGSC_ATTR_ROWS) != 0);
    offset += (size_t)count * groupBytes;
    field  += groupSize[group];
  }

  if (!lgsc_group(columns + 3, 3, count, dc)
      || !lgsc_sh(columns + 6, count, degree, flags, rdf, sh)
      || !lgsc_group(columns + 51, 1, count, opacity)
      || !lgsc_group(columns + 52, 3, count, scales)
      || !lgsc_group(columns + 55, 4, count, rotations)
      || !lgsc_finish(positions, rotations, scales, opacity, count, flags, rdf))
    goto cleanup;

  memset(out, 0, sizeof(*out));
  out->buffer = buffer;
  out->count  = count;
  out->degree = degree;

  out->offsets[1] = (size_t)count * 3 * sizeof(float);
  out->offsets[2] = (size_t)count * 7 * sizeof(float);
  out->offsets[3] = (size_t)count * 10 * sizeof(float);
  out->offsets[4] = (size_t)count * 11 * sizeof(float);
  out->strides[0] = 3 * sizeof(float);
  out->strides[1] = 4 * sizeof(float);
  out->strides[2] = 3 * sizeof(float);
  out->strides[3] = sizeof(float);
  out->strides[4] = 3 * sizeof(float);

  for (i = 0; i < width; i++) {
    out->offsets[5 + i] = ((size_t)count * 14 + i * 3) * sizeof(float);
    out->strides[5 + i] = width * 3 * sizeof(float);
  }

  ok = true;

cleanup:
  if (decompressor)
    libdeflate_free_decompressor(decompressor);
  if (scratch)
    ak_free(scratch);
  if (!ok && buffer)
    ak_free(buffer);

  return ok;
}

AK_HIDE
void
lgsc_accessor(AkAccessor *acc, const AkLGSCData *data, uint32_t slot) {
  uint32_t components;

  components                 = slot == 1 ? 4 : slot == 3 ? 1 : 3;
  acc->buffer                = data->buffer;
  acc->byteOffset            = data->offsets[slot];
  acc->byteStride            = data->strides[slot];
  acc->byteLength            = (size_t)(data->count - 1) * acc->byteStride + components * sizeof(float);
  acc->count                 = data->count;
  acc->componentType         = AKT_FLOAT;
  acc->originalComponentType = AKT_FLOAT;
  acc->componentCount        = components;
  acc->componentSize         = (AkComponentSize)components;
  acc->bytesPerComponent     = sizeof(float);
  acc->fillByteSize          = components * sizeof(float);
  acc->normalized            = false;
  acc->originallyNormalized  = false;
}
