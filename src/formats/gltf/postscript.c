/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "postscript.h"
#include "mesh_fixup.h"

void _assetkit_hide
gltf_postscript(AkGLTFState * __restrict gst) {
  gltf_mesh_fixup(gst);
}
