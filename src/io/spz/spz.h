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

#ifndef ak_io_spz_h
#define ak_io_spz_h

#include "../../common.h"
#include "../../../include/ak/assetkit.h"

/* Shared by standalone SPZ and the glTF compression extension. */
struct AkGLTFSPZLib {
  void                   *lib;
  AkGaussianSplatDecoder  decoder;
};

AK_HIDE
bool
spz_decoderOpen(struct AkGLTFSPZLib *sp);

AK_HIDE
void
spz_decoderClose(struct AkGLTFSPZLib *sp);

AK_HIDE
AkResult
spz_doc(AkDoc **dest, const char *filepath);

#endif
