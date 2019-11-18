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
  char         *coordInputName;
  int32_t       texindex, set;
  size_t        len;

  heap     = gst->heap;
  doc      = gst->doc;
  texindex = json_int32(json_get(jtexinfo, _s_gltf_index), 0);
  set      = json_int32(json_get(jtexinfo, _s_gltf_texCoord), 0);
  tex      = flist_sp_at(&doc->lib.textures, texindex);
  
  texref = ak_heap_calloc(heap, parent, sizeof(*texref));
  ak_setypeid(texref, AKT_TEXTURE_REF);

  if (set == 0) {
    coordInputName = ak_heap_strdup(heap, texref, _s_gltf_texcoordPrefix);
  } else {
    len                 = strlen(_s_gltf_texcoordPrefix) + ak_digitsize(set);
    coordInputName      = ak_heap_alloc(heap, texref, len + 1);
    coordInputName[len] = '\0';
    sprintf(coordInputName, "%s%d", _s_gltf_texcoordPrefix, set);
  }

  texref->coordInputName = coordInputName;
  texref->texture        = tex;
  texref->slot           = set;

  return texref;
}

void _assetkit_hide
gltf_textures(json_t * __restrict jtex,
              void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkDoc              *doc;
  const json_array_t *jtextures;
  const json_t       *jtexVal;
  AkTexture          *tex;

  if (!(jtextures = json_array(jtex)))
    return;

  gst  = userdata;
  doc  = gst->doc;

  jtex = jtextures->base.value;
  while (jtex) {
    AkSampler *sampler;

    jtexVal   = jtex->value;
    tex       = ak_heap_calloc(gst->heap, gst->doc, sizeof(*tex));
    tex->type = AKT_SAMPLER2D;
    sampler   = NULL;

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
      
      ak_setypeid(sampler, AKT_SAMPLER2D);
    }

    tex->sampler = sampler;

    flist_sp_insert(&gst->doc->lib.textures, tex);
    jtex = jtex->next;
  }
}
