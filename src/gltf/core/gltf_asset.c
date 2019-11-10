/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_asset.h"

static ak_enumpair gltfVersions[] = {
  {"1.0", AK_GLTF_VERSION_10},
  {"2.0", AK_GLTF_VERSION_20}
};

static size_t gltfVersionsLen = 0;

void
gltf_asset(json_t * __restrict json,
           void   * __restrict userdata) {
  AkGLTFState       *gst;
  AkAssetInf        *inf;
  AkDoc             *doc;
  AkHeap            *heap;
  AkContributor     *contrib;
  const ak_enumpair *foundVersion;

  gst  = userdata;
  heap = gst->heap;
  doc  = gst->doc;
  inf  = &doc->inf->base;

 /* instead of keeping version as string we keep enum/int to fast-comparing */
  if (gltfVersionsLen == 0) {
    gltfVersionsLen = AK_ARRAY_LEN(gltfVersions);
    qsort(gltfVersions,
          gltfVersionsLen,
          sizeof(gltfVersions[0]),
          ak_enumpair_cmp);
  }

  contrib          = ak_heap_calloc(heap, inf, sizeof(*contrib));
  inf->contributor = contrib;

  json = json->value;
  while (json) {
    if (json_key_eq(json, _s_gltf_version)) {
      if (!(foundVersion = bsearch(json,
                                   gltfVersions,
                                   gltfVersionsLen,
                                   sizeof(gltfVersions[0]),
                                   ak_enumpair_json_val_cmp))) {
        gst->stop = true;
        return; /* unsupported version */
      }

      gst->version = foundVersion->val;
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
