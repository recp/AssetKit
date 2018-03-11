/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_texture.h"
#include "gltf_profile.h"

void _assetkit_hide
gltf_textures(AkGLTFState * __restrict gst) {
  AkHeap        *heap;
  AkDoc         *doc;
  json_t        *jtextures;
  size_t         jtextureCount, i;

  heap          = gst->heap;
  doc           = gst->doc;

  jtextures     = json_object_get(gst->root, _s_gltf_textures);
  jtextureCount = json_array_size(jtextures);

  for (i = 0; i < jtextureCount; i++) {
    json_t      *jtexture, *jsampler, *jsource;
    AkSampler   *sampler;

    sampler  = NULL;
    jtexture = json_array_get(jtextures, i);
    if ((jsampler = json_object_get(jtexture, _s_gltf_sampler))) {
      int32_t samplerIndex;

      samplerIndex = (int32_t)json_integer_value(jsampler);
      sampler      = flist_sp_at(&gst->samplers, samplerIndex);
    }

    /* create default sampler */
    if (!sampler) {
      AkProfileGLTF *profile;
      AkNewParam    *param;
      AkValue       *paramVal;

      profile  = gltf_profile(gst);
      param    = ak_heap_calloc(heap, profile,  sizeof(*param));
      paramVal = ak_heap_calloc(heap, param,    sizeof(*paramVal));
      sampler  = ak_heap_calloc(heap, paramVal, sizeof(*sampler));

      memcpy(&paramVal->type,
             ak_typeDesc(AKT_SAMPLER2D),
             sizeof(paramVal->type));

      paramVal->value = sampler;
      param->val      = paramVal;

      sampler->wrapS     = AK_WRAP_MODE_WRAP;
      sampler->wrapT     = AK_WRAP_MODE_WRAP;
      sampler->minfilter = AK_MINFILTER_LINEAR;
      sampler->magfilter = AK_MAGFILTER_LINEAR;

      if (profile->newparam) {
        profile->newparam->prev = param;
        param->next = profile->newparam;
      }

      profile->newparam = param;
    }

    if ((jsource = json_object_get(jtexture, _s_gltf_source))) {
      AkImage *image;
      int32_t  imageIndex;

      imageIndex = (int32_t)json_integer_value(jsource);
      image      = doc->lib.images->chld;

      while (imageIndex != 0 && image) {
        image = image->next;
        imageIndex--;
      }

      if (image) {
        AkInstanceBase *instImage;
        instImage = ak_heap_calloc(heap, sampler, sizeof(*instImage));
        instImage->url.ptr = image;

        sampler->instanceImage = instImage;
      }
    }
  }
}
