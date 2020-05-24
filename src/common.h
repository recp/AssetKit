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

#ifndef ak_common_h
#define ak_common_h

#define AK_INTERN

#include "../include/ak/assetkit.h"
#include "../include/ak/options.h"
#include "../include/ak/map.h"
#include "../include/ak/type.h"
#include "../include/ak/url.h"

#include "mem/common.h"

#include <cglm/cglm.h>

#include <ds/rb.h>
#include <ds/forward-list.h>
#include <ds/forward-list-sep.h>

#include <stddef.h>
#include <sys/types.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _MSC_VER
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <io.h>

#  define strncasecmp _strnicmp
#  define strcasecmp  _stricmp
#  define strtok_r    strtok_s
#  define mktemp      _mktemp 
#endif

#ifdef __GNUC__
#  define AK_DESTRUCTOR __attribute__((destructor))
#  define AK_CONSTRUCTOR __attribute__((constructor))
#else
#  define AK_DESTRUCTOR
#  define AK_CONSTRUCTOR
#endif

#define AK__UNUSED(X) (void)X

#define I2P (void *)(intptr_t)

/*!
 * @brief get sign of 32 bit integer as +1 or -1
 *
 * @param X integer value
 */
#define AK_GET_SIGN(X) ((X >> 31) - (-X >> 31))

#define AK_ARRAY_SEP_CHECK (c == ' ' || c == '\n' || c == '\t' \
                              || c == '\r' || c == '\f' || c == '\v')

#define AK_ARRAY_SEPLINE_CHECK (c == ' ' || c == '\t'  || c == '\f' || c == '\v')
#define AK_ARRAY_SPACE_CHECK (c == ' ' || c == '\t' || c == '\f' || c == '\v')
#define AK_ARRAY_NLINE_CHECK (c == '\n' || c == '\r')

typedef struct ak_enumpair {
  const char *key;
  AkEnum      val;
} ak_enumpair;

typedef struct {
  const char * name;
  AkEnum       val;
} dae_enum;

int _assetkit_hide
ak_enumpair_cmp(const void * a, const void * b);

int _assetkit_hide
ak_enumpair_cmp2(const void * a, const void * b);

int _assetkit_hide
ak_enumpair_json_val_cmp(const void * a, const void * b);

AK_EXPORT
int
ak_cmp_str(void * key1, void *key2);

AK_EXPORT
int
ak_cmp_ptr(void *key1, void *key2);

AK_EXPORT
int
ak_cmp_i32(void *key1, void *key2);

AK_EXPORT
int
ak_cmp_vec3(void * key1, void *key2);

AK_EXPORT
int
ak_cmp_vec4(void *key1, void *key2);

typedef int (*AkCmpFn)(void * key1, void *key2);

#endif /* ak_common_h */
