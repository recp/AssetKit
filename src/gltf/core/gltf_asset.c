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

AkResult _assetkit_hide
gltf_asset(AkGLTFState  * __restrict gst,
           void         * __restrict memParent,
           const json_t * __restrict json,
           AkAssetInf   * __restrict dest) {
  AkHeap            *heap;
  AkContributor     *contrib;
  const ak_enumpair *foundVersion;

  heap = gst->heap;

 /* instead of keeping version as string we keep enum/int to fast-comparing */
  if (gltfVersionsLen == 0) {
    gltfVersionsLen = AK_ARRAY_LEN(gltfVersions);
    qsort(gltfVersions,
          gltfVersionsLen,
          sizeof(gltfVersions[0]),
          ak_enumpair_cmp);
  }

  contrib           = ak_heap_calloc(heap, dest, sizeof(*contrib));
  dest->contributor = contrib;

  while (json) {
    if (json_key_eq(json, _s_gltf_version)) {
      if (!(foundVersion = bsearch(json,
                                   gltfVersions,
                                   gltfVersionsLen,
                                   sizeof(gltfVersions[0]),
                                   ak_enumpair_json_val_cmp)))
        goto badf; /* unsupported version */

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
  dest->coordSys = AK_YUP;

  /* Unit is meter */
  dest->unit       = ak_heap_calloc(heap, dest, sizeof(*dest->unit));
  dest->unit->dist = 1.0;
  dest->unit->name = ak_heap_strdup(heap, dest->unit, _s_gltf_meter);

  *(AkAssetInf **)ak_heap_ext_add(heap,
                                  ak__alignof(memParent),
                                  AK_HEAP_NODE_FLAGS_INF) = dest;
  return AK_OK;

badf:
  ak_free(dest);
  return AK_EBADF;
}
