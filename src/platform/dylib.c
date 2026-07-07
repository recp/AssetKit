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

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include "dylib.h"

#ifdef AK_WINAPI
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif
#include <string.h>

static const char AK_DYLIB_ANCHOR = 0;

AK_HIDE
void*
ak_dylib_open(const char * __restrict path) {
  if (!path)
    return NULL;

#ifdef AK_WINAPI
  return (void *)LoadLibraryA(path);
#else
  return dlopen(path, RTLD_LAZY | RTLD_LOCAL);
#endif
}

static
bool
ak_dylib_make_name(char       * __restrict dst,
                   size_t                  cap,
                   const char * __restrict prefix,
                   const char * __restrict name,
                   const char * __restrict suffix) {
  size_t prefixLen;
  size_t nameLen;
  size_t suffixLen;
  size_t totalLen;
  char  *p;

  if (!dst || !prefix || !name || !suffix || cap == 0)
    return false;

  prefixLen = strlen(prefix);
  nameLen   = strlen(name);
  suffixLen = strlen(suffix);
  if (prefixLen > (size_t)-1 - nameLen
      || prefixLen + nameLen > (size_t)-1 - suffixLen)
    return false;

  totalLen = prefixLen + nameLen + suffixLen;
  if (totalLen >= cap)
    return false;

  p = dst;
  memcpy(p, prefix, prefixLen);
  p += prefixLen;
  memcpy(p, name, nameLen);
  p += nameLen;
  memcpy(p, suffix, suffixLen + 1u);

  return true;
}

static
void*
ak_dylib_openSibling(const char * __restrict file) {
  char   modpath[1024];
  char   path[1024];
  char  *sep;
  char  *sep2;
  size_t dirlen;
  size_t filelen;

  if (!file)
    return NULL;

#ifdef AK_WINAPI
  {
    HMODULE mod;
    DWORD   n;

    mod = NULL;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                            | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCSTR)&AK_DYLIB_ANCHOR,
                            &mod))
      return NULL;

    n = GetModuleFileNameA(mod, modpath, sizeof(modpath));
    if (n == 0 || n >= sizeof(modpath))
      return NULL;
  }
#else
  {
    Dl_info info;
    size_t  modlen;

    if (!dladdr((const void *)&AK_DYLIB_ANCHOR, &info) || !info.dli_fname)
      return NULL;

    modlen = strlen(info.dli_fname);
    if (modlen >= sizeof(modpath))
      return NULL;
    memcpy(modpath, info.dli_fname, modlen + 1u);
  }
#endif

  sep  = strrchr(modpath, '/');
  sep2 = strrchr(modpath, '\\');
  if (!sep || (sep2 && sep2 > sep))
    sep = sep2;
  if (!sep)
    return NULL;

  dirlen  = (size_t)(sep - modpath) + 1;
  filelen = strlen(file);
  if (dirlen + filelen >= sizeof(path))
    return NULL;

  memcpy(path, modpath, dirlen);
  memcpy(path + dirlen, file, filelen + 1);

  return ak_dylib_open(path);
}

AK_HIDE
void*
ak_dylib_openName(const char * __restrict name) {
  char path[256];
#if defined(__APPLE__)
  char rpath[256];
#endif
  void *lib;

  if (!name)
    return NULL;

#ifdef AK_WINAPI
  if (!ak_dylib_make_name(path, sizeof(path), "", name, ".dll"))
    return NULL;
#elif defined(__APPLE__)
  if (!ak_dylib_make_name(path, sizeof(path), "lib", name, ".dylib"))
    return NULL;
#else
  if (!ak_dylib_make_name(path, sizeof(path), "lib", name, ".so"))
    return NULL;
#endif

  if ((lib = ak_dylib_openSibling(path)))
    return lib;

#if defined(__APPLE__)
  if (ak_dylib_make_name(rpath, sizeof(rpath), "@rpath/lib", name, ".dylib")
      && (lib = ak_dylib_open(rpath)))
    return lib;
#endif

  return ak_dylib_open(path);
}

AK_HIDE
void*
ak_dylib_sym(void       * __restrict lib,
             const char * __restrict name) {
  if (!lib || !name)
    return NULL;

#ifdef AK_WINAPI
  return (void *)GetProcAddress((HMODULE)lib, name);
#else
  return dlsym(lib, name);
#endif
}

AK_HIDE
void
ak_dylib_close(void * __restrict lib) {
  if (!lib)
    return;

#ifdef AK_WINAPI
  FreeLibrary((HMODULE)lib);
#else
  dlclose(lib);
#endif
}
