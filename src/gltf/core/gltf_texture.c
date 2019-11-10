/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_texture.h"
#include "gltf_profile.h"
#include "gltf_sampler.h"

AkTextureRef* _assetkit_hide
gltf_texref(AkGLTFState * __restrict gst,
            void        * __restrict parent,
            json_t      * __restrict jtexinfo) {
  AkHeap       *heap;
  AkDoc        *doc;
  AkTextureRef *texref;
  AkTexture    *tex;
  char         *texcoord;
  int32_t       texindex, coordindex;
  size_t        len;

  heap       = gst->heap;
  doc        = gst->doc;
  texindex   = json_int32(json_get(jtexinfo, _s_gltf_index), 0);
  coordindex = json_int32(json_get(jtexinfo, _s_gltf_texCoord), 0);
  tex        = flist_sp_at(&doc->lib.textures, texindex);

  texref = ak_heap_calloc(heap, parent, sizeof(*texref));
  ak_setypeid(texref, AKT_TEXTURE_REF);

  len           = strlen(_s_gltf_sid_texcoord) + ak_digitsize(coordindex);
  texcoord      = ak_heap_alloc(heap, texref, len + 1);
  texcoord[len] = '\0';
  sprintf(texcoord, "%s%d", _s_gltf_sid_texcoord, coordindex);

  texref->texture  = tex;
  texref->texcoord = texcoord;

  return texref;
}

void _assetkit_hide
gltf_textures(json_t * __restrict jtex,
              void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jtextures;
  const json_t       *jtexVal;
  AkTexture          *tex;

  if (!(jtextures = json_array(jtex)))
    return;

  gst  = userdata;

  heap = gst->heap;
  doc  = gst->doc;

  jtex = jtextures->base.value;
  while (jtex) {
    AkSampler *sampler;

    jtexVal = jtex->value;
    tex     = ak_heap_calloc(gst->heap, gst->doc, sizeof(*tex));
    sampler = NULL;

    while (jtexVal) {
      if (json_key_eq(jtexVal, _s_gltf_sampler)) {
        sampler = flist_sp_at(&doc->lib.samplers, json_int32(jtexVal, -1));
      } else if (json_key_eq(jtexVal, _s_gltf_source)) {
        tex->image = flist_sp_at(&doc->lib.images, json_int32(jtexVal, -1));
      } else if (json_key_eq(jtexVal, _s_gltf_name)) {
        tex->name = json_strdup(jtexVal, gst->heap, tex);
      }

      jtexVal = jtexVal->next;
    }

    /* TODO: add option for this */
    /* create default sampler */
    if (!sampler) {
      sampler        = ak_heap_calloc(gst->heap, gst->doc, sizeof(*sampler));
      sampler->wrapS = AK_WRAP_MODE_WRAP;
      sampler->wrapT = AK_WRAP_MODE_WRAP;
    }

    tex->sampler = sampler;

    flist_sp_insert(&gst->doc->lib.textures, tex);
    jtex = jtex->next;
  }
}
