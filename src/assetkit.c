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
#include "io/obj/obj.h"
#include "io/stl/stl.h"
#include "io/ply/ply.h"
#include "io/3mf/3mf.h"
#include "io/common/package.h"

#ifndef AK_BUILD_EXPORTERS
#  define AK_BUILD_EXPORTERS 1
#endif
#ifndef AK_BUILD_DAE_EXPORTER
#  define AK_BUILD_DAE_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_GLTF_EXPORTER
#  define AK_BUILD_GLTF_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_OBJ_EXPORTER
#  define AK_BUILD_OBJ_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_STL_EXPORTER
#  define AK_BUILD_STL_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_PLY_EXPORTER
#  define AK_BUILD_PLY_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_3MF_EXPORTER
#  define AK_BUILD_3MF_EXPORTER AK_BUILD_EXPORTERS
#endif

#if AK_BUILD_DAE_EXPORTER
#  include "io/dae/exp/dae.h"
#endif
#if AK_BUILD_GLTF_EXPORTER
#  include "io/gltf/exp/gltf.h"
#endif
#if AK_BUILD_OBJ_EXPORTER
#  include "io/obj/exp/obj.h"
#endif

#if AK_BUILD_DAE_EXPORTER || AK_BUILD_GLTF_EXPORTER || AK_BUILD_OBJ_EXPORTER \
    || AK_BUILD_STL_EXPORTER || AK_BUILD_PLY_EXPORTER || AK_BUILD_3MF_EXPORTER
#  define AK_BUILD_ANY_EXPORTER 1
#else
#  define AK_BUILD_ANY_EXPORTER 0
#endif

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

#if AK_BUILD_ANY_EXPORTER
typedef struct {
  AkResult (*fexporter_fn)(AkDoc * __restrict, const char * __restrict);
  const char * fileExt;
} fexporter_t;

static
AkFileType
ak_export_file_type_from_path(const char * __restrict path) {
  const char *base;
  const char *slash;
  const char *backslash;
  const char *ext;

  if (!path)
    return AK_FILE_TYPE_AUTO;

  slash     = strrchr(path, '/');
  backslash = strrchr(path, '\\');
  if (slash && backslash)
    base = slash > backslash ? slash + 1 : backslash + 1;
  else if (slash)
    base = slash + 1;
  else if (backslash)
    base = backslash + 1;
  else
    base = path;

  ext = strrchr(base, '.');
  if (!ext || ext == base || ext[1] == '\0')
    return AK_FILE_TYPE_AUTO;
  ext++;

  if (ak_ascii_streq_ci(ext, "dae") || ak_ascii_streq_ci(ext, "collada"))
    return AK_FILE_TYPE_DAE;
  if (ak_ascii_streq_ci(ext, "gltf"))
    return AK_FILE_TYPE_GLTF;
  if (ak_ascii_streq_ci(ext, "glb"))
    return AK_FILE_TYPE_GLB;
  if (ak_ascii_streq_ci(ext, "obj"))
    return AK_FILE_TYPE_OBJ;
  if (ak_ascii_streq_ci(ext, "stl"))
    return AK_FILE_TYPE_STL;
  if (ak_ascii_streq_ci(ext, "ply"))
    return AK_FILE_TYPE_PLY;
  if (ak_ascii_streq_ci(ext, "3mf"))
    return AK_FILE_TYPE_3MF;

  return AK_FILE_TYPE_AUTO;
}

static
fexporter_t
ak_exporter_for_type(AkFileType fileType) {
  fexporter_t fexporter;

  fexporter.fexporter_fn = NULL;
  fexporter.fileExt      = NULL;

  if (fileType == AK_FILE_TYPE_AUTO) {
#if AK_BUILD_GLTF_EXPORTER
    fexporter.fexporter_fn = gltf_export;
    fexporter.fileExt      = "gltf";
#elif AK_BUILD_DAE_EXPORTER
    fexporter.fexporter_fn = dae_export;
    fexporter.fileExt      = "dae";
#elif AK_BUILD_OBJ_EXPORTER
    fexporter.fexporter_fn = wobj_export;
    fexporter.fileExt      = "obj";
#elif AK_BUILD_3MF_EXPORTER
    fexporter.fexporter_fn = ak_3mf_export;
    fexporter.fileExt      = "3mf";
#elif AK_BUILD_STL_EXPORTER
    fexporter.fexporter_fn = stl_export;
    fexporter.fileExt      = "stl";
#elif AK_BUILD_PLY_EXPORTER
    fexporter.fexporter_fn = ply_export;
    fexporter.fileExt      = "ply";
#endif
    return fexporter;
  }

  switch (fileType) {
#if AK_BUILD_DAE_EXPORTER
    case AK_FILE_TYPE_COLLADA:
      fexporter.fexporter_fn = dae_export;
      fexporter.fileExt      = "dae";
      break;
#endif
#if AK_BUILD_GLTF_EXPORTER
    case AK_FILE_TYPE_GLTF:
      fexporter.fexporter_fn = gltf_export;
      fexporter.fileExt      = "gltf";
      break;
    case AK_FILE_TYPE_GLB:
      fexporter.fexporter_fn = gltf_export_glb;
      fexporter.fileExt      = "glb";
      break;
#endif
#if AK_BUILD_OBJ_EXPORTER
    case AK_FILE_TYPE_WAVEFRONT:
      fexporter.fexporter_fn = wobj_export;
      fexporter.fileExt      = "obj";
      break;
#endif
#if AK_BUILD_STL_EXPORTER
    case AK_FILE_TYPE_STL:
      fexporter.fexporter_fn = stl_export;
      fexporter.fileExt      = "stl";
      break;
#endif
#if AK_BUILD_PLY_EXPORTER
    case AK_FILE_TYPE_PLY:
      fexporter.fexporter_fn = ply_export;
      fexporter.fileExt      = "ply";
      break;
#endif
#if AK_BUILD_3MF_EXPORTER
    case AK_FILE_TYPE_3MF:
      fexporter.fexporter_fn = ak_3mf_export;
      fexporter.fileExt      = "3mf";
      break;
#endif
    default:
      break;
  }

  return fexporter;
}

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

  if (!dir || stat(dir, &st) != 0)
    return false;

#ifdef _WIN32
  return (st.st_mode & _S_IFDIR) != 0;
#else
  return S_ISDIR(st.st_mode);
#endif
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

static
bool
ak_export_prepare_file_dir(const char * __restrict outPath) {
  const char *slash;
  const char *backslash;
  const char *sep;
  char       *dir;
  size_t      len;
  bool        ok;

  if (!outPath || !*outPath)
    return false;

  slash     = strrchr(outPath, '/');
  backslash = strrchr(outPath, '\\');
  if (slash && backslash)
    sep = slash > backslash ? slash : backslash;
  else
    sep = slash ? slash : backslash;

  if (!sep)
    return true;

  len = (size_t)(sep - outPath);
  if (len == 0)
    len = 1u;
#ifdef _WIN32
  if (len == 2u && outPath[1] == ':')
    len = 3u;
#endif

  dir = malloc(len + 1u);
  if (!dir)
    return false;

  memcpy(dir, outPath, len);
  dir[len] = '\0';
  ok = ak_export_prepare_dir(dir);
  free(dir);

  return ok;
}
#endif

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
    {"zae",  dae_archive_doc},
    {"kmz",  dae_archive_doc},
    {"zip",  ak_zip_package_doc},
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
        const char *file_ext;

        file_ext = strrchr(localurl, '.');
        if (file_ext
            && (ak_ascii_streq_ci(file_ext + 1, "zae")
                || ak_ascii_streq_ci(file_ext + 1, "kmz")
                || ak_ascii_streq_ci(file_ext + 1, "zip"))) {
          floader = &floaders[7];
        } else {
          floader = &floaders[0];
        }
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
#if AK_BUILD_ANY_EXPORTER
  fexporter_t fexporter;
  char       *fileName;
  char       *outPath;
  AkResult    result;

  if (!doc || !outDir)
    return AK_ERR;

  if (!ak_export_prepare_dir(outDir))
    return AK_EBADF;

  fexporter = ak_exporter_for_type(fileType);
  if (!fexporter.fexporter_fn)
    return AK_ERR;

  fileName = ak_export_output_file_name(doc, fexporter.fileExt);
  if (!fileName)
    return AK_ERR;

  outPath = ak_export_output_path(outDir, fileName);
  free(fileName);
  if (!outPath)
    return AK_ERR;

  result = fexporter.fexporter_fn(doc, outPath);
  free(outPath);

  return result;
#else
  (void)doc;
  (void)outDir;
  (void)fileType;

  return AK_ERR;
#endif
}

AK_EXPORT
AkResult
ak_exportFile(AkDoc * __restrict doc, const char * __restrict outPath,
              AkFileType fileType) {
#if AK_BUILD_ANY_EXPORTER
  fexporter_t fexporter;

  if (!doc || !outPath || !*outPath)
    return AK_ERR;

  if (fileType == AK_FILE_TYPE_AUTO)
    fileType = ak_export_file_type_from_path(outPath);

  fexporter = ak_exporter_for_type(fileType);
  if (!fexporter.fexporter_fn)
    return AK_ERR;

  if (!ak_export_prepare_file_dir(outPath))
    return AK_EBADF;

  return fexporter.fexporter_fn(doc, outPath);
#else
  (void)doc;
  (void)outPath;
  (void)fileType;

  return AK_ERR;
#endif
}

AK_EXPORT
AkResult
ak_convert(const char * __restrict inputPath,
           const char * __restrict outputPath,
           AkFileType              outputType) {
  AkDoc   *doc;
  AkResult result;

  if (!inputPath || !outputPath)
    return AK_ERR;

  doc = NULL;
  result = ak_load(&doc, inputPath, AK_FILE_TYPE_AUTO);
  if (result != AK_OK || !doc)
    return result == AK_OK ? AK_ERR : result;

  result = ak_exportFile(doc, outputPath, outputType);
  ak_free(doc);

  return result;
}
