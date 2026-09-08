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

/* Shared SPZ decoder for standalone files and glTF bufferViews. */
#if __has_include(<spz/load-spz.h>)
#  include <spz/load-spz.h>
#else
#  include <load-spz.h>
#endif

#include <ak/assetkit.h>
#include <cmath>
#include <cstring>

#if defined(_WIN32)
#  define AK_SPZ_EXPORT __declspec(dllexport)
#else
#  define AK_SPZ_EXPORT __attribute__((visibility("default")))
#endif

namespace {

bool
add_input(AkHeap *heap, AkMeshPrimitive *prim, AkBuffer *buffer,
          uint32_t count, uint32_t stride, uint32_t offset, uint32_t components,
          AkInputSemantic semantic, const char *name, uint32_t set = 0) {
  AkAccessor *acc;
  AkInput    *inp;

  if (!(acc = (AkAccessor *)ak_heap_calloc(heap, prim, sizeof(*acc)))
      || !(inp = (AkInput *)ak_heap_calloc(heap, prim, sizeof(*inp))))
    return false;

  acc->buffer                = buffer;
  acc->count                 = count;
  acc->byteOffset            = offset * sizeof(float);
  acc->byteStride            = stride * sizeof(float);
  acc->byteLength            = (size_t)(count - 1) * acc->byteStride + components * sizeof(float);
  acc->bytesPerComponent     = sizeof(float);
  acc->componentSize         = (AkComponentSize)components;
  acc->componentCount        = components;
  acc->componentType         = AKT_FLOAT;
  acc->originalComponentType = AKT_FLOAT;
  acc->fillByteSize          = components * sizeof(float);
  inp->accessor              = acc;
  inp->semantic              = semantic;
  inp->semanticRaw           = ak_heap_strdup(heap, inp, name);
  inp->set                   = set;
  if (!inp->semanticRaw)
    return false;

  inp->next   = prim->input;
  prim->input = inp;
  prim->inputCount++;

  if (semantic == AK_INPUT_POSITION)
    prim->pos = inp;

  return true;
}

int
decode(AkHeap *heap, AkMeshPrimitive *prim, const uint8_t *data, size_t size) {
  AkGaussianSplat   *gs;
  AkBuffer         *buffer;
  float            *values, *row;
  spz::GaussianCloud cloud;
  spz::UnpackOptions options;
  double            norm;
  uint32_t          n, shCount, stride, i, c;

  /* glTF uses LUF. libspz also rotates the quaternion and SH basis. */
  options.to = spz::CoordinateSystem::LUF;
  cloud      = spz::loadSpz(data, size, options);
  if (cloud.numPoints <= 0 || cloud.shDegree < 0 || cloud.shDegree > 4)
    return -1;

  n       = (uint32_t)cloud.numPoints;
  shCount = (uint32_t)((cloud.shDegree + 1) * (cloud.shDegree + 1));
  stride  = 11 + shCount * 3;

  if (cloud.positions.size() != (size_t)n * 3
      || cloud.rotations.size() != (size_t)n * 4
      || cloud.scales.size() != (size_t)n * 3
      || cloud.alphas.size() != n
      || cloud.colors.size() != (size_t)n * 3
      || cloud.sh.size() != (size_t)n * (shCount - 1) * 3
      || n > SIZE_MAX / sizeof(float) / stride)
    return -1;

  if (!(buffer = (AkBuffer *)ak_heap_calloc(heap, prim, sizeof(*buffer))))
    return -1;

  buffer->length = (size_t)n * stride * sizeof(float);
  if (!(buffer->data = ak_heap_alloc(heap, buffer, buffer->length)))
    return -1;

  values = (float *)buffer->data;

  /* Decode into the final interleaved allocation. SH coefficients,
     including DC, remain unmodified. */
  for (i = 0; i < n; i++) {
    row  = values + (size_t)i * stride;
    norm = 0.0;

    for (c = 0; c < 3; c++) {
      row[c]      = cloud.positions[(size_t)i * 3 + c];
      row[7 + c]  = std::exp(cloud.scales[(size_t)i * 3 + c]);
      row[11 + c] = cloud.colors[(size_t)i * 3 + c];
    }

    for (c = 0; c < 4; c++) {
      row[3 + c] = cloud.rotations[(size_t)i * 4 + c];
      norm += (double)row[3 + c] * row[3 + c];
    }

    if (!std::isfinite(norm) || norm <= 0.0)
      return -1;

    norm = 1.0 / std::sqrt(norm);

    for (c = 0; c < 4; c++)
      row[3 + c] = (float)(row[3 + c] * norm);

    row[10] = 1.0f / (1.0f + std::exp(-cloud.alphas[i]));
    if (shCount > 1)
      std::memcpy(row + 14, cloud.sh.data() + (size_t)i * (shCount - 1) * 3,
                  (shCount - 1) * 3 * sizeof(float));

    for (c = 0; c < stride; c++)
      if (!std::isfinite(row[c]))
        return -1;
  }

  if (!add_input(heap, prim, buffer, n, stride, 0, 3, AK_INPUT_POSITION, "POSITION")
      || !add_input(heap, prim, buffer, n, stride, 3, 4, AK_INPUT_ROTATION, "ROTATION")
      || !add_input(heap, prim, buffer, n, stride, 7, 3, AK_INPUT_SCALE, "SCALE")
      || !add_input(heap, prim, buffer, n, stride, 10, 1, AK_INPUT_OPACITY, "OPACITY"))
    return -1;

  for (c = 0; c < shCount; c++)
    if (!add_input(heap, prim, buffer, n, stride, 11 + c * 3, 3, AK_INPUT_SH, "SH", c))
      return -1;

  if (!(gs = prim->gsplat)) {
    if (!(gs = (AkGaussianSplat *)ak_heap_calloc(heap, prim, sizeof(*gs))))
      return -1;

    gs->kernel     = AK_GSPLAT_KERNEL_ELLIPSE;
    gs->colorSpace = AK_GSPLAT_COLOR_SRGB_REC709_DISPLAY;
    prim->gsplat   = gs;
  }

  gs->decodedCount = n;
  gs->shDegree     = (uint8_t)cloud.shDegree;
  gs->antialiased  = cloud.antialiased;

  return 0;
}

} /* namespace */

extern "C" AK_SPZ_EXPORT
int
ak_spz_decodeBytes(AkHeap *heap, AkMeshPrimitive *prim, const uint8_t *data, size_t size) {
  if (!heap || !prim || !data || !size)
    return -1;

  try {
    return decode(heap, prim, data, size);
  } catch (...) {
    return -1;
  }
}

extern "C" AK_SPZ_EXPORT
int
assetkit_gsplat_create(AkGaussianSplatDecoder *out) {
  if (!out)
    return -1;

  std::memset(out, 0, sizeof(*out));
  out->decodeBytes = ak_spz_decodeBytes;

  return 0;
}
