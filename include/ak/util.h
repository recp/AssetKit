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

#ifndef assetkit_util_h
#define assetkit_util_h

#include "common.h"

/* pre-defined compare funcs */

AK_EXPORT
int
ak_cmp_str(void *key1, void *key2);

AK_EXPORT
int
ak_cmp_ptr(void *key1, void *key2);

AK_EXPORT
int
ak_digitsize(size_t number);

#endif /* assetkit_util_h */
