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

#include "asset.h"

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
}
