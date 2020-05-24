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

#ifndef dae_value_h
#define dae_value_h

#include "../common.h"

AkValue* _assetkit_hide
dae_value(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp);

void _assetkit_hide
dae_dtype(const char *typeName, AkTypeDesc *type);

#endif /* dae_value_h */
