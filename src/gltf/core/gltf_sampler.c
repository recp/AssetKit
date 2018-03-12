/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_sampler.h"
#include "gltf_profile.h"
#include "gltf_enums.h"

AkSampler* _assetkit_hide
gltf_newsampler(AkGLTFState * __restrict gst,
                AkEffect    * __restrict effect) {
  AkHeap        *heap;
  AkProfileGLTF *profile;

  AkSampler     *sampler;
  AkNewParam    *param;
  AkValue       *paramVal;

  heap     = gst->heap;
  profile  = effect->profile; /* there is only one, I hope */
  param    = ak_heap_calloc(heap, profile,  sizeof(*param));
  paramVal = ak_heap_calloc(heap, param,    sizeof(*paramVal));
  sampler  = ak_heap_calloc(heap, paramVal, sizeof(*sampler));

  memcpy(&paramVal->type,
         ak_typeDesc(AKT_SAMPLER2D),
         sizeof(paramVal->type));

  paramVal->value   = sampler;
  param->val        = paramVal;

  if (profile->newparam) {
    profile->newparam->prev = param;
    param->next             = profile->newparam;
  }

  profile->newparam = param;

  return sampler;
}

AkSampler* _assetkit_hide
gltf_sampler(AkGLTFState * __restrict gst,
             AkEffect    * __restrict effect,
             int32_t                  index) {
  json_t    *jsamplers;
  AkSampler *sampler;
  json_t    *jsampler;
  int32_t    jsamplerCount;

  jsamplers     = json_object_get(gst->root, _s_gltf_samplers);
  jsamplerCount = (int32_t)json_array_size(jsamplers);

  if (!(index < jsamplerCount))
    return NULL;

  jsampler           = json_array_get(jsamplers, index);
  sampler            = gltf_newsampler(gst, effect);

  sampler->wrapS     = gltf_wrapMode(jsn_i32(jsampler, _s_gltf_wrapS));
  sampler->wrapT     = gltf_wrapMode(jsn_i32(jsampler, _s_gltf_wrapT));

  sampler->minfilter = gltf_minFilter(jsn_i32(jsampler, _s_gltf_minFilter));
  sampler->magfilter = gltf_magFilter(jsn_i32(jsampler, _s_gltf_magFilter));

  return sampler;
}
