/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_morph.h"
#include "gltf_accessor.h"

void _assetkit_hide
gltf_morph(AkGLTFState * __restrict gst) {
  AkHeap       *heap;
  AkDoc        *doc;
  json_t       *jskins, *jaccessors;
  AkLibItem    *lib;
  size_t        jskinCount, i;

  heap       = gst->heap;
  doc        = gst->doc;
  lib        = ak_heap_calloc(heap, doc, sizeof(*lib));

  jskins     = json_object_get(gst->root, _s_gltf_skins);
  jskinCount = (int32_t)json_array_size(jskins);
  jaccessors = json_object_get(gst->root, _s_gltf_accessors);

}
