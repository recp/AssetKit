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

#include "uri.h"

#include <stdlib.h>
#include <string.h>

AK_HIDE
bool
io_uri_has_prefix(const char * __restrict uri,
                  const char * __restrict prefix,
                  size_t                  prefixLen) {
  size_t i;

  if (!uri)
    return false;

  for (i = 0; i < prefixLen; i++) {
    if (uri[i] == '\0' || uri[i] != prefix[i])
      return false;
  }

  return true;
}

AK_HIDE
bool
io_uri_has_scheme(const char * __restrict uri) {
  const char *it;

  if (!uri)
    return false;

  for (it = uri; *it; it++) {
    if (*it == ':' && it != uri)
      return true;
    if (*it == '/' || *it == '\\' || *it == '?' || *it == '#')
      return false;
  }

  return false;
}

AK_HIDE
bool
io_path_is_abs_drive_colon(const char * __restrict path) {
  return path
         && (path[0] == '/'
             || path[0] == '\\'
             || (((path[0] >= 'A' && path[0] <= 'Z')
                  || (path[0] >= 'a' && path[0] <= 'z'))
                 && path[1] == ':'));
}

AK_HIDE
bool
io_path_is_abs_drive_slash(const char * __restrict path) {
  unsigned char drive;

  if (!path || !path[0])
    return false;

  if (path[0] == '/' || path[0] == '\\')
    return true;

  drive = (unsigned char)path[0];
  return ((drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z'))
         && path[1] == ':'
         && (path[2] == '/' || path[2] == '\\');
}

AK_HIDE
int
io_uri_hex_digit(unsigned char c) {
  if (c >= '0' && c <= '9')
    return (int)(c - '0');
  if (c >= 'A' && c <= 'F')
    return (int)(c - 'A') + 10;
  if (c >= 'a' && c <= 'f')
    return (int)(c - 'a') + 10;
  return -1;
}

AK_HIDE
bool
io_uri_pct_encoded(const char * __restrict uri, size_t i) {
  return uri
         && uri[i] == '%'
         && uri[i + 1] != '\0'
         && uri[i + 2] != '\0'
         && io_uri_hex_digit((unsigned char)uri[i + 1]) >= 0
         && io_uri_hex_digit((unsigned char)uri[i + 2]) >= 0;
}

AK_HIDE
char*
io_uri_escape_dup(const char * __restrict uri,
                  bool                    allowSlash,
                  bool                    preservePctEncoded) {
  static const char hex[] = "0123456789ABCDEF";
  char  *escaped;
  size_t len;
  size_t i;
  size_t j;

  if (!uri)
    return NULL;

  len = strlen(uri);
  if (len > ((size_t)-1 - 1u) / 3u)
    return NULL;

  escaped = malloc(len * 3u + 1u);
  if (!escaped)
    return NULL;

  j = 0;
  for (i = 0; uri[i]; i++) {
    unsigned char c;

    c = (unsigned char)uri[i];
    if ((allowSlash && c == '/') || io_uri_unreserved(c)) {
      escaped[j++] = (char)c;
    } else if (preservePctEncoded && io_uri_pct_encoded(uri, i)) {
      escaped[j++] = uri[i++];
      escaped[j++] = uri[i++];
      escaped[j++] = uri[i];
    } else {
      escaped[j++] = '%';
      escaped[j++] = hex[c >> 4];
      escaped[j++] = hex[c & 0x0f];
    }
  }
  escaped[j] = '\0';

  return escaped;
}

AK_HIDE
bool
io_uri_decode_path(const char * __restrict uri,
                   char       * __restrict dst,
                   size_t                  dstCap) {
  size_t i;
  size_t j;

  if (!uri || !dst || dstCap == 0)
    return false;

  i = 0;
  j = 0;
  while (uri[i]) {
    unsigned char c;

    if (j + 1u >= dstCap)
      return false;

    c = (unsigned char)uri[i];
    if (c == '%' && uri[i + 1] && uri[i + 2]) {
      int hi;
      int lo;

      hi = io_uri_hex_digit((unsigned char)uri[i + 1]);
      lo = io_uri_hex_digit((unsigned char)uri[i + 2]);
      if (hi >= 0 && lo >= 0) {
        dst[j++] = (char)((hi << 4) | lo);
        i += 3u;
        continue;
      }
    }

    dst[j++] = (char)c;
    i++;
  }

  dst[j] = '\0';
  return true;
}
