/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_sampler.h"
#include "gltf_profile.h"
#include "gltf_enums.h"

void _assetkit_hide
gltf_samplers(AkGLTFState * __restrict gst) {
  AkHeap        *heap;
  json_t        *jsamplers;
  AkNewParam    *last_param;
  AkProfileGLTF *profile;
  size_t         jsamplerCount, i;

  heap       = gst->heap;
  last_param = NULL;

  jsamplers     = json_object_get(gst->root, _s_gltf_samplers);
  jsamplerCount = json_array_size(jsamplers);
  profile       = gltf_profile(gst);

  for (i = jsamplerCount; i != 0; i--) {
    AkSampler2D *sampler;
    AkNewParam  *param;
    AkValue     *paramVal;
    json_t      *jsampler;

    jsampler = json_array_get(jsamplers, i - 1);
    param    = ak_heap_calloc(heap, profile,  sizeof(*param));
    paramVal = ak_heap_calloc(heap, param,    sizeof(*paramVal));
    sampler  = ak_heap_calloc(heap, paramVal, sizeof(*sampler));

    memcpy(&paramVal->type,
           ak_typeDesc(AKT_SAMPLER2D),
           sizeof(paramVal->type));

    paramVal->value = sampler;
    param->val      = paramVal;

    sampler->wrapS = gltf_wrapMode(json_int32(jsampler, _s_gltf_wrapS));
    sampler->wrapT = gltf_wrapMode(json_int32(jsampler, _s_gltf_wrapT));

    sampler->minfilter = gltf_minFilter(json_int32(jsampler,
                                                   _s_gltf_minFilter));

    sampler->magfilter = gltf_magFilter(json_int32(jsampler,
                                                   _s_gltf_magFilter));

    /* TODO: get sampler.name for debug module */

    if (last_param)
      last_param->next = param;
    else
      profile->newparam = param;

    last_param = param;

    flist_sp_insert(&gst->samplers, sampler);
  }
}
