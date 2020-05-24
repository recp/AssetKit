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

void _assetkit_hide
dae_tex_walk(RBTree *tree, RBNode *rbnode);

void _assetkit_hide
dae_fix_textures(DAEState * __restrict dst) {
  rb_walk(dst->texmap, dae_tex_walk);
}

void _assetkit_hide
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

  texref   = ak_heap_calloc(heap, cd, sizeof(*texref));
  newparam = ak_sid_resolve(&actx, dtex->texture, NULL);
  
  if (!newparam
      || !(tex = newparam->val->value)) {
    ak_free(texref);
    return;
  }

  dst           = tree->userData;
  instanceImage = rb_find(dst->instanceMap, tex->sampler);
  image         = ak_instanceObject(instanceImage);
  
  texref->texture = tex;
  
  /* this is the default */
  /* use bind_material to set texcoord */
  texref->coordInputName = ak_heap_strdup(heap, texref, "TEXCOORD");
  tex->image             = image;
  cd->texture            = texref;
  
  if (dtex->texcoord)
    texref->texcoord = ak_heap_strdup(heap, texref, dtex->texcoord);
}
