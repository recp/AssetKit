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

#include "sampler.h"
#include "profile.h"
#include "enum.h"

void AK_HIDE
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
