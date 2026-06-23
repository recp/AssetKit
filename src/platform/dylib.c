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
#include <stdio.h>

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
void*
ak_dylib_openSibling(const char * __restrict file) {
  char   modpath[1024];
  char   path[1024];
  char  *sep;
  char  *sep2;
  size_t dirlen;
  size_t filelen;
#ifndef AK_WINAPI
  int    len;
#endif

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

    if (!dladdr((const void *)&AK_DYLIB_ANCHOR, &info) || !info.dli_fname)
      return NULL;

    len = snprintf(modpath, sizeof(modpath), "%s", info.dli_fname);
    if (len <= 0 || (size_t)len >= sizeof(modpath))
      return NULL;
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
  int  len;

  if (!name)
    return NULL;

#ifdef AK_WINAPI
  len = snprintf(path, sizeof(path), "%s.dll", name);
#elif defined(__APPLE__)
  len = snprintf(path, sizeof(path), "lib%s.dylib", name);
#else
  len = snprintf(path, sizeof(path), "lib%s.so", name);
#endif

  if (len <= 0 || (size_t)len >= sizeof(path))
    return NULL;

  if ((lib = ak_dylib_openSibling(path)))
    return lib;

#if defined(__APPLE__)
  len = snprintf(rpath, sizeof(rpath), "@rpath/lib%s.dylib", name);
  if (len > 0 && (size_t)len < sizeof(rpath)
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
