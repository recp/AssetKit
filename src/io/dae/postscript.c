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
#include "fixup/mesh.h"
#include "fixup/angle.h"
#include "fixup/tex.h"
#include "fixup/ctlr.h"
#include "fixup/channel.h"
#include "bugfix/scenekit.h"
#include "../../mat/internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

AK_HIDE void
dae_attach_orphan_morphs(DAEState * __restrict dst);

static void
dae_build_material_surfaces(DAEState * __restrict dst);

static
bool
dae_post_profile_enabled(void) {
  const char *value;

  value = getenv("ASSETKIT_DAE_PROFILE");
  if (!value)
    value = getenv("ASSETKIT_BLENDER_PROFILE");

  return value && value[0] && value[0] != '0';
}

static
double
dae_post_profile_now_ms(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static
void
dae_post_profile_log(bool enabled, const char *name, double start) {
  if (!enabled)
    return;

  fprintf(stderr,
          "[AssetKit DAE post] %s=%.3fms\n",
          name,
          dae_post_profile_now_ms() - start);
}

#define DAE_POST_PROFILE_CALL(PROFILE, NAME, CALL) do {                     \
    double _dae_post_profile_start = 0.0;                                   \
    if (PROFILE)                                                            \
      _dae_post_profile_start = dae_post_profile_now_ms();                  \
    CALL;                                                                   \
    dae_post_profile_log(PROFILE, NAME, _dae_post_profile_start);           \
  } while (0)

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
      inp              = dae_input_new(heap, prim);
      inp->semantic    = inpv->semantic;
      inp->semanticRaw = inpv->semanticRaw;

      inp->indexOffset = prim->reserved1;
      inp->set         = prim->reserved2;
      inp->next        = prim->input;
      prim->input      = inp;

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
  AkCoordCvtType coordCvtType;
  AkCoordSys    *sourceCoordSys, *targetCoordSys;
  bool           fixTransform;
  bool           profile;

  profile         = dae_post_profile_enabled();
  coordCvtType    = (AkCoordCvtType)ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  sourceCoordSys  = dst->doc ? dst->doc->coordSys : NULL;
  targetCoordSys  = (void *)ak_opt_get(AK_OPT_COORD);
  fixTransform    = coordCvtType == AK_COORD_CVT_FIX_TRANSFORM
                    && sourceCoordSys
                    && targetCoordSys
                    && sourceCoordSys != targetCoordSys
                    && !ak_coordOrientationIsEq(sourceCoordSys, targetCoordSys);

  DAE_POST_PROFILE_CALL(profile, "spread_vert", dae_spread_vert(dst));

  /* first migrate 1.4 to 1.5 */
  if (dst->version < AK_COLLADA_VERSION_150)
    DAE_POST_PROFILE_CALL(profile, "dae14_loadjobs_finish",
                          dae14_loadjobs_finish(dst));

  DAE_POST_PROFILE_CALL(profile, "retain_refs", dae_retain_refs(dst));
  DAE_POST_PROFILE_CALL(profile, "input_walk",
                        rb_walk(dst->inputmap, dae_input_walk));
  DAE_POST_PROFILE_CALL(profile, "fix_angles", dae_fixAngles(dst));
  DAE_POST_PROFILE_CALL(profile, "fixup_accessors", dae_fixup_accessors(dst));
  DAE_POST_PROFILE_CALL(profile, "pre_mesh", dae_pre_mesh(dst));
  DAE_POST_PROFILE_CALL(profile, "scenekit_backfaces",
                        dae_bugfix_scenekit_backfaces(dst));

  /* fixup when finished,
     because we need to collect about source/array usages
     also we can run fixups as parallels here
  */
  if (!ak_opt_get(AK_OPT_INDICES_DEFAULT))
  {
    if (profile)
      dae_mesh_profile_reset();
    DAE_POST_PROFILE_CALL(profile, "geom_fixup_all",
                          dae_geom_fixup_all(dst->doc));
    if (profile)
      dae_mesh_profile_report();
  }

  /* fixup morph and skin because order of vertices may be changed */
  if (dst->doc->lib.controllers) {
    DAE_POST_PROFILE_CALL(profile, "fixup_ctlr", dae_fixup_ctlr(dst));
    DAE_POST_PROFILE_CALL(profile, "fixup_instctlr", dae_fixup_instctlr(dst));

    /* Soft attach: many DAE assets — particularly glTF→DAE converted ones —
       reference the morph base mesh via <instance_geometry url="#mesh"/>
       and forget to wrap it in <instance_controller>. The morph controller
       is in the library but never instanced, so dae_fixup_instctlr never
       sees it. Walk the scene graph and attach any morph controller that
       targets this geometry. */
    DAE_POST_PROFILE_CALL(profile, "attach_orphan_morphs",
                          dae_attach_orphan_morphs(dst));
  }

  /* Resolve animation channel targets that need controller/instance
     topology — currently the indexed-array form used for morph weights,
     e.g. <channel target="morph-weights(0)"/>. Must run after morph
     instances exist (including the orphan-attach pass above). */
  if (dst->doc->lib.animations)
    DAE_POST_PROFILE_CALL(profile, "fixup_channel", dae_fixup_channel(dst));

  /* now set used coordSys */
  if (coordCvtType != AK_COORD_CVT_DISABLED)
    dst->doc->coordSys = targetCoordSys;

  DAE_POST_PROFILE_CALL(profile, "fix_textures", dae_fix_textures(dst));
  DAE_POST_PROFILE_CALL(profile, "material_surfaces",
                        dae_build_material_surfaces(dst));
  DAE_POST_PROFILE_CALL(profile, "scenekit_materials",
                        dae_bugfix_scenekit_material_surfaces(dst));
  
  if (dst->doc && dst->doc->lib.visualScenes) {
    double coordStart = 0.0;
    if (profile)
      coordStart = dae_post_profile_now_ms();
    for (AkVisualScene *vscn = (void *)dst->doc->lib.visualScenes->chld;
         vscn;
         vscn = (void *)vscn->base.next) {
      if (fixTransform)
        ak_fixSceneCoordSys(vscn);
    }
    dae_post_profile_log(profile, "fix_scene_coord", coordStart);
  }
}

static void
dae_build_material_surfaces(DAEState * __restrict dst) {
  AkLibrary            *libmat;
  AkMaterial           *material;
  AkEffect             *effect;
  AkTechniqueFxCommon  *common;

  if (!dst || !dst->doc || !(libmat = dst->doc->lib.materials))
    return;

  material = (void *)libmat->chld;
  while (material) {
    if (!material->surface
        && (effect = ak_materialEffect(material))
        && (common = ak_getProfileTechniqueCommon(effect))) {
      material->surface = ak_materialSurfaceFromTechniqueCommon(dst->heap, material, common);
    }

    material = (void *)material->base.next;
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

/*
 * For every <instance_geometry> in the scene graph, check whether the
 * geometry it references has a registered morph controller in
 * meshTargets. If so, synthesize an AkInstanceMorph and attach it. This
 * compensates for DAE exporters (typically glTF→DAE converters) that
 * emit a morph controller but reference the base geometry directly via
 * <instance_geometry> rather than wrapping it in <instance_controller>.
 *
 * Idempotent: skips instGeoms that already have a morpher (real
 * <instance_controller> path already attached one in dae_fixup_instctlr).
 */
static
void
dae_attach_orphan_morphs_node(DAEState * __restrict dst, AkNode *node) {
  AkInstanceGeometry *instGeom;
  AkInstanceMorph    *instMorph;
  AkMorph            *morph;
  AkGeometry         *geom;

  for (; node; node = (AkNode *)node->next) {
    for (instGeom = node->geometry; instGeom;
         instGeom = (AkInstanceGeometry *)instGeom->base.next) {
      if (instGeom->morpher)                         continue;
      if (!(geom = instGeom->base.url.ptr))          continue;
      if (ak_typeid(geom) != AKT_GEOMETRY)           continue;
      if (!(morph = rb_find(dst->meshTargets, geom))) continue;

      instMorph                  = ak_heap_calloc(dst->heap, node,
                                                  sizeof(*instMorph));
      instMorph->morph           = morph;
      instMorph->overrideWeights = NULL;
      instGeom->morpher          = instMorph;
    }

    if (node->chld)
      dae_attach_orphan_morphs_node(dst, node->chld);
  }
}

AK_HIDE void
dae_attach_orphan_morphs(DAEState * __restrict dst) {
  AkVisualScene *vscn;

  if (!dst->meshTargets || !dst->doc->lib.visualScenes) return;

  for (vscn = (void *)dst->doc->lib.visualScenes->chld;
       vscn;
       vscn = (void *)vscn->base.next) {
    dae_attach_orphan_morphs_node(dst, vscn->node);
  }
}

AK_HIDE void
dae_input_walk(RBTree *tree, RBNode *rbnode) {
  AkAccessor *acc;
  DaeSource  *src;
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

  acc           = src->accessor;
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
        newbuff->data   = ak_heap_alloc(heap, newbuff, newbuff->length);

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
