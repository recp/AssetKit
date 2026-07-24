/*
 * Copyright (C) 2026 Recep Aslantas
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

#include "texture.h"
#include "../../common/uri.h"
#include "../../../string_fast.h"

#include <sys/stat.h>

static
bool
dae_bugfix_texture_ref_key(const char * __restrict key,
                           const char * __restrict ref) {
  size_t keyLen;
  size_t refLen;

  if (!key || !ref || !*ref)
    return false;

  keyLen = strlen(key);
  refLen = strlen(ref);
  if (keyLen == refLen)
    return memcmp(key, ref, refLen) == 0;
  if (keyLen <= refLen || memcmp(key + keyLen - refLen, ref, refLen) != 0)
    return false;

  switch (key[keyLen - refLen - 1u]) {
    case '-':
    case '_':
    case '.':
      return true;
    default:
      return false;
  }
}

AK_HIDE
AkImage*
dae_bugfix_texture_image_by_ref(DAEState   * __restrict dst,
                                const char * __restrict ref) {
  AkImage *image;
  AkImage *match;

  if (!dst || !dst->doc || !ref || !*ref)
    return NULL;

  /* Exact COLLADA ID resolution belongs to the caller. This fallback only
     accepts one unambiguous exporter-mangled ID/name candidate. */
  match = NULL;
  for (image = dst->doc->lib.images.first; image; image = image->next) {
    if (!dae_bugfix_texture_ref_key(ak_getId(image), ref)
        && !dae_bugfix_texture_ref_key(image->name, ref))
      continue;

    if (match && match != image)
      return NULL;
    match = image;
  }

  return match;
}

static
bool
dae_bugfix_texture_local_file(AkDoc       * __restrict doc,
                              const char  * __restrict uri,
                              char        * __restrict pathbuf,
                              size_t                   pathbuflen) {
  char        decoded[PATH_MAX];
  const char *filePath;
  const char *path;
  struct stat st;
  size_t      pathLen;

  if (!uri || !*uri || !pathbuf || pathbuflen == 0)
    return false;

  path = uri;
  if (ak_str_has_char_fast(uri, '%')) {
    if (!io_uri_decode_path(uri, decoded, sizeof(decoded)))
      return false;
    path = decoded;
  }

  filePath = io_uri_file_path(path,
                              IO_URI_FILE_PREFIX,
                              IO_URI_FILE_PREFIX_LEN);
  if (filePath) {
    path = filePath;
  } else if (io_uri_has_scheme(path)) {
    return false;
  } else if (!io_path_is_abs_drive_colon(path)) {
    path = ak_fullpathn(doc, path, pathbuf, pathbuflen);
    if (!path)
      return false;
  }

  pathLen = strlen(path);
  if (pathLen >= pathbuflen)
    return false;
  if (path != pathbuf)
    memmove(pathbuf, path, pathLen + 1u);

  return stat(pathbuf, &st) == 0;
}

static
bool
dae_bugfix_texture_key_path(DAEState   * __restrict dst,
                            const char * __restrict sourceUri,
                            const char * __restrict key,
                            char       * __restrict resolved,
                            size_t                  resolvedLen) {
  char        candidate[PATH_MAX];
  char        decoded[PATH_MAX];
  const char *base;
  const char *dot;
  const char *ext;
  const char *slash;
  const char *backslash;
  size_t      dirLen;
  size_t      extLen;
  size_t      keyLen;
  size_t      stemLen;

  if (!dst || !sourceUri || !key || !*key)
    return false;
  if (io_uri_has_scheme(sourceUri)
      || io_path_is_abs_drive_colon(sourceUri))
    return false;

  if (ak_str_has_char_fast(sourceUri, '%')) {
    if (!io_uri_decode_path(sourceUri, decoded, sizeof(decoded)))
      return false;
    sourceUri = decoded;
  }

  slash     = strrchr(sourceUri, '/');
  backslash = strrchr(sourceUri, '\\');
  base      = sourceUri;
  if (slash && (!backslash || slash > backslash))
    base = slash + 1;
  else if (backslash)
    base = backslash + 1;

  dot = strrchr(base, '.');
  if (!dot || dot == base || !dot[1])
    return false;

  ext     = dot + 1;
  extLen  = strlen(ext);
  keyLen  = strlen(key);
  dirLen  = (size_t)(base - sourceUri);
  if (keyLen <= extLen + 1u
      || (key[keyLen - extLen - 1u] != '_'
          && key[keyLen - extLen - 1u] != '.')
      || memcmp(key + keyLen - extLen, ext, extLen) != 0)
    return false;
  if (ak_str_has_char_fast(key, '/')
      || ak_str_has_char_fast(key, '\\')
      || ak_str_has_char_fast(key, ':'))
    return false;

  stemLen = keyLen - extLen - 1u;
  if (dirLen + stemLen + extLen + 2u > sizeof(candidate))
    return false;

  memcpy(candidate, sourceUri, dirLen);
  memcpy(candidate + dirLen, key, stemLen);
  candidate[dirLen + stemLen] = '.';
  memcpy(candidate + dirLen + stemLen + 1u, ext, extLen + 1u);

  return dae_bugfix_texture_local_file(dst->doc,
                                       candidate,
                                       resolved,
                                       resolvedLen);
}

AK_HIDE
void
dae_bugfix_texture_image_path(DAEState * __restrict dst,
                              AkImage  * __restrict image) {
  AkImageSource *source;
  const char    *keys[2];
  char           exactPath[PATH_MAX];
  char           candidatePath[PATH_MAX];
  char           matchPath[PATH_MAX];
  size_t         i;
  bool           matched;

  if (!dst || !image || !(source = image->source)
      || source->type != AK_IMAGE_SOURCE_URI
      || !source->uri || !*source->uri)
    return;
  if (source->resolvedPath
      && dae_bugfix_texture_local_file(dst->doc,
                                       source->resolvedPath,
                                       exactPath,
                                       sizeof(exactPath)))
    return;
  if (dae_bugfix_texture_local_file(dst->doc,
                                    source->uri,
                                    exactPath,
                                    sizeof(exactPath)))
    return;

  /* Preserve the authored URI. A recovered physical file is derived state,
     and conflicting ID/name candidates must not be guessed. */
  keys[0] = ak_getId(image);
  keys[1] = image->name;
  matched = false;
  for (i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
    if (!dae_bugfix_texture_key_path(dst,
                                     source->uri,
                                     keys[i],
                                     candidatePath,
                                     sizeof(candidatePath)))
      continue;
    if (matched && strcmp(matchPath, candidatePath) != 0)
      return;
    if (!matched) {
      memcpy(matchPath, candidatePath, strlen(candidatePath) + 1u);
      matched = true;
    }
  }

  if (matched)
    source->resolvedPath = ak_heap_strdup(dst->heap, source, matchPath);
}
