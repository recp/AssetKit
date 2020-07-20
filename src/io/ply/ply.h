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

#ifndef ply_h
#define ply_h

#include "common.h"

AkResult AK_HIDE
ply_ply(AkDoc     ** __restrict dest,
        const char * __restrict filepath);

AK_HIDE
void
ply_ascii(char * __restrict src, PLYState * __restrict pst);

AK_HIDE
void
ply_bin_le(PLYState * __restrict pst);

AK_HIDE
void
ply_bin_be(PLYState * __restrict pst);

#endif /* stl_h */
