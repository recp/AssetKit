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

  ak_setypeid(param, AKT_NEWPARAM);

  memcpy(&paramVal->type,
         ak_typeDesc(AKT_SAMPLER2D),
         sizeof(paramVal->type));

  paramVal->value = sampler;
  param->val      = paramVal;

  if (profile->newparam) {
    profile->newparam->prev = param;
    param->next             = profile->newparam;
  }

  profile->newparam = param;

  return sampler;
}

void _assetkit_hide
gltf_samplers(json_t * __restrict jsampler,
              void   * __restrict userdata) {
  AkGLTFState        *gst;
  const json_array_t *jsamplers;
  const json_t       *jsamplerVal;
  AkSampler          *sampler;

  if (!(jsamplers = json_array(jsampler)))
    return;

  gst = userdata;

  jsampler = jsamplers->base.value;
  while (jsampler) {
    jsamplerVal    = jsampler->value;
    sampler        = ak_heap_calloc(gst->heap, gst->doc, sizeof(*sampler));
    sampler->wrapS = AK_WRAP_MODE_WRAP;
    sampler->wrapT = AK_WRAP_MODE_WRAP;

    while (jsamplerVal) {
      if (json_key_eq(jsamplerVal, _s_gltf_wrapS)) {
        sampler->wrapS = gltf_wrapMode(json_int32(jsamplerVal,
                                                  AK_WRAP_MODE_WRAP));
      } else if (json_key_eq(jsamplerVal, _s_gltf_wrapT)) {
        sampler->wrapS = gltf_wrapMode(json_int32(jsamplerVal,
                                                  AK_WRAP_MODE_WRAP));
      } else if (json_key_eq(jsamplerVal, _s_gltf_minFilter)) {
        sampler->minfilter = gltf_minFilter(json_int32(jsamplerVal, 0));
      } else if (json_key_eq(jsamplerVal, _s_gltf_magFilter)) {
        sampler->magfilter = gltf_magFilter(json_int32(jsamplerVal, 0));
      } else if (json_key_eq(jsamplerVal, _s_gltf_name)) {
        sampler->name = json_strdup(jsamplerVal, gst->heap, sampler);
      }

      jsamplerVal = jsamplerVal->next;
    }

    flist_sp_insert(&gst->doc->lib.samplers, sampler);
    jsampler = jsampler->next;
  }
}
