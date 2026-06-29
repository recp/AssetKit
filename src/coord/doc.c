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

#include "../common.h"
#include "common.h"

typedef struct AkCoordPtrSet {
  const void **keys;
  size_t      capacity;
  size_t      count;
} AkCoordPtrSet;

typedef struct AkCoordDocCvt {
  AkDoc        *doc;
  AkCoordSys   *oldCoordSys;
  AkCoordSys   *newCoordSys;
  AkCoordPtrSet nodes;
  AkCoordPtrSet geoms;
  AkCoordPtrSet accessors;
} AkCoordDocCvt;

static
uintptr_t
ak_coord_ptr_hash(const void *key) {
  uintptr_t h;

  h  = (uintptr_t)key >> 4;
  h ^= h >> 16;
  h *= (uintptr_t)0x45d9f3bu;
  h ^= h >> 16;
  return h ? h : 1u;
}

static
bool
ak_coord_ptrset_grow(AkCoordPtrSet * __restrict set,
                     size_t                     minCapacity) {
  const void **keys, **oldKeys;
  size_t       oldCap, newCap, i;

  oldCap = set->capacity;
  newCap = oldCap ? oldCap << 1u : 64u;
  while (newCap < minCapacity)
    newCap <<= 1u;

  keys = ak_calloc(NULL, sizeof(*keys) * newCap);
  if (!keys)
    return false;

  oldKeys = set->keys;
  for (i = 0; i < oldCap; i++) {
    const void *key;
    uintptr_t   h, mask;

    if (!(key = oldKeys[i]))
      continue;

    h    = ak_coord_ptr_hash(key);
    mask = newCap - 1u;
    for (;;) {
      const void **slot;

      slot = &keys[h & mask];
      if (!*slot) {
        *slot = key;
        break;
      }
      h++;
    }
  }

  set->keys     = keys;
  set->capacity = newCap;
  if (oldKeys)
    ak_free((void *)oldKeys);

  return true;
}

static
bool
ak_coord_ptrset_seen(AkCoordPtrSet * __restrict set,
                     const void    * __restrict key) {
  uintptr_t h, mask;

  if (!key)
    return true;

  if (!set->keys
      || (set->count + 1u) * 4u >= set->capacity * 3u) {
    if (!ak_coord_ptrset_grow(set, set->count + 1u))
      return false;
  }

  h    = ak_coord_ptr_hash(key);
  mask = set->capacity - 1u;
  for (;;) {
    const void **slot;

    slot = &set->keys[h & mask];
    if (*slot == key)
      return true;
    if (!*slot) {
      *slot = key;
      set->count++;
      return false;
    }
    h++;
  }
}

static
void
ak_coord_ptrset_free(AkCoordPtrSet * __restrict set) {
  if (set->keys)
    ak_free((void *)set->keys);
  set->keys = NULL;
  set->capacity = set->count = 0;
}

static
bool
ak_coord_cvt_accessor_quat(AkAccessor * __restrict acc,
                           AkCoordSys * __restrict oldCoordSys,
                           AkCoordSys * __restrict newCoordSys) {
  unsigned char *data;
  size_t         rowBytes, stride, last;
  uint32_t       i;

  if (!acc
      || !oldCoordSys
      || !newCoordSys
      || oldCoordSys == newCoordSys
      || acc->count == 0
      || acc->componentCount < 4)
    return true;

  if (acc->componentType != AKT_FLOAT
      || acc->normalized
      || acc->bytesPerComponent != sizeof(float))
    ak_accessorMakeFloat(acc);

  if (acc->componentType != AKT_FLOAT
      || acc->normalized
      || acc->bytesPerComponent != sizeof(float)
      || !acc->buffer
      || !acc->buffer->data)
    return false;

  rowBytes = (size_t)acc->componentCount * sizeof(float);
  stride   = acc->byteStride ? acc->byteStride : rowBytes;
  if (stride < sizeof(float) * 4u
      || acc->byteOffset > acc->buffer->length)
    return false;

  if ((size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;
  last = acc->byteOffset + (size_t)(acc->count - 1u) * stride
         + sizeof(float) * 4u;
  if (last > acc->buffer->length)
    return false;

  data = (unsigned char *)acc->buffer->data + acc->byteOffset;

  for (i = 0; i < acc->count; i++) {
    float *row;

    row = (float *)(void *)(data + (size_t)i * stride);
    ak_coordCvtQuatTo(oldCoordSys, row, newCoordSys);
  }

  return true;
}

static
void
ak_coord_doc_cvt_vec3(AkCoordDocCvt * __restrict st,
                      AkAccessor    * __restrict acc,
                      bool                       noSign) {
  if (!acc || ak_coord_ptrset_seen(&st->accessors, acc))
    return;

  ak_coordCvtAccessorVec3(acc, st->oldCoordSys, st->newCoordSys, noSign);
}

static
void
ak_coord_doc_cvt_quat(AkCoordDocCvt * __restrict st,
                      AkAccessor    * __restrict acc) {
  if (!acc || ak_coord_ptrset_seen(&st->accessors, acc))
    return;

  ak_coord_cvt_accessor_quat(acc, st->oldCoordSys, st->newCoordSys);
}

static
void
ak_coord_doc_cvt_input(AkCoordDocCvt * __restrict st,
                       AkInput       * __restrict input) {
  for (; input; input = input->next) {
    switch (input->semantic) {
      case AK_INPUT_POSITION:
      case AK_INPUT_NORMAL:
      case AK_INPUT_TANGENT:
      case AK_INPUT_BINORMAL:
      case AK_INPUT_TEXBINORMAL:
      case AK_INPUT_TEXTANGENT:
        ak_coord_doc_cvt_vec3(st, input->accessor, false);
        break;
      default:
        break;
    }
  }
}

static
void
ak_coord_doc_cvt_geom(AkCoordDocCvt * __restrict st,
                      AkGeometry    * __restrict geom) {
  AkObject *primitive;

  if (!geom || ak_coord_ptrset_seen(&st->geoms, geom))
    return;

  primitive = geom->gdata;
  if (primitive) {
    switch ((AkGeometryType)primitive->type) {
      case AK_GEOMETRY_MESH: {
        AkMeshPrimitive *prim;
        AkMesh          *mesh;

        mesh = ak_objGet(primitive);
        if (mesh) {
          ak_coordCvtVector(st->oldCoordSys, mesh->center, st->newCoordSys);
          if (mesh->bbox)
            mesh->bbox->isvalid = false;

          for (prim = mesh->primitive; prim; prim = prim->next) {
            ak_coordCvtVector(st->oldCoordSys,
                              prim->center,
                              st->newCoordSys);
            ak_coord_doc_cvt_input(st, prim->input);
            if (prim->pos)
              ak_coord_doc_cvt_input(st, prim->pos);
            if (prim->bbox)
              prim->bbox->isvalid = false;
          }
        }
        break;
      }
      case AK_GEOMETRY_SPLINE:
      case AK_GEOMETRY_BREP:
        break;
    }
  }

  if (geom->bbox)
    geom->bbox->isvalid = false;
}

static
AkAnimSampler*
ak_coord_doc_channel_sampler(AkChannel * __restrict channel) {
  AkAnimSampler *sampler;

  if (!channel)
    return NULL;

  sampler = channel->source.ptr;
  if (!sampler)
    sampler = ak_getObjectByUrl(&channel->source);

  return sampler;
}

static
AkAccessor*
ak_coord_doc_sampler_accessor(AkAnimSampler  * __restrict sampler,
                              AkInputSemantic             semantic) {
  AkInput *input;

  if (!sampler)
    return NULL;

  switch (semantic) {
    case AK_INPUT_OUTPUT:
      if (sampler->outputInput)
        return sampler->outputInput->accessor;
      break;
    case AK_INPUT_IN_TANGENT:
      if (sampler->inTangentInput)
        return sampler->inTangentInput->accessor;
      break;
    case AK_INPUT_OUT_TANGENT:
      if (sampler->outTangentInput)
        return sampler->outTangentInput->accessor;
      break;
    default:
      break;
  }

  for (input = sampler->input; input; input = input->next) {
    if (input->semantic == semantic)
      return input->accessor;
  }

  return NULL;
}

static
void
ak_coord_doc_cvt_anim_channel(AkCoordDocCvt * __restrict st,
                              AkChannel     * __restrict channel) {
  AkAnimSampler *sampler;
  bool           noSign;

  sampler = ak_coord_doc_channel_sampler(channel);
  if (!sampler)
    return;

  switch (channel->targetType) {
    case AK_TARGET_POSITION:
    case AK_TARGET_ROTATE:
      ak_coord_doc_cvt_vec3(st,
                            ak_coord_doc_sampler_accessor(sampler,
                                                          AK_INPUT_OUTPUT),
                            false);
      ak_coord_doc_cvt_vec3(st,
                            ak_coord_doc_sampler_accessor(sampler,
                                                          AK_INPUT_IN_TANGENT),
                            false);
      ak_coord_doc_cvt_vec3(st,
                            ak_coord_doc_sampler_accessor(sampler,
                                                          AK_INPUT_OUT_TANGENT),
                            false);
      break;
    case AK_TARGET_SCALE:
      noSign = true;
      ak_coord_doc_cvt_vec3(st,
                            ak_coord_doc_sampler_accessor(sampler,
                                                          AK_INPUT_OUTPUT),
                            noSign);
      ak_coord_doc_cvt_vec3(st,
                            ak_coord_doc_sampler_accessor(sampler,
                                                          AK_INPUT_IN_TANGENT),
                            noSign);
      ak_coord_doc_cvt_vec3(st,
                            ak_coord_doc_sampler_accessor(sampler,
                                                          AK_INPUT_OUT_TANGENT),
                            noSign);
      break;
    case AK_TARGET_QUAT:
      ak_coord_doc_cvt_quat(st,
                            ak_coord_doc_sampler_accessor(sampler,
                                                          AK_INPUT_OUTPUT));
      ak_coord_doc_cvt_quat(st,
                            ak_coord_doc_sampler_accessor(sampler,
                                                          AK_INPUT_IN_TANGENT));
      ak_coord_doc_cvt_quat(st,
                            ak_coord_doc_sampler_accessor(sampler,
                                                          AK_INPUT_OUT_TANGENT));
      break;
    default:
      break;
  }
}

static
void
ak_coord_doc_cvt_animation(AkCoordDocCvt * __restrict st,
                           AkAnimation   * __restrict anim) {
  AkChannel *channel;

  for (; anim; anim = anim->next) {
    for (channel = anim->channel; channel; channel = channel->next)
      ak_coord_doc_cvt_anim_channel(st, channel);

    if (anim->animation)
      ak_coord_doc_cvt_animation(st, anim->animation);
  }
}

static
void
ak_coord_doc_cvt_instancing(AkCoordDocCvt  * __restrict st,
                            AkGpuInstancing * __restrict instancing) {
  if (!instancing)
    return;

  ak_coord_doc_cvt_vec3(st, instancing->translation, false);
  ak_coord_doc_cvt_quat(st, instancing->rotation);
  ak_coord_doc_cvt_vec3(st, instancing->scale, true);
}

static
void
ak_coord_doc_cvt_node_tree(AkCoordDocCvt * __restrict st,
                           AkNode        * __restrict node) {
  for (; node; node = node->next) {
    if (ak_coord_ptrset_seen(&st->nodes, node))
      continue;

    ak_coordCvtNodeTransformsTo(st->doc,
                                node,
                                st->oldCoordSys,
                                st->newCoordSys);

    if (node->matrix)
      ak_coordCvtMatrixTo(st->oldCoordSys,
                          node->matrix->val,
                          st->newCoordSys);
    if (node->matrixWorld)
      ak_coordCvtMatrixTo(st->oldCoordSys,
                          node->matrixWorld->val,
                          st->newCoordSys);
    if (node->bbox)
      node->bbox->isvalid = false;

    ak_coord_doc_cvt_instancing(st, node->gpuInstancing);

    if (node->chld)
      ak_coord_doc_cvt_node_tree(st, node->chld);
  }
}

static
void
ak_coord_doc_cvt_nodes(AkCoordDocCvt * __restrict st) {
  AkScene *scene;
  AkNode  *node;

  for (node = st->doc->lib.nodes.first; node; node = node->docNext)
    ak_coord_doc_cvt_node_tree(st, node);

  for (scene = st->doc->lib.scenes.first; scene; scene = scene->next) {
    if (scene->node)
      ak_coord_doc_cvt_node_tree(st, scene->node);
    if (scene->bbox)
      scene->bbox->isvalid = false;
  }

  if (st->doc->scene && st->doc->scene->node)
    ak_coord_doc_cvt_node_tree(st, st->doc->scene->node);
}

static
void
ak_coord_doc_cvt_morphable(AkCoordDocCvt * __restrict st,
                           AkMorphable   * __restrict morphable) {
  for (; morphable; morphable = morphable->next)
    ak_coord_doc_cvt_input(st, morphable->input);
}

static
void
ak_coord_doc_cvt_morphs(AkCoordDocCvt * __restrict st) {
  AkMorph       *morph;
  AkMorphTarget *target;

  for (morph = st->doc->lib.morphs.first; morph; morph = morph->next) {
    for (target = morph->target; target; target = target->next) {
      AkObject *targetObj;

      targetObj = target->target;
      if (!targetObj)
        continue;

      switch ((AkMorphableType)targetObj->type) {
        case AK_MORPHABLE_GEOMETRY:
          ak_coord_doc_cvt_geom(st, ak_objGetTarget(targetObj));
          break;
        case AK_MORPHABLE_MORPHABLE:
          ak_coord_doc_cvt_morphable(st, ak_objGet(targetObj));
          break;
        default:
          break;
      }
    }
  }
}

static
void
ak_coord_doc_cvt_skins(AkCoordDocCvt * __restrict st) {
  AkSkin *skin;
  size_t  i;

  for (skin = st->doc->lib.skins.first; skin; skin = skin->next) {
    ak_coordCvtMatrixTo(st->oldCoordSys,
                        skin->bindShapeMatrix,
                        st->newCoordSys);
    for (i = 0; i < skin->nJoints; i++) {
      if (!skin->invBindPoses)
        break;
      ak_coordCvtMatrixTo(st->oldCoordSys,
                          skin->invBindPoses[i],
                          st->newCoordSys);
    }
  }
}

AK_EXPORT
void
ak_changeCoordSys(AkDoc * __restrict doc,
                  AkCoordSys * newCoordSys) {
  AkCoordDocCvt st;
  AkGeometry   *geom;

  if (!doc || !newCoordSys)
    return;

  if (!doc->coordSys)
    doc->coordSys = AK_YUP;

  if (doc->coordSys == newCoordSys)
    return;

  memset(&st, 0, sizeof(st));
  st.doc         = doc;
  st.oldCoordSys = doc->coordSys;
  st.newCoordSys = newCoordSys;

  for (geom = doc->lib.geometries.first; geom; geom = geom->next)
    ak_coord_doc_cvt_geom(&st, geom);

  ak_coord_doc_cvt_morphs(&st);
  ak_coord_doc_cvt_nodes(&st);
  ak_coord_doc_cvt_animation(&st, doc->lib.animations.first);
  ak_coord_doc_cvt_skins(&st);

  doc->coordSys = newCoordSys;

  if (doc->inf)
    doc->inf->base.coordSys = newCoordSys;

  ak_coord_ptrset_free(&st.nodes);
  ak_coord_ptrset_free(&st.geoms);
  ak_coord_ptrset_free(&st.accessors);
}
