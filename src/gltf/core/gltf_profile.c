/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_profile.h"

AkProfileCommon* _assetkit_hide
gltf_cmnEffect(AkGLTFState * __restrict gst) {
  AkLibItem       *lib;
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
  effect->next    = lib->chld;
  lib->chld       = effect;

  ak_setypeid(profile, AKT_PROFILE);
  ak_setypeid(effect,  AKT_EFFECT);

  return effect->profile;
}
