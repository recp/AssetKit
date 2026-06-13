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

#include "tex.h"
#include "../strpool.h"

#include <string.h>

AK_HIDE
void
dae_tex_walk(RBTree *tree, RBNode *rbnode);

AK_HIDE
void
dae_fix_textures(DAEState * __restrict dst) {
  rb_walk(dst->texmap, dae_tex_walk);
}

static
AkNewParam*
dae_find_newparam_by_sid(AkNewParam *param, const char *sid) {
  while (param) {
    const char *paramSid;

    paramSid = ak_sid_get(param);
    if (paramSid && strcmp(paramSid, sid) == 0)
      return param;

    param = param->next;
  }

  return NULL;
}

static
AkNewParam*
dae_tex_find_newparam(AkColorDesc *cd, const char *sid) {
  void *parent;

  if (!cd || !sid)
    return NULL;

  for (parent = cd; parent; parent = ak_mem_parent(parent)) {
    switch (ak_typeid(parent)) {
      case AKT_PROFILE: {
        AkProfile *profile;
        AkNewParam *param;

        profile = parent;
        param   = dae_find_newparam_by_sid(profile->newparam, sid);
        if (param)
          return param;
        break;
      }
      case AKT_EFFECT: {
        AkEffect *effect;
        AkNewParam *param;

        effect = parent;
        param  = dae_find_newparam_by_sid(effect->newparam, sid);
        if (param)
          return param;
        break;
      }
      default:
        break;
    }
  }

  return NULL;
}

static
bool
dae_tex_needs_sid_resolve(const char *target) {
  return target
         && (target[0] == '#'
             || target[0] == '.'
             || strchr(target, '/')
             || strchr(target, '.'));
}

static
AkImage*
dae_tex_find_image_by_id(AkDoc *doc, const char *id) {
  AkImage *image;
  void    *obj;

  if (!doc || !id)
    return NULL;

  obj = ak_getObjectById(doc, id);
  if (!obj)
    return NULL;

  image = doc->lib.images.first;
  while (image) {
    if ((void *)image == obj)
      return image;

    image = image->next;
  }

  return NULL;
}

AK_HIDE
void
dae_tex_walk(RBTree *tree, RBNode *rbnode) {
  AkHeap          *heap;
  AkNewParam      *newparam;
  AkColorDesc     *cd;
  AkDAETextureRef *dtex;
  AkTextureRef    *texref;
  AkTexture       *tex;
  AkImage         *image;
  DAEState        *dst;
  AkInstanceBase  *instanceImage;
  AkContext        actx = {0};

  cd       = rbnode->key;
  dtex     = rbnode->val;
  heap     = ak_heap_getheap(cd);
  
  actx.doc = ak_heap_data(heap);

  if (!dtex || !dtex->texture)
    return;

  texref   = ak_heap_calloc(heap, cd, sizeof(*texref));
  newparam = dae_tex_find_newparam(cd, dtex->texture);

  if (!newparam) {
    if ((image = dae_tex_find_image_by_id(actx.doc, dtex->texture))) {
      tex = ak_heap_calloc(heap, cd, sizeof(*tex));
      ak_setypeid(tex, AKT_TEXTURE);

      tex->image = image;
      tex->type  = AKT_SAMPLER2D;
      goto bind_texture;
    }
  }

  if (!newparam && dae_tex_needs_sid_resolve(dtex->texture))
    newparam = ak_sid_resolve(&actx, dtex->texture, NULL);
  
  if (!newparam
      || !newparam->val
      || !(tex = newparam->val->value)) {
    ak_free(texref);
    return;
  }

  dst           = tree->userData;
  instanceImage = tex->sampler ? rb_find(dst->instanceMap, tex->sampler) : NULL;
  image         = ak_instanceObject(instanceImage);

bind_texture:
  texref->texture = tex;
  ak_texref_usage(texref, dtex->colorSpace, dtex->channels);
  
  /* this is the default */
  /* use bind_material to set texcoord */
  texref->coordInputName = ak_heap_strdup(heap, texref, _s_dae_TEXCOORD);
  tex->image             = image;
  cd->texture            = texref;
  
  if (dtex->texcoord)
    texref->texcoord = ak_heap_strdup(heap, texref, dtex->texcoord);
}
