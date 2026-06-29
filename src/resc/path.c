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

#include "../common.h"
#include "../string_fast.h"
#include "../../include/ak/path.h"
#include "../../include/ak/string.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#if defined(__APPLE__)
#  include <sys/clonefile.h>
#endif

#ifdef _MSC_VER
#  ifndef PATH_MAX
#    define PATH_MAX 260
#  endif
#endif

#define CHR_SLASH       '/'
#define CHR_BACK_SLASH  '\\'
#define CHR_COLON       ':'
#define STR_SLASH       "/"
#define AK_PATH_FILE_SCHEME_U32 AK_STR_PACK4_CHARS('f', 'i', 'l', 'e')
#define AK_PATH_FILE_PREFIX_LEN 7
#define AK_PATH_FILE_PREFIX_U64 AK_STR_PACK8_CHARS('f', 'i', 'l', 'e',      \
                                                    ':', '/', '/', '\0')
#define APPEND_SLASH                                                          \
  do {                                                                        \
    *buf++ = CHR_SLASH;                                                       \
    len++;                                                                    \
  } while (0)

#define AK_STRTRM_SET(X)                                                      \
  do {                                                                        \
    *it2 = *it1;                                                              \
    len++;                                                                    \
    it1++;                                                                    \
    it2++;                                                                    \
    skp = X;                                                                  \
  } while (0)

static
bool
ak_path_same_file(const char * __restrict a,
                  const char * __restrict b) {
  struct stat sta;
  struct stat stb;

  if (!a || !b)
    return false;

  if (strcmp(a, b) == 0)
    return true;

  if (stat(a, &sta) != 0 || stat(b, &stb) != 0)
    return false;

  return sta.st_dev == stb.st_dev && sta.st_ino == stb.st_ino;
}

AK_EXPORT
const char *
ak_path_fragment(const char *path) {
  return strrchr(path, '#');
}

AK_EXPORT
int
ak_path_isfile(const char *path) {
  const char *it, *schemeEnd;

  it = path;
  while (*it == ' ')
    it++;

  if (*it == '/' || *it == '\\')
    return 1;

  if ((schemeEnd = strstr(it, "://")))
    if ((size_t)(schemeEnd - it) != 4
        || ak_str_pack4_ci_fast(it, 4) != AK_PATH_FILE_SCHEME_U32)
      return 0;

  return 1;
}

AK_EXPORT
bool
ak_clonefile(const char * __restrict src,
             const char * __restrict dst) {
  if (!src || !dst)
    return false;

#if defined(__APPLE__)
  return clonefile(src, dst, 0) == 0;
#else
  (void)src;
  (void)dst;
  return false;
#endif
}

AK_EXPORT
bool
ak_copyfile(const char * __restrict src,
            const char * __restrict dst) {
  FILE          *infile;
  FILE          *outfile;
  unsigned char  buf[64u * 1024u];
  size_t         nread;
  bool           ok;

  if (!src || !dst)
    return false;

  if (ak_path_same_file(src, dst))
    return true;

  if (ak_clonefile(src, dst))
    return true;

  infile = fopen(src, "rb");
  if (!infile)
    return false;

  outfile = fopen(dst, "wb");
  if (!outfile) {
    fclose(infile);
    return false;
  }

  ok = true;
  while ((nread = fread(buf, 1, sizeof(buf), infile)) > 0) {
    if (fwrite(buf, 1, nread, outfile) != nread) {
      ok = false;
      break;
    }
  }

  if (ferror(infile))
    ok = false;
  if (fclose(outfile) != 0)
    ok = false;
  fclose(infile);

  if (!ok)
    remove(dst);

  return ok;
}

AK_EXPORT
char*
ak_path_dir(AkHeap     * __restrict heap,
            void       * __restrict memparent,
            const char * __restrict path) {
  char *dir;
  char *slash;

  slash = strrchr(path, '/');
  if (!slash)
    return ak_heap_strdup(heap, memparent, "./");

  dir = ak_heap_strndup(heap,
                        memparent,
                        path,
                        slash - path);
  return dir;
}

AK_EXPORT
size_t
ak_path_trim(const char *path,
             char *trimmed) {
  const char *it1;
  char       *it2;
  size_t      len;
  int         skp;
  int         proto;
  int         local;

  len = skp = proto = 0;
  it1 = path;
  it2 = trimmed;

  while (*it1 == ' ')
    it1++;

  local = *it1 == '/' || *it1 == '\\';

  while (*it1) {
    if (*it1 == '/' || *it1 == '\\') {
      if (skp != 0) {
        if (proto != 1) {
          it1++;
          continue;
        }

        proto = 2;
      }

      AK_STRTRM_SET(1);
    } else {
      proto = !proto && *it1 == ':' && !local;
      AK_STRTRM_SET(0);
    }
  }

  while (*--it2 && (*it2 == ' ' || *it2 == '/' || *it2 == '\\'))
    len--;

  *(it2 + (len > 0)) = '\0';

  return len;
}

AK_EXPORT
int
ak_path_join(char   *fragments[],
             char   *buf,
             size_t *size) {
  const char *frag;
  const char *frag_end;
  size_t      len;
  size_t      frag_len;
  size_t      frag_idx;
  size_t      frag_idx_v;
  char        c;

  if (!fragments
      || !*fragments /* there are no fragments to join */
      || !buf
      || !size)
    return -EINVAL;

  frag_idx_v = frag_idx = len = 0;

  /* build path */
  while ((frag = fragments[frag_idx++])) {
    frag_len = strlen(frag);
    frag_end = frag + frag_len - 1;

    if (frag_len == 0)
      continue;

    /* starts with slash e.g. /usr */
    c = *frag;
    if (frag_idx_v == 0
        && (c == CHR_SLASH || c == CHR_BACK_SLASH)) {
      APPEND_SLASH;
      frag++;

      /* avoid extra slash after non-protocol frag 
         like /http:/ -> /http:// 
       */
      frag_idx_v++;
    }

    /* l-trim */
    while ((c = *frag) != '\0'
           && (c == CHR_SLASH || c == CHR_BACK_SLASH))
      frag++;

    /* r-trim */
    while (frag_end > frag
           && (c = *frag_end) != '\0'
           && (c == CHR_SLASH || c == CHR_BACK_SLASH))
      frag_end--;

    /* only slashes */
    if (!*frag) {
      if (len == 0)
        APPEND_SLASH;

      continue;
    }

    if (len > 1 && *(buf - 1) != CHR_SLASH)
      APPEND_SLASH;

    /* protocol e.g. http://, tcp:// */
    if (len > 1 && frag_idx_v == 1 && *(buf - 2) == CHR_COLON)
      APPEND_SLASH;

    frag_len = ++frag_end - frag;
    len += frag_len;

    memcpy(buf, frag, frag_len);

    buf += frag_len;
    frag_idx_v++;
  }

  memset(buf, '\0', 1);
  *size = len;

  return 0;
}

static
const char*
ak_path_file_uri_path(const char * __restrict ref) {
  size_t      prefixLen;

  prefixLen = 0;
  while (prefixLen < AK_PATH_FILE_PREFIX_LEN && ref[prefixLen] != '\0')
    prefixLen++;

  if (prefixLen == AK_PATH_FILE_PREFIX_LEN
      && ak_str_pack8_fast(ref, AK_PATH_FILE_PREFIX_LEN)
      == AK_PATH_FILE_PREFIX_U64) {
    return ref + AK_PATH_FILE_PREFIX_LEN
#ifndef _WIN32
    - 1
#endif
    ;
  }

  return NULL;
}

static
const char*
ak_fullpath__join(AkDoc       * __restrict doc,
                  const char  * __restrict ref,
                  char        * __restrict buf) {
  size_t      pathlen;
  const char *ptr;
  char       *fragments[] = {
    (char *)doc->inf->dir,
    "/",
    (char *)ref,
    NULL
  };

  ak_path_join(fragments, buf, &pathlen);

  ptr = ak_strltrim_fast(buf);

  return ptr;
}

AK_EXPORT
const char*
ak_fullpathn(AkDoc       * __restrict doc,
             const char  * __restrict ref,
             char        * __restrict buf,
             size_t                   buflen) {
  const char *filePath;
  size_t      dirLen;
  size_t      refLen;

  if (!ref)
    return NULL;

  if ((filePath = ak_path_file_uri_path(ref)))
    return filePath;

  if (!doc || !doc->inf || !doc->inf->dir || !buf || buflen == 0)
    return NULL;

  dirLen = strlen(doc->inf->dir);
  refLen = strlen(ref);
  if (dirLen >= buflen || refLen >= buflen || dirLen + refLen + 4u > buflen)
    return NULL;

  return ak_fullpath__join(doc, ref, buf);
}

AK_EXPORT
const char*
ak_fullpath(AkDoc       * __restrict doc,
            const char  * __restrict ref,
            char        * __restrict buf) {
  const char *filePath;

  if ((filePath = ak_path_file_uri_path(ref)))
    return filePath;

  return ak_fullpath__join(doc, ref, buf);
}
