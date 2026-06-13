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

#ifndef io_common_uri_h
#define io_common_uri_h

#include "../../common.h"

#include <stdbool.h>
#include <stddef.h>

#define IO_URI_DATA_PREFIX     "data:"
#define IO_URI_DATA_PREFIX_LEN 5u
#define IO_URI_FILE_PREFIX     "file://"
#define IO_URI_FILE_PREFIX_LEN 7u

AK_HIDE
bool
io_uri_has_prefix(const char * __restrict uri,
                  const char * __restrict prefix,
                  size_t                  prefixLen);

AK_HIDE
bool
io_uri_has_scheme(const char * __restrict uri);

AK_HIDE
bool
io_path_is_abs_drive_colon(const char * __restrict path);

AK_INLINE
const char*
io_uri_file_path(const char * __restrict uri,
                 const char * __restrict filePrefix,
                 size_t                  filePrefixLen) {
  const char *path;

  if (!uri)
    return NULL;

  if (io_uri_has_prefix(uri, filePrefix, filePrefixLen)) {
    path = uri + filePrefixLen;
#ifdef _WIN32
    if (path[0] == '/'
        && (((path[1] >= 'A' && path[1] <= 'Z')
             || (path[1] >= 'a' && path[1] <= 'z'))
            && path[2] == ':'))
      path++;
#endif
    return path;
  }

  return io_path_is_abs_drive_colon(uri) ? uri : NULL;
}

AK_HIDE
bool
io_path_is_abs_drive_slash(const char * __restrict path);

AK_HIDE
int
io_uri_hex_digit(unsigned char c);

AK_HIDE
bool
io_uri_pct_encoded(const char * __restrict uri, size_t i);

AK_INLINE
bool
io_uri_unreserved(unsigned char c) {
  return (c >= 'A' && c <= 'Z')
         || (c >= 'a' && c <= 'z')
         || (c >= '0' && c <= '9')
         || c == '-'
         || c == '_'
         || c == '.'
         || c == '~';
}

AK_HIDE
char*
io_uri_escape_dup(const char * __restrict uri,
                  bool                    allowSlash,
                  bool                    preservePctEncoded);

AK_HIDE
bool
io_uri_decode_path(const char * __restrict uri,
                   char       * __restrict dst,
                   size_t                  dstCap);

#endif /* io_common_uri_h */
