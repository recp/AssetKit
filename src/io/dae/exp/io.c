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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#  include <direct.h>
#  define DAE_EXP_MKDIR(PATH) _mkdir(PATH)
#else
#  include <unistd.h>
#  define DAE_EXP_MKDIR(PATH) mkdir(PATH, 0777)
#endif

AK_HIDE
char*
dae_output_dir(const char * __restrict filepath) {
  const char *slash;
  const char *backslash;
  const char *lastSlash;
  char       *dir;
  size_t      len;

  if (!filepath)
    return NULL;

  slash      = strrchr(filepath, '/');
  backslash  = strrchr(filepath, '\\');
  if (slash && backslash)
    lastSlash = slash > backslash ? slash : backslash;
  else
    lastSlash = slash ? slash : backslash;

  if (!lastSlash) {
    dir = malloc(2u);
    if (!dir)
      return NULL;
    dir[0] = '.';
    dir[1] = '\0';
    return dir;
  }

  len = (size_t)(lastSlash - filepath);
  if (len == 0)
    len = 1;

  dir = malloc(len + 1u);
  if (!dir)
    return NULL;

  memcpy(dir, filepath, len);
  dir[len] = '\0';

  return dir;
}

AK_HIDE
bool
dae_path_is_abs(const char * __restrict path) {
  return path
         && (path[0] == '/'
             || path[0] == '\\'
             || (((path[0] >= 'A' && path[0] <= 'Z')
                  || (path[0] >= 'a' && path[0] <= 'z'))
                 && path[1] == ':'));
}

static
bool
dae_uri_has_prefix(const char * __restrict uri,
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
dae_uri_has_scheme(const char * __restrict uri) {
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
dae_uri_is_data(const char * __restrict uri) {
  return dae_uri_has_prefix(uri, "data:", 5u);
}

AK_HIDE
bool
dae_uri_is_file_scheme(const char * __restrict uri) {
  return dae_uri_has_prefix(uri, "file://", 7u);
}

static
int
dae_uri_hex(unsigned char c) {
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
dae_uri_decode_path(const char * __restrict uri,
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

      hi = dae_uri_hex((unsigned char)uri[i + 1]);
      lo = dae_uri_hex((unsigned char)uri[i + 2]);
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

AK_HIDE
const char*
dae_uri_file_path(const char * __restrict uri) {
  const char *path;

  if (!uri)
    return NULL;

  if (dae_uri_is_file_scheme(uri)) {
    path = uri + 7u;
#ifdef _WIN32
    if (path[0] == '/'
        && (((path[1] >= 'A' && path[1] <= 'Z')
             || (path[1] >= 'a' && path[1] <= 'z'))
            && path[2] == ':'))
      path++;
#endif
    return path;
  }

  return dae_path_is_abs(uri) ? uri : NULL;
}

AK_HIDE
bool
dae_uri_rel_safe(const char * __restrict uri) {
  const char *seg;
  const char *it;

  if (!uri || uri[0] == '\0' || dae_path_is_abs(uri))
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

static
bool
dae_join_path_parts(const char * __restrict dir,
                    const char * __restrict rel,
                    size_t     * __restrict dirLen,
                    size_t     * __restrict relLen,
                    bool       * __restrict sep,
                    size_t     * __restrict need) {
  size_t dlen;
  size_t rlen;
  size_t slen;
  size_t sum;
  size_t max;

  if (!dir || !rel || !dirLen || !relLen || !sep || !need)
    return false;

  dlen = strlen(dir);
  rlen = strlen(rel);
  slen = dlen > 0 && dir[dlen - 1u] != '/' && dir[dlen - 1u] != '\\';
  max  = (size_t)-1;
  if (dlen > max - rlen)
    return false;
  sum = dlen + rlen;
  if (sum > max - slen || sum + slen > max - 1u)
    return false;

  *dirLen = dlen;
  *relLen = rlen;
  *sep    = slen != 0;
  *need   = sum + slen + 1u;

  return true;
}

static
void
dae_join_path_write(char       * __restrict path,
                    const char * __restrict dir,
                    const char * __restrict rel,
                    size_t                  dirLen,
                    size_t                  relLen,
                    bool                    sep) {
  memcpy(path, dir, dirLen);
  if (sep)
    path[dirLen++] = '/';
  memcpy(path + dirLen, rel, relLen);
  path[dirLen + relLen] = '\0';
}

AK_HIDE
char*
dae_join_path(const char * __restrict dir, const char * __restrict rel) {
  size_t dirLen;
  size_t relLen;
  size_t need;
  bool   sep;
  char  *path;

  if (!dae_join_path_parts(dir, rel, &dirLen, &relLen, &sep, &need))
    return NULL;

  path = malloc(need);
  if (!path)
    return NULL;

  dae_join_path_write(path, dir, rel, dirLen, relLen, sep);

  return path;
}

AK_HIDE
bool
dae_join_path_buf(const char * __restrict dir,
                  const char * __restrict rel,
                  char       * __restrict path,
                  size_t                  pathCap) {
  size_t dirLen;
  size_t relLen;
  size_t need;
  bool   sep;

  if (!path || pathCap == 0)
    return false;

  if (!dae_join_path_parts(dir, rel, &dirLen, &relLen, &sep, &need))
    return false;
  if (need > pathCap)
    return false;

  dae_join_path_write(path, dir, rel, dirLen, relLen, sep);

  return true;
}

AK_HIDE
bool
dae_mkdir_parent_dirs(char * __restrict path) {
  char *it;

  if (!path)
    return false;

  for (it = path + 1; *it; it++) {
    if (*it != '/' && *it != '\\')
      continue;

    {
      char saved;

      saved = *it;
      *it = '\0';
      if (path[0] != '\0'
          && DAE_EXP_MKDIR(path) != 0
          && errno != EEXIST) {
        *it = saved;
        return false;
      }
      *it = saved;
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

AK_HIDE
char*
dae_strdup(const char * __restrict src) {
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
