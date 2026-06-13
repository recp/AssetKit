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

#include "string.h"

#include <stdlib.h>
#include <string.h>

AK_HIDE
char*
io_strdup(const char * __restrict src) {
  char  *dst;
  size_t len;

  if (!src)
    return NULL;

  len = strlen(src);
  dst = malloc(len + 1u);
  if (!dst)
    return NULL;

  memcpy(dst, src, len + 1u);
  return dst;
}
