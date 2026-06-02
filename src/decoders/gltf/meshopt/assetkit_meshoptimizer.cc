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

#include <stddef.h>

#include <ak/assetkit.h>

#include "meshoptimizer.h"

#if defined(_WIN32)
#  define AK_MESHOPT_EXPORT __declspec(dllexport)
#else
#  define AK_MESHOPT_EXPORT __attribute__((visibility("default")))
#endif

typedef enum AkMeshoptMode {
  AK_MESHOPT_MODE_UNKNOWN    = 0,
  AK_MESHOPT_MODE_ATTRIBUTES = 1,
  AK_MESHOPT_MODE_TRIANGLES  = 2,
  AK_MESHOPT_MODE_INDICES    = 3
} AkMeshoptMode;

typedef enum AkMeshoptFilter {
  AK_MESHOPT_FILTER_NONE        = 0,
  AK_MESHOPT_FILTER_OCTAHEDRAL  = 1,
  AK_MESHOPT_FILTER_QUATERNION  = 2,
  AK_MESHOPT_FILTER_EXPONENTIAL = 3
} AkMeshoptFilter;

extern "C"
AK_MESHOPT_EXPORT
int
ak_meshopt_decode_gltf_buffer(void                *destination,
                              size_t               destination_size,
                              const unsigned char *buffer,
                              size_t               buffer_size,
                              size_t               count,
                              size_t               stride,
                              int                  mode,
                              int                  filter) {
  int res;

  if (!destination
      || !buffer
      || stride == 0
      || count > ((size_t)-1) / stride
      || destination_size < count * stride)
    return -1;

  res = -1;
  switch ((AkMeshoptMode)mode) {
    case AK_MESHOPT_MODE_ATTRIBUTES:
      res = meshopt_decodeVertexBuffer(destination,
                                       count,
                                       stride,
                                       buffer,
                                       buffer_size);
      break;
    case AK_MESHOPT_MODE_TRIANGLES:
      res = meshopt_decodeIndexBuffer(destination,
                                      count,
                                      stride,
                                      buffer,
                                      buffer_size);
      break;
    case AK_MESHOPT_MODE_INDICES:
      res = meshopt_decodeIndexSequence(destination,
                                        count,
                                        stride,
                                        buffer,
                                        buffer_size);
      break;
    default:
      return -1;
  }

  if (res != 0)
    return res;

  switch ((AkMeshoptFilter)filter) {
    case AK_MESHOPT_FILTER_OCTAHEDRAL:
      meshopt_decodeFilterOct(destination, count, stride);
      break;
    case AK_MESHOPT_FILTER_QUATERNION:
      meshopt_decodeFilterQuat(destination, count, stride);
      break;
    case AK_MESHOPT_FILTER_EXPONENTIAL:
      meshopt_decodeFilterExp(destination, count, stride);
      break;
    default:
      break;
  }

  return 0;
}
