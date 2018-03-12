/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_texture.h"
#include "gltf_profile.h"
#include "gltf_sampler.h"

AkFxTexture* _assetkit_hide
gltf_texref(AkGLTFState * __restrict gst,
            AkEffect    * __restrict effect,
            void        * __restrict parent,
            json_t      * __restrict jtexinfo) {
  AkHeap      *heap;
  AkSampler   *sampler;
  AkNewParam  *param;
  AkFxTexture *tex;
  char        *sid, *sem;
  int32_t      texindex, texCoord;
  size_t       len;

  heap     = gst->heap;
  texindex = jsn_i32(jtexinfo, _s_gltf_index);
  texCoord = jsn_i32(jtexinfo, _s_gltf_texCoord);
  sampler  = gltf_texture(gst, effect, texindex);
  param    = ak_mem_parent(ak_mem_parent(sampler));

  ak_setypeid(param, AKT_NEWPARAM);

  /* set sid for bind_vertex_input */
  sid = (char *)ak_id_gen(heap, param, _s_gltf_sid_sampler);
  ak_sid_set(param, sid);

  tex = ak_heap_calloc(heap, parent, sizeof(*tex));
  ak_setypeid(tex, AKT_TEXTURE);

  len      = strlen(_s_gltf_sid_texcoord) + ak_digitsize(texindex) + 1;
  sem      = ak_heap_alloc(heap, param, len);
  sem[len] = '\0';
  sprintf(sem, "%s%d", _s_gltf_sid_texcoord, texCoord);

  tex->texture  = sid;
  tex->texcoord = sem;

  if (tex->texture)
    ak_setypeid((void *)tex->texture, AKT_TEXTURE_NAME);

  if (tex->texcoord)
    ak_setypeid((void *)tex->texcoord, AKT_TEXCOORD);

  return tex;
}

AkSampler* _assetkit_hide
gltf_texture(AkGLTFState * __restrict gst,
             AkEffect    * __restrict effect,
             int32_t                  index) {
  AkHeap        *heap;
  AkDoc         *doc;
  json_t        *jtextures;
  json_t        *jtexture, *jsampler, *jsource;
  AkSampler     *sampler;
  int32_t        jtextureCount;

  heap          = gst->heap;
  doc           = gst->doc;

  jtextures     = json_object_get(gst->root, _s_gltf_textures);
  jtextureCount = (int32_t)json_array_size(jtextures);

  if (!(index < jtextureCount))
    return NULL;

  sampler  = NULL;
  jtexture = json_array_get(jtextures, index);
  if ((jsampler = json_object_get(jtexture, _s_gltf_sampler))) {
    int32_t samplerIndex;

    samplerIndex = (int32_t)json_integer_value(jsampler);
    sampler      = gltf_sampler(gst, effect, samplerIndex);
  }

  /* create default sampler */
  if (!sampler) {
    sampler = gltf_newsampler(gst, effect);

    sampler->wrapS     = AK_WRAP_MODE_WRAP;
    sampler->wrapT     = AK_WRAP_MODE_WRAP;
    sampler->minfilter = AK_MINFILTER_LINEAR;
    sampler->magfilter = AK_MAGFILTER_LINEAR;
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

  return sampler;
}
