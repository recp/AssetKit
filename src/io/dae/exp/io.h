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

#ifndef assetkit_dae_exp_io_h
#define assetkit_dae_exp_io_h

#include "dae.h"

#include <stdbool.h>
#include <stddef.h>

AK_HIDE
bool
dae_uri_rel_safe(const char * __restrict uri);

AK_HIDE
bool
dae_write_file_bytes(const char * __restrict dst,
                     const void * __restrict data,
                     size_t                  len);

#endif /* assetkit_dae_exp_io_h */
