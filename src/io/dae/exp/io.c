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

#include "io.h"
#include "../strpool.h"
#include "../../common/path.h"
#include "../../common/uri.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AK_HIDE
bool
dae_uri_rel_safe(const char * __restrict uri) {
  const char *seg;
  const char *it;

  if (!uri || uri[0] == '\0' || io_path_is_abs_drive_colon(uri))
    return false;

  seg = uri;
  for (it = uri; ; it++) {
    if (*it == '/' || *it == '\\' || *it == '\0') {
      size_t len;

      len = (size_t)(it - seg);
      if (len == 2u && seg[0] == '.' && seg[1] == '.')
        return false;
      if (*it == '\0')
        break;
      seg = it + 1;
    }
  }

  return true;
}

AK_HIDE
bool
dae_write_file_bytes(const char * __restrict dst,
                     const void * __restrict data,
                     size_t                  len) {
  FILE *out;
  bool  ok;

  if (!dst || !data || len == 0)
    return false;

  out = fopen(dst, "wb");
  if (!out)
    return false;

  ok = fwrite(data, 1, len, out) == len;
  if (fclose(out) != 0)
    ok = false;

  if (!ok)
    remove(dst);

  return ok;
}
