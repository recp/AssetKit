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

#include "../include/ak/assetkit.h"

#include "utils.h"
#include "io/dae/dae.h"
#include "io/gltf/imp/gltf.h"
#include "io/gltf/exp/gltf.h"
#include "io/obj/obj.h"
#include "io/stl/stl.h"
#include "io/ply/ply.h"
#include "io/3mf/imp/3mf.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#ifdef _WIN32
#  include <direct.h>
#endif

typedef struct {
  const char * fext;
  AkResult (*floader_fn)(AkDoc ** __restrict, const char * __restrict);
} floader_t;

typedef struct {
  AkResult (*fexporter_fn)(AkDoc * __restrict, const char * __restrict);
  const char * fileExt;
} fexporter_t;

static
int
ak_export_mkdir(const char * __restrict path) {
#ifdef _WIN32
  return _mkdir(path);
#else
  return mkdir(path, 0777);
#endif
}

static
bool
ak_export_is_dir(const char * __restrict dir) {
  struct stat st;

  return dir && stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
}

static
bool
ak_export_mkdirs(char * __restrict dir) {
  char *it;
  char  saved;

  if (!dir || !*dir)
    return false;

  for (it = dir; *it; it++) {
    if (*it != '/' && *it != '\\')
      continue;

    if (it == dir)
      continue;

#ifdef _WIN32
    if (it == dir + 2 && dir[1] == ':')
      continue;
#endif

    saved = *it;
    *it   = '\0';
    if (*dir
        && !ak_export_is_dir(dir)
        && ak_export_mkdir(dir) != 0
        && errno != EEXIST) {
      *it = saved;
      return false;
    }
    *it = saved;
  }

  if (!ak_export_is_dir(dir)
      && ak_export_mkdir(dir) != 0
      && errno != EEXIST)
    return false;

  return ak_export_is_dir(dir);
}

static
bool
ak_export_prepare_dir(const char * __restrict dir) {
  char  *path;
  size_t len;
  bool   ok;

  if (!dir || !*dir)
    return false;

  if (ak_export_is_dir(dir))
    return true;

  len = strlen(dir);
  while (len > 1u && (dir[len - 1] == '/' || dir[len - 1] == '\\'))
    len--;

  path = malloc(len + 1u);
  if (!path)
    return false;

  memcpy(path, dir, len);
  path[len] = '\0';

  ok = ak_export_mkdirs(path);
  free(path);

  return ok;
}

static
bool
ak_export_filename_char_ok(unsigned char ch) {
  return ch >= 0x20
         && ch != '/'
         && ch != '\\'
         && ch != ':'
         && ch != '*'
         && ch != '?'
         && ch != '"'
         && ch != '<'
         && ch != '>'
         && ch != '|';
}

static
bool
ak_ascii_streq_ci(const char * __restrict a, const char * __restrict b) {
  unsigned char ca;
  unsigned char cb;

  if (!a || !b)
    return false;

  while (*a && *b) {
    ca = (unsigned char)*a++;
    cb = (unsigned char)*b++;

    if (ca >= 'A' && ca <= 'Z')
      ca = (unsigned char)(ca + ('a' - 'A'));
    if (cb >= 'A' && cb <= 'Z')
      cb = (unsigned char)(cb + ('a' - 'A'));

    if (ca != cb)
      return false;
  }

  return *a == '\0' && *b == '\0';
}

static
const char*
ak_export_name_source(AkDoc * __restrict doc) {
  if (!doc)
    return NULL;

  if (doc->inf && doc->inf->name && *doc->inf->name)
    return doc->inf->name;

  if (doc->inf && doc->inf->base.title && *doc->inf->base.title)
    return doc->inf->base.title;

  if (doc->scene && doc->scene->name && *doc->scene->name)
    return doc->scene->name;

  return NULL;
}

static
char*
ak_export_output_file_name(AkDoc       * __restrict doc,
                           const char  * __restrict fileExt) {
  const char *src;
  const char *base;
  const char *slash;
  const char *backslash;
  const char *dot;
  char       *fileName;
  size_t      stemLen;
  size_t      extLen;
  size_t      outLen;
  size_t      i;
  bool        any;

  if (!fileExt || !*fileExt)
    return NULL;

  src = ak_export_name_source(doc);
  if (!src || !*src)
    src = "model";

  slash     = strrchr(src, '/');
  backslash = strrchr(src, '\\');
  if (slash && backslash)
    base = slash > backslash ? slash + 1 : backslash + 1;
  else if (slash)
    base = slash + 1;
  else if (backslash)
    base = backslash + 1;
  else
    base = src;

  if (!*base)
    base = "model";

  dot = strrchr(base, '.');
  stemLen = dot && dot != base ? (size_t)(dot - base) : strlen(base);
  while (stemLen > 0 && (base[stemLen - 1] == ' ' || base[stemLen - 1] == '.'))
    stemLen--;
  if (stemLen == 0) {
    base    = "model";
    stemLen = 5u;
  }

  any = false;
  for (i = 0; i < stemLen; i++) {
    unsigned char ch;

    ch = (unsigned char)base[i];
    if (ak_export_filename_char_ok(ch)) {
      if (ch != ' ')
        any = true;
    } else {
      any = true;
    }
  }

  if (!any) {
    base    = "model";
    stemLen = 5u;
  }

  extLen = strlen(fileExt);
  if (stemLen > (size_t)-1 - extLen - 2u)
    return NULL;

  outLen = stemLen + extLen + 1u;
  fileName = malloc(outLen + 1u);
  if (!fileName)
    return NULL;

  for (i = 0; i < stemLen; i++) {
    unsigned char ch;

    ch = (unsigned char)base[i];
    if (ak_export_filename_char_ok(ch)) {
      fileName[i] = (char)ch;
    } else {
      fileName[i] = '_';
    }
  }

  fileName[stemLen] = '.';
  memcpy(fileName + stemLen + 1u, fileExt, extLen);
  fileName[stemLen + 1u + extLen] = '\0';

  return fileName;
}

static
char*
ak_export_output_path(const char * __restrict dir,
                      const char * __restrict fileName) {
  char  *path;
  size_t dirLen;
  size_t fileNameLen;
  size_t totalLen;
  bool   needSlash;

  if (!dir || !fileName)
    return NULL;

  dirLen = strlen(dir);
  while (dirLen > 0
         && (dir[dirLen - 1] == '/' || dir[dirLen - 1] == '\\'))
    dirLen--;

  fileNameLen = strlen(fileName);
  needSlash   = dirLen > 0;

  if (dirLen > (size_t)-1 - fileNameLen - (needSlash ? 2u : 1u))
    return NULL;

  totalLen = dirLen + fileNameLen + (needSlash ? 1u : 0u);
  path = malloc(totalLen + 1u);
  if (!path)
    return NULL;

  if (dirLen > 0)
    memcpy(path, dir, dirLen);
  if (needSlash)
    path[dirLen++] = '/';
  memcpy(path + dirLen, fileName, fileNameLen);
  path[totalLen] = '\0';

  return path;
}

AK_EXPORT
AkResult
ak_load(AkDoc ** __restrict dest, const char * __restrict url, ...) {
  floader_t  *floader;
  const char *localurl;
  int         file_type;
  int         _err_no;

  va_list pref_args;
  va_start(pref_args, url);
  file_type = va_arg(pref_args, int);
  va_end(pref_args);

  localurl = ak_getFile(url);
  if (!localurl)
    return AK_EBADF;

  floader_t floaders[] = {
    {"dae",  dae_doc},
    {"gltf", gltf_gltf},
    {"glb",  gltf_glb},
    {"obj",  wobj_obj},
    {"stl",  stl_stl},
    {"ply",  ply_ply},
    {"3mf",  imp_3mf},
  };

  floader = NULL;

  if (file_type == AK_FILE_TYPE_AUTO) {
    char * file_ext;
    file_ext = strrchr(localurl, '.');
    if (file_ext) {
      int floader_len;
      int i;

      ++file_ext;
      floader_len = AK_ARRAY_LEN(floaders);
      for (i = 0; i < floader_len; i++) {
        if (ak_ascii_streq_ci(file_ext, floaders[i].fext)) {
          floader = &floaders[i];
          break;
        }
      }
    } else {
      /* TODO */
    }
  } else {
    switch (file_type) {
      case AK_FILE_TYPE_COLLADA: {
        floader = &floaders[0];
        break;
      }
      case AK_FILE_TYPE_GLTF: {
        floader = &floaders[1];
        break;
      }
      case AK_FILE_TYPE_GLB: {
        floader = &floaders[2];
        break;
      }
      case AK_FILE_TYPE_WAVEFRONT:
        floader = &floaders[3];
        break;
      case AK_FILE_TYPE_STL:
        floader = &floaders[4];
        break;
      case AK_FILE_TYPE_PLY:
        floader = &floaders[5];
        break;
      case AK_FILE_TYPE_3MF:
        floader = &floaders[6];
        break;
      default:
        *dest = NULL;
        break;
    }
  }

  if (floader)
    _err_no = floader->floader_fn(dest, localurl);
  else
    goto err;

  return _err_no;
err:
  *dest = NULL;
  return AK_ERR;
}

AK_EXPORT
AkResult
ak_export(AkDoc * __restrict doc, const char * __restrict outDir,
          AkFileType fileType) {
  fexporter_t *fexporter;
  char        *fileName;
  char        *outPath;
  AkResult     result;

  if (!doc || !outDir)
    return AK_ERR;

  if (!ak_export_prepare_dir(outDir))
    return AK_EBADF;

  fexporter_t fexporters[] = {
    {gltf_export,     "gltf"},
    {gltf_export_glb, "glb"},
  };

  fexporter = NULL;

  if (fileType == AK_FILE_TYPE_AUTO) {
    fexporter = &fexporters[0];
  } else {
    switch (fileType) {
      case AK_FILE_TYPE_GLTF:
        fexporter = &fexporters[0];
        break;
      case AK_FILE_TYPE_GLB:
        fexporter = &fexporters[1];
        break;
      default:
        break;
    }
  }

  if (!fexporter)
    return AK_ERR;

  fileName = ak_export_output_file_name(doc, fexporter->fileExt);
  if (!fileName)
    return AK_ERR;

  outPath = ak_export_output_path(outDir, fileName);
  free(fileName);
  if (!outPath)
    return AK_ERR;

  result = fexporter->fexporter_fn(doc, outPath);
  free(outPath);

  return result;
}
