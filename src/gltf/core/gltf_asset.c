/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_asset.h"

void
gltf_asset(json_t * __restrict json,
           void   * __restrict userdata) {
  AkGLTFState   *gst;
  AkAssetInf    *inf;
  AkDoc         *doc;
  AkHeap        *heap;
  AkContributor *contrib;

  gst  = userdata;
  heap = gst->heap;
  doc  = gst->doc;
  inf  = &doc->inf->base;

  contrib          = ak_heap_calloc(heap, inf, sizeof(*contrib));
  inf->contributor = contrib;

  json = json->value;
  while (json) {
    if (json_key_eq(json, _s_gltf_version)) {
      if (json_float(json, 0.0f) < 2.0f) {
        gst->stop = true;
        return; /* unsupported version */
      }
    } else if (json_key_eq(json, _s_gltf_copyright)) {
      contrib->copyright = json_strdup(json, heap, contrib);
    } else if (json_key_eq(json, _s_gltf_generator)) {
      contrib->authoringTool = json_strdup(json, heap, contrib);
    }

    json = json->next;
  }
  
  /* glTF default definitions */

  /* CoordSys is Y_UP */
  inf->coordSys = AK_YUP;

  /* Unit is meter */
  inf->unit       = ak_heap_calloc(heap, inf, sizeof(*inf->unit));
  inf->unit->dist = 1.0;
  inf->unit->name = ak_heap_strdup(heap, inf->unit, _s_gltf_meter);

  *(AkAssetInf **)ak_heap_ext_add(heap,
                                  ak__alignof(doc),
                                  AK_HEAP_NODE_FLAGS_INF) = inf;

  doc->coordSys = inf->coordSys;
  doc->unit     = inf->unit;

  return;
}
