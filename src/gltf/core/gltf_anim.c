/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_anim.h"

#include "../gltf_common.h"
#include "gltf_accessor.h"
#include "gltf_enums.h"

#define k_path 0
#define k_node 1

void _assetkit_hide
gltf_animations(json_t * __restrict janim,
                void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *janims;
  AkLibItem          *lib;
  AkAnimation        *last_anim, *anim;

  if (!(janims = json_array(janim)))
    return;

  gst       = userdata;
  heap      = gst->heap;
  doc       = gst->doc;
  janim     = janims->base.value;
  lib       = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_anim = NULL;
  
  while (janim) {
    json_t     *janimVal;
    AkSource   *last_source;
    AkInput    *last_input;
    const char *animid;
    
    anim        = ak_heap_calloc(heap, lib,  sizeof(*anim));
    last_source = NULL;
    last_input  = NULL;
    
    /* sets id "anim-[i]" */
    animid = ak_id_gen(heap, anim, _s_gltf_anim);
    ak_heap_setId(heap, ak__alignof(anim), (void *)animid);
    
    janimVal = janim->value;
    
    if (json_key_eq(janimVal, _s_gltf_name)) {
      anim->name = json_strdup(janimVal, heap, anim);
    } else if (json_key_eq(janimVal, _s_gltf_samplers)) {
      AkAnimSampler *sampler, *last_sampler;
      json_array_t  *jsamplers;
      json_t        *jsampler;
      
      if (!(jsamplers = json_array(janimVal)))
        goto anm_nxt;

      jsampler     = jsamplers->base.value;
      last_sampler = NULL;
      last_source  = NULL;
      last_input   = NULL;
      
      /* samplers */
      while (jsampler) {
        json_t *jsamplerVal;
        
        jsamplerVal = jsampler->value;
        sampler     = ak_heap_calloc(heap, anim, sizeof(*sampler));
        
        if (json_key_eq(jsamplerVal, _s_gltf_input)) {
          AkInput  *input;
          AkSource *source;
          
          source = ak_heap_calloc(heap, anim, sizeof(*source));
          
          ak_setypeid(source, AKT_SOURCE);
          
          input = ak_heap_calloc(heap, sampler, sizeof(*input));
          input->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_input);
          input->semantic    = AK_INPUT_SEMANTIC_INPUT;
          source->tcommon    = flist_sp_at(&doc->lib.accessors,
                                           json_int32(jsamplerVal, -1));
          input->source.ptr  = source;
          
          if (last_source)
            last_source->next = source;
          else
            anim->source = source;
          last_source = source;
          
          if (last_input)
            last_input->next = input;
          else
            sampler->input = input;
          last_input = input;
        } else if (json_key_eq(jsamplerVal, _s_gltf_interpolation)) {
          sampler->uniInterpolation = gltf_interp(json_string(jsamplerVal));
        } else if (json_key_eq(jsamplerVal, _s_gltf_output)) {
          AkInput  *input;
          AkSource *source;
          
          source = ak_heap_calloc(heap, anim, sizeof(*source));
          
          ak_setypeid(source, AKT_SOURCE);
          
          input              = ak_heap_calloc(heap, sampler, sizeof(*input));
          input->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_output);
          input->semantic    = AK_INPUT_SEMANTIC_OUTPUT;
          source->tcommon    =  flist_sp_at(&doc->lib.accessors,
                                            json_int32(jsamplerVal, -1));
          input->source.ptr  = source;
          
          if (last_source)
            last_source->next = source;
          else
            anim->source = source;
          last_source = source;
          
          if (last_input)
            last_input->next = input;
          else
            sampler->input = input;
          /* last_input = input; */
        }

        /* Default is LINEAR */
        if (sampler->uniInterpolation == AK_INTERPOLATION_UNKNOWN) {
          sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;
        }

        if (last_sampler)
          last_sampler->next = sampler;
        else
          anim->sampler = sampler;
        last_sampler = sampler;
        
        jsampler = jsampler->next;
      }
    } else if (json_key_eq(janimVal, _s_gltf_channels)) {
      AkChannel     *ch, *last_ch;
      json_array_t  *jchannels;
      json_t        *jchannel;
      
      if (!(jchannels = json_array(janimVal)))
        goto anm_nxt;
      
      jchannel = jchannels->base.value;
      last_ch  = NULL;
      
      while (jchannel) {
        json_t *jchVal;
        
        ch     = ak_heap_calloc(heap, anim, sizeof(*ch));
        jchVal = jchannel->value;
        
        if (json_key_eq(jchVal, _s_gltf_sampler)) {
          AkAnimSampler *sampler;
          int32_t        samplerIndex;
          
          if ((samplerIndex = json_int32(jchVal, -1)) > 0) {
            sampler = anim->sampler;
            while (samplerIndex > 0) {
              sampler = sampler->next;
              samplerIndex--;
            }
            
            ch->source.ptr = sampler;
          }
        } else if (json_key_eq(jchVal, _s_gltf_target)) {
          const char   *path;
          AkNode       *node;
          json_t       *it;
          uint32_t      pathLen;
          
          json_objmap_t targetMap[] = {
            JSON_OBJMAP_OBJ(_s_gltf_path,         I2P k_path),
            JSON_OBJMAP_OBJ(_s_gltf_node,         I2P k_node)
          };

          json_objmap(jchVal->value, targetMap, JSON_ARR_LEN(targetMap));
          
          path    = NULL;
          pathLen = 0;
          

          if ((it = targetMap[k_path].object)) {
            path    = json_string(it);
            pathLen = it->valSize;
          }
          
          if (path && (it = targetMap[k_node].object)) {
            char     nodeid[16];
            uint32_t nodeIndex;
            
            if ((nodeIndex = json_int32(it, -1)) > -1) {
              sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex + 1);
              
              if ((node = ak_getObjectById(doc, nodeid))) {
                AkObject *transItem;
            
                /* make sure that node has target element */
                if (strncasecmp(path, _s_gltf_rotation, pathLen) == 0) {
                  ch->targetType = AK_TARGET_QUAT;
                  transItem      = ak_getTransformTRS(node, AKT_QUATERNION);
                } else if (strncasecmp(path, _s_gltf_translation, pathLen) == 0) {
                  ch->targetType = AK_TARGET_POSITION;
                  transItem      = ak_getTransformTRS(node, AKT_TRANSLATE);
                } else if (strncasecmp(path, _s_gltf_scale, pathLen) == 0) {
                  ch->targetType = AK_TARGET_SCALE;
                  transItem      = ak_getTransformTRS(node, AKT_SCALE);
                } else {
                  transItem = NULL;
                }
                
                ch->resolvedTarget = transItem;
              }
            } /* if nodeIndex */
          } /* if k_node */
        } /* if _s_gltf_target */
        
        if (last_ch)
          last_ch->next = ch;
        else
          anim->channel = ch;
        last_ch = ch;

        jchannel = jchannel->next;
      }
    }

  anm_nxt:

    if (last_anim)
      last_anim->next = anim;
    else
      lib->chld = anim;
    
    last_anim = anim;
    lib->count++;
    
    janim = janim->next;
  }

  doc->lib.animations = lib;
}
