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

#include <stdbool.h>
#include <stddef.h>

static inline
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

static inline
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

static inline
bool
io_path_is_abs_drive_colon(const char * __restrict path) {
  return path
         && (path[0] == '/'
             || path[0] == '\\'
             || (((path[0] >= 'A' && path[0] <= 'Z')
                  || (path[0] >= 'a' && path[0] <= 'z'))
                 && path[1] == ':'));
}

static inline
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

#endif /* io_common_uri_h */
