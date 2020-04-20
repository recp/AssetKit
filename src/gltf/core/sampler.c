/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "sampler.h"
#include "profile.h"
#include "enum.h"

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

    ak_setypeid(sampler, AKT_SAMPLER2D);
    
    while (jsamplerVal) {
      if (json_key_eq(jsamplerVal, _s_gltf_wrapS)) {
        sampler->wrapS = gltf_wrapMode(json_int32(jsamplerVal,
                                                  AK_WRAP_MODE_WRAP));
      } else if (json_key_eq(jsamplerVal, _s_gltf_wrapT)) {
        sampler->wrapT = gltf_wrapMode(json_int32(jsamplerVal,
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
