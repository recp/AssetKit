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

#include "anim.h"
#include "accessor.h"
#include "enum.h"

#define k_path 0
#define k_node 1

#define k_anim_samplers 0
#define k_anim_channels 1
#define k_anim_name     2

void _assetkit_hide
gltf_animations(json_t * __restrict janim,
                void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *janims;
  AkLibrary          *lib;
  AkAnimation        *anim;

  if (!(janims = json_array(janim)))
    return;

  gst   = userdata;
  heap  = gst->heap;
  doc   = gst->doc;
  janim = janims->base.value;
  lib   = ak_heap_calloc(heap, doc, sizeof(*lib));
  
  while (janim) {
    json_t *anim_it;
    
    json_objmap_t animMap[] = {
      JSON_OBJMAP_OBJ(_s_gltf_samplers, I2P k_anim_samplers),
      JSON_OBJMAP_OBJ(_s_gltf_channels, I2P k_anim_channels),
      JSON_OBJMAP_OBJ(_s_gltf_name,     I2P k_anim_name),
    };
    
    json_objmap(janim, animMap, JSON_ARR_LEN(animMap));
    
    anim = ak_heap_calloc(heap, lib,  sizeof(*anim));

    if ((anim_it = animMap[k_anim_name].object)) {
      anim->name = json_strdup(anim_it, heap, anim);
    }
    
    if ((anim_it = animMap[k_anim_samplers].object)) {
      AkAnimSampler *sampler;
      json_array_t  *jsamplers;
      json_t        *jsampler;
      
      if (!(jsamplers = json_array(anim_it)))
        goto anm_nxt;
      
      jsampler = jsamplers->base.value;
      
      /* samplers */
      while (jsampler) {
        json_t *jsampVal;
        
        jsampVal = jsampler->value;
        sampler     = ak_heap_calloc(heap, anim, sizeof(*sampler));
        
        while (jsampVal) {
          if (json_key_eq(jsampVal, _s_gltf_input)) {
            AkInput *inp;
            
            inp              = ak_heap_calloc(heap, sampler, sizeof(*inp));
            inp->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_input);
            inp->semantic    = AK_INPUT_SEMANTIC_INPUT;
            inp->accessor    = flist_sp_at(&doc->lib.accessors,
                                           json_int32(jsampVal, -1));
            
            inp->next      = sampler->input;
            sampler->input = inp;
          } else if (json_key_eq(jsampVal, _s_gltf_interpolation)) {
            sampler->uniInterpolation = gltf_interp(jsampVal);
          } else if (json_key_eq(jsampVal, _s_gltf_output)) {
            AkInput *inp;
            
            inp              = ak_heap_calloc(heap, sampler, sizeof(*inp));
            inp->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_output);
            inp->semantic    = AK_INPUT_SEMANTIC_OUTPUT;
            inp->accessor    = flist_sp_at(&doc->lib.accessors,
                                           json_int32(jsampVal, -1));
            
            inp->next      = sampler->input;
            sampler->input = inp;
          }
          
          /* Default is LINEAR */
          if (sampler->uniInterpolation == AK_INTERPOLATION_UNKNOWN) {
            sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;
          }

          jsampVal = jsampVal->next;
        }

        sampler->base.next = (void *)anim->sampler;
        anim->sampler      = sampler;

        jsampler = jsampler->next;
      }
    }
    
    if ((anim_it = animMap[k_anim_channels].object)) {
      AkChannel    *ch;
      json_array_t *jchannels;
      json_t       *jchannel;
      
      if (!(jchannels = json_array(anim_it)))
        goto anm_nxt;
      
      jchannel = jchannels->base.value;
      
      while (jchannel) {
        json_t *jchVal;
        
        ch     = ak_heap_calloc(heap, anim, sizeof(*ch));
        jchVal = jchannel->value;
        
        while (jchVal) {
          if (json_key_eq(jchVal, _s_gltf_sampler)) {
            AkAnimSampler *sampler;
            int32_t        samplerIndex;
            
            samplerIndex = json_int32(jchVal, -1);
            GETCHILD(anim->sampler, sampler, samplerIndex);
            ch->source.ptr = sampler;
          } else if (json_key_eq(jchVal, _s_gltf_target)) {
            const char *path;
            AkNode     *node;
            json_t     *it;
            uint32_t    pathLen;
            
            json_objmap_t targetMap[] = {
              JSON_OBJMAP_OBJ(_s_gltf_path, I2P k_path),
              JSON_OBJMAP_OBJ(_s_gltf_node, I2P k_node)
            };

            json_objmap(jchVal, targetMap, JSON_ARR_LEN(targetMap));

            path    = NULL;
            pathLen = 0;

            if ((it = targetMap[k_path].object)) {
              path    = json_string(it);
              pathLen = it->valsize;
            }

            if (path && (it = targetMap[k_node].object)) {
              char    nodeid[16];
              int32_t nodeIndex;
              
              if ((nodeIndex = json_int32(it, -1)) > -1) {
                sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex);
                
                if ((node = ak_getObjectById(doc, nodeid))) {
                  
                  /* make sure that node has target element */
                  if (strncasecmp(path, _s_gltf_rotation, pathLen) == 0) {
                    ch->targetType     = AK_TARGET_QUAT;
                    ch->resolvedTarget = ak_getTransformTRS(node, AKT_QUATERNION);
                  } else if (strncasecmp(path, _s_gltf_translation, pathLen) == 0) {
                    ch->targetType     = AK_TARGET_POSITION;
                    ch->resolvedTarget = ak_getTransformTRS(node, AKT_TRANSLATE);
                  } else if (strncasecmp(path, _s_gltf_scale, pathLen) == 0) {
                    ch->targetType     = AK_TARGET_SCALE;
                    ch->resolvedTarget = ak_getTransformTRS(node, AKT_SCALE);
                  } else if (strncasecmp(path, _s_gltf_weights, pathLen) == 0) {
                    AkInstanceMorph    *morpher;
                    AkInstanceGeometry *instGeom;

                    ch->targetType = AK_TARGET_WEIGHTS;
                    if ((instGeom = node->geometry)
                        && (morpher = instGeom->morpher))
                      ch->resolvedTarget = morpher->overrideWeights;
                  }
                }
              } /* if nodeIndex */
            } /* if k_node */
          } /* if _s_gltf_target */
          
          jchVal = jchVal->next;
        }
        ch->next      = anim->channel;
        anim->channel = ch;
        
        jchannel = jchannel->next;
      }
    }
    
  anm_nxt:

    anim->base.next = (void *)lib->chld;
    lib->chld       = (void *)anim;
    lib->count++;

    janim = janim->next;
  }

  doc->lib.animations = lib;
}
