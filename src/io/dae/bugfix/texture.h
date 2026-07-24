/*
 * Copyright (C) 2026 Recep Aslantas
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

#ifndef dae_bugfix_texture_h
#define dae_bugfix_texture_h

#include "../common.h"

AK_HIDE
AkImage*
dae_bugfix_texture_image_by_ref(DAEState   * __restrict dst,
                                const char * __restrict ref);

AK_HIDE
void
dae_bugfix_texture_image_path(DAEState * __restrict dst,
                              AkImage  * __restrict image);

#endif /* dae_bugfix_texture_h */
