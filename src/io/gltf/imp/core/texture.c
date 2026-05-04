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

#include "texture.h"
#include "profile.h"
#include "sampler.h"

AK_INLINE
char*
coordInputName(AkHeap * __restrict heap,
               void   * __restrict parent,
               int                 set) {
  char  *coordInputName;
  size_t len;

  if (set == 0) {
    coordInputName = ak_heap_strdup(heap, parent, _s_gltf_texcoordPrefix);
  } else {
    len                 = strlen(_s_gltf_texcoordPrefix) + ak_digitsize(set);
    coordInputName      = ak_heap_alloc(heap, parent, len + 1);
    coordInputName[len] = '\0';
    /* sprintf(coordInputName, "%s%d", _s_gltf_texcoordPrefix, set); */
    snprintf(coordInputName, len + 1, "%s%d", _s_gltf_texcoordPrefix, set);
  }

  return coordInputName;
}

AK_HIDE
AkTextureRef*
gltf_texref(AkGLTFState * __restrict gst,
            void        * __restrict parent,
            json_t      * __restrict jtexinfo) {
  AkHeap       *heap;
  AkDoc        *doc;
  AkTextureRef *texref;
  AkTexture    *tex;
  json_t       *jext;
  int32_t       texindex, set;

  heap     = gst->heap;
  doc      = gst->doc;
  texindex = json_int32(json_get(jtexinfo, _s_gltf_index), 0);
  set      = json_int32(json_get(jtexinfo, _s_gltf_texCoord), 0);
  tex      = flist_sp_at(&doc->lib.textures, texindex);
  
  texref = ak_heap_calloc(heap, parent, sizeof(*texref));
  ak_setypeid(texref, AKT_TEXTURE_REF);

  texref->coordInputName = coordInputName(heap, texref, set);

  if ((jext = json_get(jtexinfo, _s_gltf_extensions))) {
    json_t *jval;
    if ((jval = json_get(jext, _s_gltf_KHR_texture_transform))) {
      AkTextureTransform *texTransf;

      texTransf               = ak_heap_calloc(heap, texref, sizeof(*texTransf));
      texref->transform       = texTransf;
      texTransf->slot = -1;
      texTransf->scale[0]     = 1.0;
      texTransf->scale[1]     = 1.0;

      jval = jval->value;
      while (jval) {
        if (json_key_eq(jval, _s_gltf_offset)) {
          json_array_float(texTransf->offset, jval, 0.0f, 2, true);
        } else if (json_key_eq(jval, _s_gltf_rotation)) {
          texTransf->rotation = json_float(jval, 0.0f);
        } else if (json_key_eq(jval, _s_gltf_scale)) {
          json_array_float(texTransf->scale, jval, 0.0f, 2, true);
        } else if (json_key_eq(jval, _s_gltf_texCoord)) {
          texTransf->slot           = json_int32(jval, -1);
          texTransf->coordInputName = coordInputName(heap, texTransf, texTransf->slot);
        }
        jval = jval->next;
      }
    }
  }

  texref->texture = tex;
  texref->slot    = set;

  return texref;
}

AK_HIDE
void
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
