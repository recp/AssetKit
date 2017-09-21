/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_profile.h"

void _assetkit_hide
gltf_setprofile(AkGLTFState * __restrict gst) {
  AkLibItem     *lib;
  AkEffect      *effect;
  AkProfileGLTF *profile;

  if (gst->doc->lib.effects)
    return;

  lib     = ak_heap_calloc(gst->heap, gst->doc, sizeof(*lib));
  effect  = ak_heap_calloc(gst->heap, lib,      sizeof(*effect));
  profile = ak_heap_calloc(gst->heap, effect,   sizeof(*profile));
  profile->type = AK_PROFILE_TYPE_GLTF;

  effect->profile = profile;
  lib->count      = 1;
  lib->chld       = effect;

  gst->doc->lib.effects = lib;
}

AkProfileGLTF* _assetkit_hide
gltf_profile(AkGLTFState * __restrict gst) {
  AkEffect *effect;

  if (!gst->doc->lib.effects)
    gltf_setprofile(gst);

  effect = gst->doc->lib.effects->chld;

  return effect->profile;
}
