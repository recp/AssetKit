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

#ifndef io_common_binary_h
#define io_common_binary_h

#include <stdint.h>
#include <string.h>

static inline
uint16_t
io_load_u16le(const unsigned char * __restrict src) {
  return (uint16_t)((uint16_t)src[0] | ((uint16_t)src[1] << 8u));
}

static inline
uint32_t
io_load_u32le(const unsigned char * __restrict src) {
  return (uint32_t)src[0]
         | ((uint32_t)src[1] << 8u)
         | ((uint32_t)src[2] << 16u)
         | ((uint32_t)src[3] << 24u);
}

static inline
void
io_store_u16le(unsigned char dst[2], uint16_t value) {
  dst[0] = (unsigned char)(value & 0xffu);
  dst[1] = (unsigned char)((value >> 8u) & 0xffu);
}

static inline
void
io_store_u32le(unsigned char dst[4], uint32_t value) {
  dst[0] = (unsigned char)(value & 0xffu);
  dst[1] = (unsigned char)((value >> 8u) & 0xffu);
  dst[2] = (unsigned char)((value >> 16u) & 0xffu);
  dst[3] = (unsigned char)((value >> 24u) & 0xffu);
}

static inline
void
io_store_f32le(unsigned char dst[4], float value) {
  uint32_t bits;

  memcpy(&bits, &value, sizeof(bits));
  io_store_u32le(dst, bits);
}

#endif /* io_common_binary_h */
