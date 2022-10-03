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

#include "profile.h"

AK_HIDE
AkProfileCommon*
gltf_cmnEffect(AkGLTFState * __restrict gst) {
  AkLibrary       *lib;
  AkEffect        *effect;
  AkProfileCommon *profile;

  if (!(lib = gst->doc->lib.effects)) {
    lib = ak_heap_calloc(gst->heap, gst->doc, sizeof(*lib));
    gst->doc->lib.effects = lib;
  }

  effect  = ak_heap_calloc(gst->heap, lib,      sizeof(*effect));
  profile = ak_heap_calloc(gst->heap, effect,   sizeof(*profile));
  profile->type = AK_PROFILE_TYPE_COMMON;

  lib->count++;

  effect->profile = profile;
  effect->next    = (void *)lib->chld;
  lib->chld       = (void *)effect;

  ak_setypeid(profile, AKT_PROFILE);
  ak_setypeid(effect,  AKT_EFFECT);

  return effect->profile;
}
