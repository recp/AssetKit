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

#ifndef ak_array_h
#define ak_array_h

#include "../include/ak/assetkit.h"

AkResult _assetkit_hide
ak_strtostr_array(AkHeap         * __restrict heap,
                  void           * __restrict memParent,
                  char                       *content,
                  char                        separator,
                  AkStringArray ** __restrict array);

AkResult _assetkit_hide
ak_strtostr_arrayL(AkHeap          * __restrict heap,
                   void            * __restrict memParent,
                   char                        *content,
                   char                         separator,
                   AkStringArrayL ** __restrict array);

#endif /* ak_array_h */
