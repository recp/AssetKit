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

#include "postscript.h"
#include "../../xml.h"

#include "1.4/dae14.h"

#include "fixup/geom.h"
#include "fixup/angle.h"
#include "fixup/tex.h"
#include "fixup/ctlr.h"

AK_HIDE void
dae_retain_refs(DAEState * __restrict dst);

AK_HIDE void
dae_fixup_accessors(DAEState * __restrict dst);

AK_HIDE void
dae_pre_mesh(DAEState * __restrict dst);

AK_HIDE void
dae_pre_walk(RBTree *tree, RBNode *rbnode);

AK_HIDE void
dae_input_walk(RBTree *tree, RBNode *rbnode);

AK_HIDE
void
dae_spread_vert(DAEState * __restrict dst) {
  AkHeap               *heap;
  AkVertices           *vert;
  AkMeshPrimitive      *prim;
  AkDAEVerticesMapItem *item;
  FListItem            *fitem;
  AkInput              *inp;
  AkInput              *inpv;
  AkURL                *url;

  if (!(heap = dst->heap) || !(fitem = dst->vertMap)) {
    return;
  }

  /* copy <vertices> to all primitives */
  do {
    item = fitem->data;
    prim = item->prim;
    inp  = item->inp;
    url  = rb_find(dst->inputmap, inp);

    if (!(vert = ak_getObjectByUrl(url)))
      continue;

    inpv = vert->input;

    while (inpv) {
      inp  = ak_heap_calloc(heap, prim, sizeof(*inp));
      inp->semantic = inpv->semantic;
      if (inpv->semanticRaw)
        inp->semanticRaw = ak_heap_strdup(heap, inp, inpv->semanticRaw);

      inp->offset = prim->reserved1;
      inp->set    = prim->reserved2;
      inp->next   = prim->input;
      prim->input = inp;

      if (inp->semantic == AK_INPUT_POSITION) {
        prim->pos = inp;
        if (!rb_find(dst->meshInfo, prim->mesh)) {
          AkDaeMeshInfo *mi;

          mi      = ak_heap_calloc(heap, NULL, sizeof(*mi));
          mi->pos = inp;

          rb_insert(dst->meshInfo, prim->mesh, mi);
        }
      }

      if ((url = rb_find(dst->inputmap, inpv))) {
        ak_url_dup(url, inp, url);
        rb_insert(dst->inputmap, inp, url);
      }

      prim->inputCount++;
      inpv = inpv->next;
    }

    /* cleanup will be automatically,
       because same vertices may be used in multiple places
     */
  } while ((fitem = fitem->next));
}

AK_HIDE void
dae_postscript(DAEState * __restrict dst) {
  dae_spread_vert(dst);

  /* first migrate 1.4 to 1.5 */
  if (dst->version < AK_COLLADA_VERSION_150)
    dae14_loadjobs_finish(dst);

  dae_retain_refs(dst);
  rb_walk(dst->inputmap, dae_input_walk);
  dae_fixAngles(dst);
  dae_fixup_accessors(dst);
  dae_pre_mesh(dst);

  /* fixup when finished,
     because we need to collect about source/array usages
     also we can run fixups as parallels here
  */
  if (!ak_opt_get(AK_OPT_INDICES_DEFAULT))
    dae_geom_fixup_all(dst->doc);

  /* fixup morph and skin because order of vertices may be changed */
  if (dst->doc->lib.controllers) {
    dae_fixup_ctlr(dst);
    dae_fixup_instctlr(dst);
  }

  /* now set used coordSys */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
    dst->doc->coordSys = (void *)ak_opt_get(AK_OPT_COORD);

  dae_fix_textures(dst);
  
  if (dst->doc && dst->doc->lib.visualScenes) {
    for (AkVisualScene *vscn = (void *)dst->doc->lib.visualScenes->chld;
         vscn;
         vscn = (void *)vscn->base.next) {
      if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
        ak_fixSceneCoordSys(vscn);
    }
  }
}

AK_HIDE void
dae_retain_refs(DAEState * __restrict dst) {
  AkHeapAllocator *alc;
  AkURLQueue      *it, *tofree;
  AkURL           *url;
  AkHeapNode      *hnode;
  int             *refc;
  AkResult         ret;

  alc = dst->heap->allocator;
  it  = dst->urlQueue;

  while (it) {
    url    = it->url;
    tofree = it;

    /* currently only retain objects in this doc */
    if (it->url->doc == dst->doc) {
      hnode = NULL;
      ret   = ak_heap_getNodeByURL(dst->heap, url, &hnode);
      if (ret == AK_OK && hnode) {
        /* retain <source> and source arrays ... */
        refc         = ak_heap_ext_add(dst->heap, hnode, AK_HEAP_NODE_FLAGS_REFC);
        it->url->ptr = ak__alignas(hnode);

        (*refc)++;
      }
    }

    it = it->next;
    alc->free(tofree);
  }
}

AK_HIDE void
dae_input_walk(RBTree *tree, RBNode *rbnode) {
  AkAccessor *acc;
  AkSource   *src;
  AkInput    *inp;
  AkURL      *url;

  AK__UNUSED(tree);

  inp = rbnode->key;
  if (inp->semantic == AK_INPUT_SEMANTIC_VERTEX) {
    return;
  }

  url = rbnode->val;
  if (!(src = ak_getObjectByUrl(url)))
    return;

  acc           = src->tcommon;
  inp->accessor = acc;

  /* TODO: handle error if null?? */
  if (acc) {
    ak_retain(acc);
  }

  /* TODO: */
//  ak_free(src);
//  ak_free(url);
//
//  rb_destroy(tree);
}

AK_HIDE void
dae_fixup_accessors(DAEState * __restrict dst) {
  AkHeap        *heap;
  AkDoc         *doc;
  FListItem     *item;
  AkAccessor    *acc;
  AkAccessorDAE *accdae;
  AkBuffer      *buff;
  AkTypeDesc    *type;

  item = dst->accessors;
  heap = dst->heap;
  doc  = dst->doc;
  
  while (item) {
    acc         = item->data;
    accdae      = ak_userData(acc);
    buff        = ak_getObjectByUrl(&accdae->source);
    acc->buffer = buff;

    if ((buff = ak_getObjectByUrl(&accdae->source))) {
      AkBuffer    *newbuff;
      AkDataParam *dp;
      char        *olditms, *newitms;
      uint32_t     i, j, count, dpoff, bytesPerComponent;
      size_t       oldByteStride, newByteStride;

      acc->componentType = (AkTypeId)(uintptr_t)ak_userData(buff);

      if ((type = ak_typeDesc(acc->componentType)))
        bytesPerComponent = type->size;
      else
        goto cont;

      count                  = acc->count;
      acc->byteStride        = accdae->stride * bytesPerComponent;
      acc->byteLength        = count * accdae->stride * bytesPerComponent;
      acc->byteOffset        = accdae->offset * bytesPerComponent;
      accdae->bound          = accdae->stride;

      acc->fillByteSize      = accdae->bound * bytesPerComponent;
      acc->componentCount    = accdae->bound;
      acc->bytesPerComponent = bytesPerComponent;

      /*--------------------------------------------------------------------*

         eliminate / remove Data Params e.g. X, Y, Z
         to make Accessor more small and cleaner
       
       *--------------------------------------------------------------------*/
      
      /* the buffer is used more than one place, so duplicate data */
      /* TODO: check param that has empty name */
      if (acc->buffer && ak_refc(buff) > 1) {
        oldByteStride   = acc->byteStride;
        newByteStride   = accdae->bound * bytesPerComponent;
        newbuff         = ak_heap_calloc(heap, doc, sizeof(*newbuff));
        newbuff->length = count * newByteStride;
        newbuff->data   = ak_heap_calloc(heap, newbuff, newbuff->length);

        newitms         = (char *)newbuff->data;
        olditms         = (char *)buff->data + acc->byteOffset;
        
        for (i = 0; i < count; i++) {
          j     = 0;
          dpoff = 0;
          dp    = accdae->param;

          while (dp) {
            if (dp->name) {
              memcpy(newitms + newByteStride * i + bytesPerComponent * j++,
                     olditms + oldByteStride * i + dpoff,
                     dp->type.size);
            }

            dpoff += dp->type.size;
            dp     = dp->next;
          }
        }
        
        ak_release(acc->buffer);
        ak_retain(newbuff);

        acc->buffer = newbuff;
      }

      ak_heap_ext_rm(heap, ak__alignof(buff), AK_HEAP_NODE_FLAGS_USR);
    }

    ak_heap_ext_rm(heap, ak__alignof(accdae), AK_HEAP_NODE_FLAGS_USR);
    ak_free(accdae);

  cont:
    item = item->next;
  }

  flist_sp_destroy(&dst->accessors);
}

AK_HIDE void
dae_pre_walk(RBTree *tree, RBNode *rbnode) {
  AkDaeMeshInfo *mi;
  AkAccessor    *posAcc;

  AK__UNUSED(tree);

  mi     = rbnode->val;
  posAcc = NULL;

  if (!(posAcc = mi->pos->accessor))
    return;

  mi->nVertex = posAcc->count;
}

AK_HIDE void
dae_pre_mesh(DAEState * __restrict dst) {
  rb_walk(dst->meshInfo, dae_pre_walk);
}
