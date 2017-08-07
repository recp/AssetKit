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
gltf_asset(AkGLTFState * __restrict gst,
           void        * __restrict memParent,
           AkAssetInf  * __restrict dest) {
  AkAssetInf       **extp;
  AkContributor     *contrib;
  json_t            *jasset, *jval;
  const char        *sval;
  const ak_enumpair *foundVersion;

  if (!dest) {
    dest = ak_heap_calloc(gst->heap,
                          memParent,
                          sizeof(*dest));
  }

  jasset = json_object_get(gst->root, _s_gltf_asset);
  jval   = json_object_get(jasset, _s_gltf_version);

  /* glTF version is required to parse / load */
  if (!jval || !(sval = json_string_value(jval)))
    goto badf;

  /* instead of keeping version as string we keep enum/int to fast-comparing */
  if (gltfVersionsLen == 0) {
   gltfVersionsLen = AK_ARRAY_LEN(gltfVersions);
    qsort(gltfVersions,
          gltfVersionsLen,
          sizeof(gltfVersions[0]),
          ak_enumpair_cmp);
  }

  /* get version info */
  foundVersion = bsearch(sval,
                         gltfVersions,
                         gltfVersionsLen,
                         sizeof(gltfVersions[0]),
                         ak_enumpair_cmp2);
  if (!foundVersion)
    goto badf;

  gst->version = foundVersion->val;

  contrib = NULL;
  jval = json_object_get(jasset, _s_gltf_copyright);
  if (jval && (sval = json_string_value(jval))) {
    if (!contrib) {
      contrib = ak_heap_calloc(gst->heap, dest, sizeof(*contrib));
      dest->contributor = contrib;
    }

    contrib->copyright = ak_heap_strdup(gst->heap, contrib, sval);
  }

  jval = json_object_get(jasset, _s_gltf_generator);
  if (jval && (sval = json_string_value(jval))) {
    if (!contrib) {
      contrib = ak_heap_calloc(gst->heap, dest, sizeof(*contrib));
      dest->contributor = contrib;
    }

    contrib->authoringTool = ak_heap_strdup(gst->heap, contrib, sval);
  }

  /* glTF default definitions */

  /* CoordSys is Y_UP */
  dest->coordSys = AK_YUP;

  /* Unit is meter */
  dest->unit = ak_heap_calloc(gst->heap, dest, sizeof(*dest->unit));
  dest->unit->dist = 1.0;
  dest->unit->name = ak_heap_strdup(gst->heap,
                                    dest->unit,
                                    _s_gltf_meter);

  extp = ak_heap_ext_add(gst->heap,
                         ak__alignof(memParent),
                         AK_HEAP_NODE_FLAGS_INF);
  *extp = dest;

  return AK_OK;

badf:
  ak_free(dest);
  return AK_EBADF;
}
