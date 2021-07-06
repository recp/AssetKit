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

#ifndef assetkit_context_h
#define assetkit_context_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "map.h"
#include "util.h"

typedef struct AkContext {
  AkDoc              *doc;
  AkTechniqueHint    *techniqueHint;
  AkInstanceMaterial *instanceMaterial;
  //  AkMap              *bindVertexInput;
} AkContext;

#ifdef __cplusplus
}
#endif
#endif /* assetkit_context_h */
