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

void _assetkit_hide
gltf_animations(AkGLTFState * __restrict gst) {
  AkHeap      *heap;
  AkDoc       *doc;
  json_t      *janims, *jaccessors;
  AkLibItem   *lib;
  AkAnimation *last_anim, *anim;
  size_t       janimCount, i;

  heap        = gst->heap;
  doc         = gst->doc;
  lib         = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_anim   = NULL;

  janims      = json_object_get(gst->root, _s_gltf_animations);
  janimCount  = json_array_size(janims);
  jaccessors  = json_object_get(gst->root, _s_gltf_accessors);

  for (i = 0; i < janimCount; i++) {
    json_t     *janim, *jchannels, *jsamps;
    AkSource   *last_source;
    AkInput    *last_input;
    const char *animid;
    const char *sval;

    janim       = json_array_get(janims, i);
    anim        = ak_heap_calloc(heap, lib,  sizeof(*anim));
    last_source = NULL;
    last_input  = NULL;

    /* sets id "anim-[i]" */
    animid = ak_id_gen(heap, anim, _s_gltf_anim);
    ak_heap_setId(heap, ak__alignof(anim), (void *)animid);

    if ((sval = json_cstr(janim, _s_gltf_name)))
      anim->name = ak_heap_strdup(gst->heap, anim, sval);

    if ((jsamps = json_object_get(janim, _s_gltf_samplers))) {
      AkAnimSampler *sampler, *last_sampler;
      int32_t j, sampCount;

      last_sampler = NULL;
      last_source  = NULL;
      sampCount    = (int32_t)json_array_size(jsamps);

      /* samplers */
      for (j = 0; j < sampCount; j++) {
        json_t *jsamp, *jinput, *joutput, *jinterp, *jacc;

        jsamp      = json_array_get(jsamps, j);
        sampler    = ak_heap_calloc(heap, anim, sizeof(*sampler));
        last_input = NULL;

        if ((jinterp = json_object_get(jsamp, _s_gltf_interpolation))) {
          sampler->uniInterpolation = gltf_interp(json_string_value(jinterp));
        }

        /* Default is LINEAR */
        else {
          sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;
        }

        if ((jinput = json_object_get(jsamp, _s_gltf_input))) {
          AkInput  *input;
          AkSource *source;

          source = ak_heap_calloc(heap, anim, sizeof(*source));
          jacc   = json_array_get(jaccessors, json_integer_value(jinput));

          ak_setypeid(source, AKT_SOURCE);

          input = ak_heap_calloc(heap, sampler, sizeof(*input));
          input->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_input);
          input->semantic    = AK_INPUT_SEMANTIC_INPUT;
          source->tcommon    = gltf_accessor(gst, source, jacc);
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
        }

        if ((joutput = json_object_get(jsamp, _s_gltf_output))) {
          AkInput  *input;
          AkSource *source;

          source = ak_heap_calloc(heap, anim, sizeof(*source));
          jacc   = json_array_get(jaccessors, json_integer_value(joutput));

          ak_setypeid(source, AKT_SOURCE);

          input              = ak_heap_calloc(heap, sampler, sizeof(*input));
          input->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_output);
          input->semantic    = AK_INPUT_SEMANTIC_OUTPUT;
          source->tcommon    = gltf_accessor(gst, source, jacc);
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

        if (last_sampler)
          last_sampler->next = sampler;
        else
          anim->sampler = sampler;
        last_sampler = sampler;
      }
    }

    /* channels */
    if ((jchannels = json_object_get(janim, _s_gltf_channels))) {
      AkChannel *ch, *last_ch;
      int32_t j, chCount;

      last_ch = NULL;
      chCount = (int32_t)json_array_size(jchannels);

      for (j = chCount - 1; j >= 0; j--) {
        json_t *jch, *jsamp, *jpath, *jtarget, *jnode;

        jch = json_array_get(jchannels, j);
        ch  = ak_heap_calloc(heap, anim, sizeof(*ch));

        if ((jsamp = json_object_get(jch, _s_gltf_sampler))) {
          AkAnimSampler *sampler;
          int32_t        samplerIndex;

          samplerIndex = (int32_t)json_integer_value(jsamp);
          sampler      = anim->sampler;
          while (samplerIndex > 0) {
            sampler = sampler->next;
            samplerIndex--;
          }

          if (sampler)
            ch->source.ptr = sampler;
        }

        if ((jtarget = json_object_get(jch, _s_gltf_target))) {
          const char   *sval;
          AkNode       *node;

          sval = NULL;
          if ((jpath = json_object_get(jtarget, _s_gltf_path)))
            sval = json_string_value(jpath);

          if (sval && (jnode = json_object_get(jtarget, _s_gltf_node))) {
            char     nodeid[16];
            uint32_t nodeIndex;

            nodeIndex = (uint32_t)json_integer_value(jnode);
            sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex + 1);

            if ((node = ak_getObjectById(doc, nodeid))) {
              AkObject *transItem;

              /* make sure that node has target element */
              if (strcasecmp(sval, _s_gltf_rotation) == 0) {
                ch->targetType = AK_TARGET_QUAT;
                transItem = ak_getTransformTRS(node, AKT_QUATERNION);
              } else if (strcasecmp(sval, _s_gltf_translation) == 0) {
                ch->targetType = AK_TARGET_POSITION;
                transItem = ak_getTransformTRS(node, AKT_TRANSLATE);
              } else if (strcasecmp(sval, _s_gltf_scale) == 0) {
                ch->targetType = AK_TARGET_SCALE;
                transItem = ak_getTransformTRS(node, AKT_SCALE);
              } else {
                transItem = NULL;
              }

              ch->resolvedTarget = transItem;
            }
          }
        }

        if (last_ch)
          last_ch->next = ch;
        else
          anim->channel = ch;
        last_ch = ch;
      }
    }

    if (last_anim)
      last_anim->next = anim;
    else
      lib->chld = anim;

    last_anim = anim;
    lib->count++;
  }

  doc->lib.animations = lib;
}
