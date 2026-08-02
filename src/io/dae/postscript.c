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
#include "strpool.h"
#include "../../xml.h"

#include "1.4/dae14.h"

#include "core/index_parse.h"
#include "core/source.h"
#include "fixup/geom.h"
#include "fixup/mesh.h"
#include "fixup/angle.h"
#include "fixup/tex.h"
#include "fixup/ctlr.h"
#include "fixup/channel.h"
#include "bugfix/scenekit.h"
#include "../../mat/internal.h"
#include "../../instance/list.h"

#include <string.h>

AK_HIDE void
dae_retain_refs(DAEState * __restrict dst);

static void
dae_bind_active_scene(DAEState * __restrict dst);

static void
dae_apply_bind_materials(DAEState * __restrict dst);

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

static void
dae_material_attach_effect_extras(DAEState   * __restrict dst,
                                  AkMaterial * __restrict material,
                                  AkEffect   * __restrict effect);

static AkComponentSize
dae_component_size_for_accessor(uint32_t componentCount,
                                AkDataParam * __restrict singleNamedParam) {
  if (singleNamedParam) {
    switch (singleNamedParam->type.typeId) {
      case AKT_FLOAT2:   return AK_COMPONENT_SIZE_VEC2;
      case AKT_FLOAT3:   return AK_COMPONENT_SIZE_VEC3;
      case AKT_FLOAT4:   return AK_COMPONENT_SIZE_VEC4;
      case AKT_FLOAT2x2: return AK_COMPONENT_SIZE_MAT2;
      case AKT_FLOAT3x3: return AK_COMPONENT_SIZE_MAT3;
      case AKT_FLOAT4x4: return AK_COMPONENT_SIZE_MAT4;
      default:
        break;
    }
  }

  switch (componentCount) {
    case 1:  return AK_COMPONENT_SIZE_SCALAR;
    case 2:  return AK_COMPONENT_SIZE_VEC2;
    case 3:  return AK_COMPONENT_SIZE_VEC3;
    case 4:  return AK_COMPONENT_SIZE_VEC4;
    case 9:  return AK_COMPONENT_SIZE_MAT3;
    case 16: return AK_COMPONENT_SIZE_MAT4;
    default:
      break;
  }

  return AK_COMPONENT_SIZE_UNKNOWN;
}

static bool
dae_accessor_component_byte_size(AkTypeId typeId,
                                 uint32_t * __restrict size) {
  AkTypeDesc *type;

  if ((type = ak_typeDesc(typeId))) {
    *size = (uint32_t)type->size;
    return *size > 0;
  }

  switch (typeId) {
    case AKT_IDREF:
    case AKT_NAME:
    case AKT_SIDREF:
    case AKT_TOKEN:
      *size = sizeof(char *);
      return true;
    default:
      break;
  }

  return false;
}

static void
dae_mark_accessor_type_buffer(RBTree    * __restrict typeBuffers,
                              AkBuffer  * __restrict buff) {
  if (typeBuffers && buff
      && ak_heap_ext_get(ak__alignof(buff), AK_HEAP_NODE_FLAGS_USR)
      && !rb_find(typeBuffers, buff))
    rb_insert(typeBuffers, buff, (void *)1);
}

static void
dae_clear_accessor_type_buffer(RBTree *tree, RBNode *rbnode) {
  AkBuffer *buff;
  AkHeap   *heap;

  AK__UNUSED(tree);

  buff = rbnode->key;
  if (!buff || !(heap = ak_heap_getheap(buff)))
    return;

  ak_heap_ext_rm(heap, ak__alignof(buff), AK_HEAP_NODE_FLAGS_USR);
}

static
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

    vert = ak_getObjectByUrl(url);
    if (!vert)
      vert = item->fallbackVertices;
    if (!vert)
      continue;

    inpv = vert->input;

    while (inpv) {
      inp              = dae_input_new(heap, prim);
      inp->semantic    = inpv->semantic;
      inp->semanticRaw = inpv->semanticRaw;

      inp->indexOffset = item->indexOffset;
      inp->set         = inpv->set;
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
        /* inputmap borrows this source URL only during post-processing. */
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

  coordCvtType    = (AkCoordCvtType)ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  sourceCoordSys  = dst->doc ? dst->doc->coordSys : NULL;
  targetCoordSys  = (void *)ak_opt_get(AK_OPT_COORD);
  fixTransform    = coordCvtType == AK_COORD_CVT_FIX_TRANSFORM
                    && sourceCoordSys
                    && targetCoordSys
                    && sourceCoordSys != targetCoordSys
                    && !ak_coordOrientationIsEq(sourceCoordSys, targetCoordSys);

  dae_parse_float_sources(dst);
  dae_parse_index_arrays(dst);
  dae_spread_vert(dst);

  /* first migrate 1.4 to 1.5 */
  if (dst->version < AK_COLLADA_VERSION_150)
    dae14_loadjobs_finish(dst);

  dae_retain_refs(dst);
  dae_apply_bind_materials(dst);
  dae_bind_active_scene(dst);
  rb_walk(dst->inputmap, dae_input_walk);
  dae_fixAngles(dst);
  dae_fixup_accessors(dst);
  dae_pre_mesh(dst);
  dae_scenekit_normalize_colors(dst);
  dae_bugfix_scenekit_backfaces(dst);

  /* fixup when finished,
     because we need to collect about source/array usages
     also we can run fixups as parallels here
  */
  if (!ak_opt_get(AK_OPT_INDICES_DEFAULT))
  {
    dae_geom_fixup_all(dst->doc, dst->controllers != NULL);
  }

  /* fixup morph and skin because order of vertices may be changed */
  if (dst->controllers) {
    dae_fixup_ctlr(dst);
    dae_fixup_instctlr(dst);

    /* Soft attach: many DAE assets — particularly glTF→DAE converted ones —
       reference the morph base mesh via <instance_geometry url="#mesh"/>
       and forget to wrap it in <instance_controller>. The morph controller
       is in the library but never instanced, so dae_fixup_instctlr never
       sees it. Walk the scene graph and attach any morph controller that
       targets this geometry. */
    dae_attach_orphan_morphs(dst);
  }

  /* Resolve animation channel targets that need controller/instance
     topology — currently the indexed-array form used for morph weights,
     e.g. <channel target="morph-weights(0)"/>. Must run after morph
     instances exist (including the orphan-attach pass above). */
  if (dst->doc->lib.animations.first)
    dae_fixup_channel(dst);
  dae_scenekit_normalize_animation_colors(dst);

  /* now set used coordSys */
  if (coordCvtType != AK_COORD_CVT_DISABLED)
    dst->doc->coordSys = targetCoordSys;

  dae_fix_textures(dst);
  dae_build_material_surfaces(dst);
  dae_bugfix_scenekit_material_surfaces(dst);
  
  if (dst->doc && dst->doc->lib.scenes.first) {
    for (AkScene *vscn = dst->doc->lib.scenes.first;
         vscn;
         vscn = vscn->next) {
      if (fixTransform)
        ak_fixSceneCoordSys(vscn);
    }
  }
}

static void
dae_apply_bind_materials(DAEState * __restrict dst) {
  AkDAEBindMaterialUse *use;
  FListItem            *item;

  if (!dst)
    return;

  for (item = dst->bindMaterials; item; item = item->next) {
    use = item->data;
    if (use)
      ak__instanceGeometryApplyBindMaterial(use->instance, use->bindMaterial);
  }
}

static void
dae_bind_active_scene(DAEState * __restrict dst) {
  AkScene *scene;
  void    *resolved;

  if (!dst || !dst->doc)
    return;

  if (dst->activeScene.url) {
    resolved = ak_getObjectByUrl(&dst->activeScene);
    if (resolved && ak_typeid(resolved) == AKT_SCENE)
      dst->doc->scene = resolved;
  }

  if (!dst->doc->scene && (scene = dst->doc->lib.scenes.first))
    dst->doc->scene = scene;
}

static void
dae_extra_append_child(AkTreeNode * __restrict parent,
                       AkTreeNode * __restrict child) {
  AkTreeNode *tail;

  if (!parent || !child)
    return;

  child->parent = parent;
  child->next   = NULL;
  child->prev   = NULL;

  if (!parent->chld) {
    parent->chld = child;
    parent->chldc++;
    return;
  }

  tail = parent->chld;
  while (tail->next)
    tail = tail->next;

  tail->next = child;
  child->prev = tail;
  parent->chldc++;
}

static AkTreeNode*
dae_extra_clone_node(AkHeap           * __restrict heap,
                     AkTreeNode       * __restrict parent,
                     const AkTreeNode * __restrict src) {
  const AkTreeNodeAttr *sattr;
  const AkTreeNode     *schild;
  AkTreeNodeAttr      **attrTail;
  AkTreeNodeAttr       *prevAttr;
  AkTreeNode           *dst;

  if (!heap || !parent || !src)
    return NULL;

  dst = ak_heap_calloc(heap, parent, sizeof(*dst));
  if (src->name)
    dst->name = ak_heap_strdup(heap, dst, src->name);
  if (src->val)
    dst->val = ak_heap_strdup(heap, dst, src->val);

  attrTail = &dst->attribs;
  prevAttr = NULL;
  for (sattr = src->attribs; sattr; sattr = sattr->next) {
    AkTreeNodeAttr *attr;

    attr = ak_heap_calloc(heap, dst, sizeof(*attr));
    if (sattr->name)
      attr->name = ak_heap_strdup(heap, attr, sattr->name);
    if (sattr->val)
      attr->val = ak_heap_strdup(heap, attr, sattr->val);

    attr->prev = prevAttr;
    *attrTail  = attr;
    prevAttr   = attr;
    attrTail   = &attr->next;
    dst->attrc++;
  }

  for (schild = src->chld; schild; schild = schild->next)
    dae_extra_append_child(dst, dae_extra_clone_node(heap, dst, schild));

  return dst;
}

static void
dae_extra_clone_children(AkHeap           * __restrict heap,
                         AkTreeNode       * __restrict parent,
                         const AkTreeNode * __restrict srcRoot) {
  const AkTreeNode *child;

  if (!heap || !parent || !srcRoot)
    return;

  for (child = srcRoot->chld; child; child = child->next)
    dae_extra_append_child(parent, dae_extra_clone_node(heap, parent, child));
}

static AkTreeNode*
dae_material_extra_root(DAEState   * __restrict dst,
                        AkMaterial * __restrict material) {
  AkTreeNode *root;

  if (!dst || !material)
    return NULL;

  root = ak_extra(material);
  if (!root) {
    root       = ak_heap_calloc(dst->heap, material, sizeof(*root));
    root->name = ak_heap_strdup(dst->heap, root, _s_dae_extra);
    ak_extra_set(material, root);
  }

  return root;
}

static AkTreeNode*
dae_extra_append_wrapper(DAEState   * __restrict dst,
                         AkMaterial * __restrict material,
                         AkTreeNode * __restrict parent,
                         const char * __restrict name) {
  AkTreeNode *node;

  if (!dst || !material || !parent || !name)
    return NULL;

  node       = ak_heap_calloc(dst->heap, parent, sizeof(*node));
  node->name = ak_heap_strdup(dst->heap, node, name);
  dae_extra_append_child(parent, node);

  return node;
}

static void
dae_material_attach_effect_extras(DAEState   * __restrict dst,
                                  AkMaterial * __restrict material,
                                  AkEffect   * __restrict effect) {
  AkProfile     *profile;
  AkTreeNode    *root;

  if (!dst || !material || !effect)
    return;
  if (!ak_opt_get(AK_OPT_PRESERVE_EXTRAS))
    return;

  root = NULL;
  if (effect->extra && effect->extra->chld) {
    AkTreeNode *wrapper;

    root = dae_material_extra_root(dst, material);
    wrapper = dae_extra_append_wrapper(dst, material, root, "effect");
    dae_extra_clone_children(dst->heap, wrapper, effect->extra);
  }

  for (profile = effect->profile; profile; profile = profile->next) {
    AkTechniqueFx *techn;
    AkTreeNode    *profileNode;
    const char    *profileName;
    bool           hasExtra;

    hasExtra = profile->extra && profile->extra->chld;
    for (techn = profile->technique; !hasExtra && techn; techn = techn->next)
      hasExtra = techn->extra && techn->extra->chld;

    if (!hasExtra)
      continue;

    if (!root)
      root = dae_material_extra_root(dst, material);

    profileName = profile->type == AK_PROFILE_TYPE_COMMON ? "profile_COMMON" : "profile";
    profileNode = dae_extra_append_wrapper(dst, material, root, profileName);
    if (profile->extra && profile->extra->chld)
      dae_extra_clone_children(dst->heap, profileNode, profile->extra);

    for (techn = profile->technique; techn; techn = techn->next) {
      AkTreeNode *techniqueNode;

      if (!techn->extra || !techn->extra->chld)
        continue;

      techniqueNode = dae_extra_append_wrapper(dst, material, profileNode, "technique");
      dae_extra_clone_children(dst->heap, techniqueNode, techn->extra);
    }
  }
}

static void
dae_build_material_surfaces(DAEState * __restrict dst) {
  AkMaterial           *material;
  AkEffect             *effect;
  AkTechniqueFxCommon  *common;

  if (!dst || !dst->doc)
    return;

  for (material = dst->doc->lib.materials.first; material; material = material->next) {
    effect = dae_material_effect(dst, material);
    if (!effect)
      continue;

    dae_material_attach_effect_extras(dst, material, effect);

    if (!material->surface
        && (common = ak_getProfileTechniqueCommon(effect))) {
      material->surface = ak_materialSurfaceFromTechniqueCommon(dst->heap,
                                                                 material,
                                                                 common,
                                                                 false);
    }
  }
}

AK_HIDE void
dae_retain_refs(DAEState * __restrict dst) {
  AkHeapAllocator *alc;
  AkURLQueue      *it, *tofree;
  AkURL           *url;
  AkNode          *targetNode;
  AkHeapNode      *hnode;
  int             *refc;
  AkResult         ret;

  alc = dst->heap->allocator;
  it  = dst->urlQueue;

  while (it) {
    url    = it->url;
    tofree = it;

    if (!url)
      goto next;

    /* Same-document refs can be retained by this document heap. External
       refs remain owned by their source document; resolve only cached
       instance targets / scene item lists here. */
    if (url->doc == dst->doc) {
      hnode = NULL;
      ret   = ak_heap_getNodeByURL(dst->heap, url, &hnode);
      if (ret == AK_OK && hnode) {
        /* retain <source> and source arrays ... */
        refc     = ak_heap_ext_add(dst->heap, hnode, AK_HEAP_NODE_FLAGS_REFC);
        url->ptr = ak__alignas(hnode);

        (*refc)++;
      }
    }

    if (it->scene && it->instance) {
      switch (it->instance->type) {
        case AK_INSTANCE_CAMERA:
          ak_sceneAddCamera(it->scene, it->instance);
          break;
        case AK_INSTANCE_LIGHT:
          ak_sceneAddLight(it->scene, it->instance);
          break;
        default:
          break;
      }
    }

    if (it->scene && it->nodeRef) {
      targetNode = ak_instanceNodeTarget(it->nodeRef);
      if (targetNode)
        ak_sceneAddItems(it->scene, targetNode);
    }

next:
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

    if (node->node) {
      AkInstanceNode *instNode;

      for (instNode = node->node; instNode; instNode = instNode->next) {
        AkNode *target;

        target = ak_instanceNodeTarget(instNode);
        if (target)
          dae_attach_orphan_morphs_node(dst, target);
      }
    }
  }
}

AK_HIDE void
dae_attach_orphan_morphs(DAEState * __restrict dst) {
  AkScene *vscn;

  if (!dst->meshTargets || !dst->doc->lib.scenes.first) return;

  for (vscn = dst->doc->lib.scenes.first;
       vscn;
       vscn = vscn->next) {
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
  RBTree        *typeBuffers;

  item        = dst->accessors;
  heap        = dst->heap;
  doc         = dst->doc;
  typeBuffers = rb_newtree_ptr();
  
  while (item) {
    acc         = item->data;
    accdae      = ak_userData(acc);
    buff        = ak_getObjectByUrl(&accdae->source);
    acc->buffer = buff;

    if (buff) {
      AkBuffer        *newbuff;
      AkDataParam     *dp;
      AkDataParam     *singleNamedParam;
      char            *olditms, *newitms;
      uint32_t         i, count, bytesPerComponent, visibleComponents;
      uint32_t         namedParamCount;
      size_t           oldByteStride, newByteStride;
      size_t           paramOffset, firstVisibleOffset, nextVisibleOffset;
      size_t           visibleByteSize, paramSize;
      bool             hasParams, compactNeeded, contiguousVisible;

      dae_mark_accessor_type_buffer(typeBuffers, buff);
      acc->componentType = (AkTypeId)(uintptr_t)ak_userData(buff);

      if (!dae_accessor_component_byte_size(acc->componentType,
                                            &bytesPerComponent))
        goto cleanup_accessor;

      count             = acc->count;
      oldByteStride     = accdae->stride * bytesPerComponent;
      hasParams         = accdae->param != NULL;
      compactNeeded     = false;
      contiguousVisible = true;
      firstVisibleOffset = 0;
      nextVisibleOffset  = 0;
      visibleByteSize    = 0;
      namedParamCount    = 0;
      singleNamedParam   = NULL;
      paramOffset        = 0;

      if (hasParams) {
        for (dp = accdae->param; dp; dp = dp->next) {
          paramSize = dp->type.size > 0
                      ? (size_t)dp->type.size
                      : (size_t)bytesPerComponent;

          if (dp->name && *dp->name) {
            if (namedParamCount == 0) {
              firstVisibleOffset = paramOffset;
              nextVisibleOffset  = paramOffset;
              singleNamedParam   = dp;
            } else if (paramOffset != nextVisibleOffset) {
              contiguousVisible = false;
              singleNamedParam = NULL;
            } else {
              singleNamedParam = NULL;
            }

            nextVisibleOffset = paramOffset + paramSize;
            visibleByteSize  += paramSize;
            namedParamCount++;
          }

          paramOffset += paramSize;
        }

        if (visibleByteSize == 0 && namedParamCount == 0) {
          firstVisibleOffset = 0;
          visibleByteSize    = oldByteStride;
        }
      } else {
        visibleByteSize = oldByteStride;
      }

      if (visibleByteSize > UINT32_MAX
          || (bytesPerComponent == 0)
          || (visibleByteSize % bytesPerComponent) != 0)
        goto cleanup_accessor;

      visibleComponents      = (uint32_t)(visibleByteSize / bytesPerComponent);
      accdae->bound          = visibleComponents;
      acc->bytesPerComponent = bytesPerComponent;
      acc->componentCount    = visibleComponents;
      acc->componentSize     = dae_component_size_for_accessor(visibleComponents,
                                                               singleNamedParam);
      acc->byteOffset        = accdae->offset * bytesPerComponent;
      acc->fillByteSize      = visibleByteSize;

      if (hasParams && visibleByteSize > 0) {
        compactNeeded = !contiguousVisible;
        if (!compactNeeded)
          acc->byteOffset += firstVisibleOffset;
      }

      if (!compactNeeded) {
        acc->byteStride = oldByteStride;
        acc->byteLength = count > 0
                          ? (size_t)(count - 1u) * acc->byteStride
                            + acc->fillByteSize
                          : 0;
      } else {
        newByteStride   = visibleByteSize;
        newbuff         = ak_heap_calloc(heap, doc, sizeof(*newbuff));
        newbuff->name   = buff->name;
        newbuff->length = count * newByteStride;
        newbuff->data   = ak_heap_alloc(heap, newbuff, newbuff->length);

        newitms = (char *)newbuff->data;
        olditms = (char *)buff->data + accdae->offset * bytesPerComponent;

        for (i = 0; i < count; i++) {
          size_t dstOffset;
          size_t srcOffset;

          dstOffset = 0;
          srcOffset = 0;
          for (dp = accdae->param; dp; dp = dp->next) {
            paramSize = dp->type.size > 0
                        ? (size_t)dp->type.size
                        : (size_t)bytesPerComponent;

            if (dp->name && *dp->name) {
              memcpy(newitms + newByteStride * i + dstOffset,
                     olditms + oldByteStride * i + srcOffset,
                     paramSize);
              dstOffset += paramSize;
            }

            srcOffset += paramSize;
          }
        }

        ak_release(acc->buffer);
        ak_retain(newbuff);
        ak_setUserData(newbuff, (void *)(uintptr_t)acc->componentType);
        dae_mark_accessor_type_buffer(typeBuffers, newbuff);
        AK_LIB_PREPEND(doc->lib.buffers, newbuff, next);

        acc->buffer     = newbuff;
        acc->byteOffset = 0;
        acc->byteStride = newByteStride;
        acc->byteLength = count * newByteStride;
      }

      /* Keep source-array type metadata until all accessors are fixed up.
         Shared-array sources may be referenced by multiple accessors. */
    }

  cleanup_accessor:
    ak_heap_ext_rm(heap, ak__alignof(accdae), AK_HEAP_NODE_FLAGS_USR);
    ak_free(accdae);

    item = item->next;
  }

  if (typeBuffers) {
    rb_walk(typeBuffers, dae_clear_accessor_type_buffer);
    rb_destroy(typeBuffers);
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
