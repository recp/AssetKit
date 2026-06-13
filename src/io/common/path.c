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

#include "path.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#  include <direct.h>
#endif

static
int
io_path_mkdir(const char * __restrict path) {
#ifdef _WIN32
  return _mkdir(path);
#else
  return mkdir(path, 0777);
#endif
}

AK_HIDE
char*
io_path_output_dir_dup(const char * __restrict filepath) {
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
    len = 1u;

  dir = malloc(len + 1u);
  if (!dir)
    return NULL;

  memcpy(dir, filepath, len);
  dir[len] = '\0';

  return dir;
}

AK_HIDE
const char*
io_path_basename(const char * __restrict path) {
  const char *slash;
  const char *backslash;

  if (!path)
    return NULL;

  slash     = strrchr(path, '/');
  backslash = strrchr(path, '\\');

  if (slash && backslash)
    return slash > backslash ? slash + 1 : backslash + 1;
  if (slash)
    return slash + 1;
  if (backslash)
    return backslash + 1;

  return path;
}

AK_HIDE
const char*
io_path_basename_nonempty(const char * __restrict path) {
  const char *base;

  base = io_path_basename(path);
  return base && *base ? base : path;
}

AK_HIDE
char*
io_path_basename_dup(const char * __restrict path) {
  const char *base;
  char       *dst;
  size_t      len;

  base = io_path_basename(path);
  if (!base)
    return NULL;

  len = strlen(base);
  dst = malloc(len + 1u);
  if (!dst)
    return NULL;

  memcpy(dst, base, len + 1u);
  return dst;
}

AK_HIDE
char*
io_path_replace_extension_dup(const char * __restrict filepath,
                              const char * __restrict ext,
                              size_t                  extLen) {
  const char *base;
  const char *dot;
  char       *path;
  size_t      stemLen;

  if (!filepath || !ext || extLen == 0)
    return NULL;

  base    = io_path_basename(filepath);
  dot     = strrchr(base, '.');
  stemLen = dot && dot != base ? (size_t)(dot - filepath) : strlen(filepath);

  if (stemLen > (size_t)-1 - extLen)
    return NULL;

  path = malloc(stemLen + extLen);
  if (!path)
    return NULL;

  memcpy(path, filepath, stemLen);
  memcpy(path + stemLen, ext, extLen);

  return path;
}

static
bool
io_path_join_parts_ex(const char * __restrict dir,
                      const char * __restrict rel,
                      bool                    trimDir,
                      size_t     * __restrict dirLen,
                      size_t     * __restrict relLen,
                      bool       * __restrict sep,
                      size_t     * __restrict need) {
  size_t dlen;
  size_t rlen;
  size_t slen;

  if (!dir || !rel || !dirLen || !relLen || !sep || !need)
    return false;

  dlen = strlen(dir);
  if (trimDir) {
    while (dlen > 0
           && (dir[dlen - 1u] == '/' || dir[dlen - 1u] == '\\'))
      dlen--;
  }
  rlen = strlen(rel);
  slen = dlen > 0 && dir[dlen - 1u] != '/' && dir[dlen - 1u] != '\\';
  if (dlen > (size_t)-1 - rlen
      || dlen + rlen > (size_t)-1 - slen
      || dlen + rlen + slen > (size_t)-1 - 1u)
    return false;

  *dirLen = dlen;
  *relLen = rlen;
  *sep    = slen != 0;
  *need   = dlen + rlen + slen + 1u;

  return true;
}

AK_HIDE
void
io_path_join_write(char       * __restrict path,
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
io_path_join_dup(const char * __restrict dir,
                 const char * __restrict rel) {
  size_t dirLen;
  size_t relLen;
  size_t need;
  bool   sep;
  char  *path;

  if (!io_path_join_parts_ex(dir, rel, false, &dirLen, &relLen, &sep, &need))
    return NULL;

  path = malloc(need);
  if (!path)
    return NULL;

  io_path_join_write(path, dir, rel, dirLen, relLen, sep);

  return path;
}

AK_HIDE
char*
io_path_join_dup_trim_dir(const char * __restrict dir,
                          const char * __restrict rel) {
  size_t dirLen;
  size_t relLen;
  size_t need;
  bool   sep;
  char  *path;

  if (!io_path_join_parts_ex(dir, rel, true, &dirLen, &relLen, &sep, &need))
    return NULL;

  path = malloc(need);
  if (!path)
    return NULL;

  io_path_join_write(path, dir, rel, dirLen, relLen, sep);

  return path;
}

AK_HIDE
bool
io_path_join_buf(const char * __restrict dir,
                 const char * __restrict rel,
                 char       * __restrict path,
                 size_t                  pathCap) {
  size_t dirLen;
  size_t relLen;
  size_t need;
  bool   sep;

  if (!path || pathCap == 0)
    return false;

  if (!io_path_join_parts_ex(dir, rel, false, &dirLen, &relLen, &sep, &need))
    return false;
  if (need > pathCap)
    return false;

  io_path_join_write(path, dir, rel, dirLen, relLen, sep);

  return true;
}

AK_HIDE
bool
io_path_mkdir_parent_dirs(char * __restrict path,
                          bool              normalizeSeparators) {
  char *it;

  if (!path)
    return false;

  for (it = path; *it; it++) {
    char saved;

    if (*it != '/' && *it != '\\')
      continue;
    if (it == path)
      continue;

    saved = *it;
    *it  = '\0';
    if (path[0] != '\0'
        && io_path_mkdir(path) != 0
        && errno != EEXIST) {
      *it = normalizeSeparators ? '/' : saved;
      return false;
    }
    *it = normalizeSeparators ? '/' : saved;
  }

  return true;
}
