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

#include "dylib.h"

#ifdef AK_WINAPI
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

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
