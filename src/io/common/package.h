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

#ifndef assetkit_io_common_package_h
#define assetkit_io_common_package_h

#include "../../common.h"

AK_HIDE
AkResult
ak_zip_package_doc(AkDoc      ** __restrict dest,
                   const char  * __restrict filepath);

#endif /* assetkit_io_common_package_h */
