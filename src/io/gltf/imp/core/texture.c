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
#include "ext.h"
#include "profile.h"
#include "sampler.h"
#include "../extra.h"

AK_INLINE
char*
coordInputName(AkHeap * __restrict heap,
               void   * __restrict parent,
               int                 set) {
  char  *coordInputName;
  size_t len;

  if (set == 0)
    return (char *)_s_gltf_texcoordPrefix;

  len                 = _s_gltf_texcoordPrefix_len + ak_digitsize(set);
  coordInputName      = ak_heap_alloc(heap, parent, len + 1);
  coordInputName[len] = '\0';
  /* sprintf(coordInputName, "%s%d", _s_gltf_texcoordPrefix, set); */
  snprintf(coordInputName, len + 1, "%s%d", _s_gltf_texcoordPrefix, set);

  return coordInputName;
}

AK_INLINE
AkSampler*
gltf_defaultSampler(AkGLTFState * __restrict gst) {
  AkSampler *sampler;

  if ((sampler = gst->defaultSampler))
    return sampler;

  sampler        = ak_heap_calloc(gst->heap, gst->doc, sizeof(*sampler));
  sampler->wrapS = AK_WRAP_MODE_WRAP;
  sampler->wrapT = AK_WRAP_MODE_WRAP;
  ak_setypeid(sampler, AKT_SAMPLER2D);

  return gst->defaultSampler = sampler;
}

AK_HIDE
AkTextureRef*
gltf_texref(AkGLTFState * __restrict gst,
            void        * __restrict parent,
            json_t      * __restrict jtexinfo) {
  AkHeap       *heap;
  AkTextureRef *texref;
  AkTexture    *tex;
  json_t       *jext;
  int32_t       texindex, set;

  heap     = gst->heap;
  texindex = json_int32(GLTF_JSON_GET8(jtexinfo, index), 0);
  set      = json_int32(GLTF_JSON_GET8(jtexinfo, texCoord), 0);
  tex      = gltf_texture_at(gst, texindex);
  
  texref = ak_heap_calloc(heap, parent, sizeof(*texref));
  ak_setypeid(texref, AKT_TEXTURE_REF);
  gltf_extra(gst,
             texref,
             GLTF_JSON_GET8(jtexinfo, extras),
             gltf_jsonGetLen(jtexinfo, _s_gltf_extensions, 10));

  texref->coordInputName = coordInputName(heap, texref, set);

  if ((jext = gltf_jsonGetLen(jtexinfo, _s_gltf_extensions, 10))) {
    json_t *jval;
    if ((jval = gltf_jsonGetLen(jext, _s_gltf_KHR_texture_transform, 21))) {
      AkTextureTransform *texTransf;

      texTransf               = ak_heap_calloc(heap, texref, sizeof(*texTransf));
      texref->transform       = texTransf;
      texTransf->slot = -1;
      texTransf->scale[0]     = 1.0;
      texTransf->scale[1]     = 1.0;

      jval = jval->value;
      while (jval) {
        if (GLTF_JSON_KEY_EQ8(jval, offset)) {
          json_array_float(texTransf->offset, jval, 0.0f, 2, true);
        } else if (GLTF_JSON_KEY_EQ8(jval, rotation)) {
          texTransf->rotation = json_float(jval, 0.0f);
        } else if (GLTF_JSON_KEY_EQ8(jval, scale)) {
          json_array_float(texTransf->scale, jval, 0.0f, 2, true);
        } else if (GLTF_JSON_KEY_EQ8(jval, texCoord)) {
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
  const json_array_t *jtextures;
  const json_t       *jtexVal;
  AkTexture          *tex;
  size_t              textureIndex;

  if (!(jtextures = json_array(jtex)))
    return;

  gst  = userdata;
  gst->texturesCount   = jtextures->count;
  gst->texturesByIndex = ak_heap_calloc(gst->heap,
                                        gst->tmpParent,
                                        sizeof(*gst->texturesByIndex)
                                        * gst->texturesCount);
  textureIndex = gst->texturesCount;

  jtex = jtextures->base.value;
  while (jtex) {
    AkSampler *sampler;

    jtexVal   = jtex->value;
    tex       = ak_heap_calloc(gst->heap, gst->doc, sizeof(*tex));
    tex->type = AKT_SAMPLER2D;
    sampler   = NULL;
    gltf_extra(gst,
               tex,
               GLTF_JSON_GET8(jtex, extras),
               gltf_jsonGetLen(jtex, _s_gltf_extensions, 10));

    while (jtexVal) {
      if (GLTF_JSON_KEY_EQ8(jtexVal, sampler)) {
        sampler = gltf_sampler_at(gst, json_int32(jtexVal, -1));
      } else if (GLTF_JSON_KEY_EQ8(jtexVal, source)) {
        tex->image = gltf_image_at(gst, json_int32(jtexVal, -1));
      } else if (GLTF_JSON_KEY_EQ8(jtexVal, name)) {
        tex->name = json_strdup(jtexVal, gst->heap, tex);
      } else if (gltf_jsonKeyEqLen(jtexVal, _s_gltf_extensions, 10)) {
        /* Texture-source extensions. WebP can go through the normal image
           loader. KTX2/BasisU is selected only when the optional decoder
           shim is available, so a PNG/JPEG fallback remains intact. */
        json_t *jwebp;
        json_t *jktx2;
        json_t *jaltSrc;

        jwebp = gltf_jsonGetLen(jtexVal, _s_gltf_EXT_texture_webp, 16);
        jktx2 = gltf_jsonGetLen(jtexVal, _s_gltf_KHR_texture_basisu, 18);

        if (jktx2
            && gltf_ext_textureBasisu(gst)
            && (jaltSrc = GLTF_JSON_GET8(jktx2, source))) {
          AkImage *altImage = gltf_image_at(gst, json_int32(jaltSrc, -1));
          if (altImage)
            tex->image = altImage;
        }
        if (jwebp && (jaltSrc = GLTF_JSON_GET8(jwebp, source))) {
          AkImage *altImage = gltf_image_at(gst, json_int32(jaltSrc, -1));
          if (altImage)
            tex->image = altImage;
        }
      }

      jtexVal = jtexVal->next;
    }

    /* TODO: add option for this */
    /* create default sampler */
    if (!sampler)
      sampler = gltf_defaultSampler(gst);

    tex->sampler = sampler;

    flist_sp_insert(&gst->doc->lib.textures, tex);
    if (textureIndex > 0)
      gst->texturesByIndex[--textureIndex] = tex;
    jtex = jtex->next;
  }
}
