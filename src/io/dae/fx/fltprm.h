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

#ifndef dae_fltprm_h
#define dae_fltprm_h

#include "../common.h"

//AK_HIDE AkFloatOrParam*
//dae_floatOrParam(DAEState * __restrict dst,
//                 xml_t    * __restrict xml,
//                 void     * __restrict memp);

AK_HIDE
float
dae_float(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp,
          size_t                off,
          float                 defaultVal);

#endif /* dae_fltprm_h */
