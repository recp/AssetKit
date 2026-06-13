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

#ifndef io_common_buffer_h
#define io_common_buffer_h

#include "../../../include/ak/common.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct IOBuffer {
  char    *data;
  size_t   len;
  size_t   cap;
  AkResult result;
} IOBuffer;

static inline
void
io_buffer_init(IOBuffer * __restrict buf) {
  memset(buf, 0, sizeof(*buf));
  buf->result = AK_OK;
}

static inline
bool
io_buffer_ok(const IOBuffer * __restrict buf) {
  return buf && buf->result == AK_OK;
}

static inline
bool
io_buffer_reserve(IOBuffer * __restrict buf,
                  size_t                  extra,
                  size_t                  initialCap) {
  char  *data;
  size_t newCap;

  if (buf->result != AK_OK)
    return false;
  if (extra <= buf->cap - buf->len)
    return true;

  newCap = buf->cap ? buf->cap * 2u : initialCap;
  while (extra > newCap - buf->len) {
    if (newCap > SIZE_MAX / 2u) {
      buf->result = AK_ERR;
      return false;
    }
    newCap *= 2u;
  }

  data = realloc(buf->data, newCap);
  if (!data) {
    buf->result = AK_ERR;
    return false;
  }

  buf->data = data;
  buf->cap  = newCap;
  return true;
}

static inline
void
io_buffer_raw(IOBuffer  * __restrict buf,
              const void * __restrict data,
              size_t                   len,
              size_t                   initialCap) {
  if (!io_buffer_reserve(buf, len, initialCap))
    return;

  memcpy(buf->data + buf->len, data, len);
  buf->len += len;
}

#define IO_BUFFER_LIT(BUF, LIT, INITIAL_CAP)                                  \
  io_buffer_raw((BUF), "" LIT, sizeof("" LIT) - 1u, (INITIAL_CAP))

static inline
void
io_buffer_cstr(IOBuffer   * __restrict buf,
               const char * __restrict str,
               size_t                   initialCap) {
  io_buffer_raw(buf, str, strlen(str), initialCap);
}

static inline
void
io_buffer_ch(IOBuffer * __restrict buf, char ch, size_t initialCap) {
  if (!io_buffer_reserve(buf, 1u, initialCap))
    return;

  buf->data[buf->len++] = ch;
}

static inline
void
io_buffer_terminate(IOBuffer * __restrict buf, size_t initialCap) {
  if (!io_buffer_reserve(buf, 1u, initialCap))
    return;

  buf->data[buf->len] = '\0';
}

static inline
void
io_buffer_free(IOBuffer * __restrict buf) {
  free(buf->data);
  memset(buf, 0, sizeof(*buf));
}

#endif /* io_common_buffer_h */
