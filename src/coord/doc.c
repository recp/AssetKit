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
#include "../accessor.h"
#include "common.h"

typedef struct AkCoordPtrSet {
  const void **keys;
  size_t      capacity;
  size_t      count;
  bool        failed;
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
  if (oldCap > (size_t)-1 / 2u)
    goto fail;
  newCap = oldCap ? oldCap << 1u : 64u;
  while (newCap < minCapacity) {
    if (newCap > (size_t)-1 / 2u)
      goto fail;
    newCap <<= 1u;
  }

  if (newCap > (size_t)-1 / sizeof(*keys))
    goto fail;

  keys = ak_calloc(NULL, sizeof(*keys) * newCap);
  if (!keys)
    goto fail;

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

fail:
  set->failed = true;
  return false;
}

static
bool
ak_coord_ptrset_seen(AkCoordPtrSet * __restrict set,
                     const void    * __restrict key) {
  uintptr_t h, mask;

  if (!key)
    return true;

  /* On allocation failure, conservatively skip further conversions.  Treating
     an untracked pointer as new would allow a shared accessor to be mutated
     repeatedly and make the result dependent on traversal order. */
  if (set->failed || set->count == (size_t)-1)
    return true;

  if (!set->keys
      || set->count + 1u >= set->capacity - set->capacity / 4u) {
    if (!ak_coord_ptrset_grow(set, set->count + 1u))
      return true;
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
  set->failed = false;
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
  last = acc->byteOffset + (size_t)(acc->count - 1u) * stride;
  if (last > acc->buffer->length
      || sizeof(float) * 4u > acc->buffer->length - last)
    return false;

  data = (unsigned char *)acc->buffer->data + acc->byteOffset;

  for (i = 0; i < acc->count; i++) {
    float values[4];

    memcpy(values, data + (size_t)i * stride, sizeof(values));
    ak_coordCvtQuatTo(oldCoordSys, values, newCoordSys);
    memcpy(data + (size_t)i * stride, values, sizeof(values));
  }

  return true;
}

static
bool
ak_coord_cvt_accessor_mat4(AkAccessor * __restrict acc,
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
      || acc->componentCount != 16)
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
  if (stride < sizeof(float) * 16u
      || acc->byteOffset > acc->buffer->length)
    return false;

  if ((size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;
  last = acc->byteOffset + (size_t)(acc->count - 1u) * stride;
  if (last > acc->buffer->length
      || sizeof(float) * 16u > acc->buffer->length - last)
    return false;

  data = (unsigned char *)acc->buffer->data + acc->byteOffset;
  for (i = 0; i < acc->count; i++) {
    unsigned char *row;
    mat4          matrix;

    row = data + (size_t)i * stride;
    memcpy(matrix, row, sizeof(matrix));
    ak_coordCvtMatrixTo(oldCoordSys, matrix, newCoordSys);
    memcpy(row, matrix, sizeof(matrix));
  }

  return true;
}

static
bool
ak_coord_cvt_accessor_scalar(AkAccessor * __restrict acc,
                             uint32_t                 component,
                             float                    factor) {
  unsigned char *data;
  size_t         componentBytes, rowBytes, stride, last;
  uint32_t       i;

  if (!acc || acc->count == 0 || factor == 1.0f)
    return true;

  if (acc->componentType != AKT_FLOAT
      || acc->normalized
      || acc->bytesPerComponent != sizeof(float))
    ak_accessorMakeFloat(acc);

  if (acc->componentType != AKT_FLOAT
      || acc->normalized
      || acc->bytesPerComponent != sizeof(float)
      || component >= acc->componentCount
      || !acc->buffer
      || !acc->buffer->data)
    return false;

  if ((size_t)acc->componentCount > (size_t)-1 / sizeof(float))
    return false;

  componentBytes = ((size_t)component + 1u) * sizeof(float);
  rowBytes       = (size_t)acc->componentCount * sizeof(float);
  stride         = acc->byteStride ? acc->byteStride : rowBytes;
  if (stride < componentBytes
      || acc->byteOffset > acc->buffer->length)
    return false;

  if ((size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;
  last = acc->byteOffset + (size_t)(acc->count - 1u) * stride;
  if (last > acc->buffer->length
      || componentBytes > acc->buffer->length - last)
    return false;

  data = (unsigned char *)acc->buffer->data + acc->byteOffset;
  for (i = 0; i < acc->count; i++) {
    unsigned char *row;
    float          value;

    row = data + (size_t)i * stride + (size_t)component * sizeof(float);
    memcpy(&value, row, sizeof(value));
    value *= factor;
    memcpy(row, &value, sizeof(value));
  }

  return true;
}

static
bool
ak_coord_cvt_accessor_vec3_paired(AkAccessor * __restrict acc,
                                  AkCoordSys * __restrict oldCoordSys,
                                  AkCoordSys * __restrict newCoordSys,
                                  bool                    noSign,
                                  uint32_t                targetComponents) {
  AkAxisAccessor a0, a1;
  unsigned char *data;
  size_t         rowBytes, stride, last;
  uint32_t       i, c;

  if (!acc
      || !oldCoordSys
      || !newCoordSys
      || oldCoordSys == newCoordSys
      || acc->count == 0u
      || targetComponents < 3u
      || acc->componentCount != targetComponents * 2u)
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
  if (stride < rowBytes || acc->byteOffset > acc->buffer->length)
    return false;
  if ((size_t)(acc->count - 1u) > ((size_t)-1 - acc->byteOffset) / stride)
    return false;
  last = acc->byteOffset + (size_t)(acc->count - 1u) * stride;
  if (last > acc->buffer->length || rowBytes > acc->buffer->length - last)
    return false;

  data = (unsigned char *)acc->buffer->data + acc->byteOffset;
  ak_coordAxisAccessors(oldCoordSys, newCoordSys, &a0, &a1);
  for (i = 0; i < acc->count; i++) {
    float times[3], values[3], tmp[3];

    for (c = 0; c < 3u; c++) {
      memcpy(&times[c],
             data + (size_t)i * stride + (size_t)(c * 2u) * sizeof(float),
             sizeof(float));
      memcpy(&values[c],
             data + (size_t)i * stride + (size_t)(c * 2u + 1u) * sizeof(float),
             sizeof(float));
    }
    AK_CVT_VEC_NOSIGN(times);
    if (noSign) {
      AK_CVT_VEC_NOSIGN(values);
    } else {
      AK_CVT_VEC(values);
    }
    for (c = 0; c < 3u; c++) {
      memcpy(data + (size_t)i * stride
             + (size_t)(c * 2u) * sizeof(float),
             &times[c],
             sizeof(float));
      memcpy(data + (size_t)i * stride
             + (size_t)(c * 2u + 1u) * sizeof(float),
             &values[c],
             sizeof(float));
    }
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
ak_coord_doc_cvt_vec3_tangent(AkCoordDocCvt * __restrict st,
                              AkAccessor    * __restrict acc,
                              bool                        noSign,
                              uint32_t                    targetComponents) {
  if (!acc)
    return;

  if (acc->componentCount == targetComponents * 2u) {
    if (!ak_coord_ptrset_seen(&st->accessors, acc))
      ak_coord_cvt_accessor_vec3_paired(acc,
                                        st->oldCoordSys,
                                        st->newCoordSys,
                                        noSign,
                                        targetComponents);
    return;
  }

  ak_coord_doc_cvt_vec3(st, acc, noSign);
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
ak_coord_doc_cvt_mat4(AkCoordDocCvt * __restrict st,
                      AkAccessor    * __restrict acc) {
  if (!acc || ak_coord_ptrset_seen(&st->accessors, acc))
    return;

  ak_coord_cvt_accessor_mat4(acc, st->oldCoordSys, st->newCoordSys);
}

static AkAnimSampler*
ak_coord_doc_channel_sampler(AkChannel * __restrict channel);

static
AkAccessor*
ak_coord_doc_clone_accessor(void       * __restrict parent,
                            AkAccessor * __restrict sourceAccessor) {
  AkHeap     *heap;
  AkAccessor *cloneAccessor;
  AkBuffer   *sourceBuffer, *cloneBuffer;

  if (!parent
      || !sourceAccessor
      || !(heap = ak_heap_getheap(parent))
      || !(cloneAccessor = ak_accessor_dup(sourceAccessor)))
    return NULL;

  ak_heap_setpm(cloneAccessor, parent);
  cloneAccessor->next = NULL;
  sourceBuffer = sourceAccessor->buffer;
  cloneBuffer  = NULL;

  if (sourceBuffer) {
    cloneBuffer = ak_heap_calloc(heap, cloneAccessor, sizeof(*cloneBuffer));
    if (!cloneBuffer) {
      ak_free(cloneAccessor);
      return NULL;
    }
    *cloneBuffer      = *sourceBuffer;
    cloneBuffer->next = NULL;
    cloneBuffer->data = NULL;

    if (sourceBuffer->data && sourceBuffer->length > 0u) {
      cloneBuffer->data = ak_heap_alloc(heap,
                                        cloneBuffer,
                                        sourceBuffer->length);
      if (!cloneBuffer->data) {
        ak_free(cloneAccessor);
        return NULL;
      }
      memcpy(cloneBuffer->data,
             sourceBuffer->data,
             sourceBuffer->length);
    }
  }
  cloneAccessor->buffer = cloneBuffer;
  return cloneAccessor;
}

static
AkAnimSampler*
ak_coord_doc_clone_sampler(AkAnimation   * __restrict anim,
                           AkChannel     * __restrict channel,
                           AkAnimSampler * __restrict sampler) {
  AkHeap        *heap;
  AkAnimSampler *clone;
  AkInput       *sourceInput, *cloneInput, *lastInput;

  if (!anim || !channel || !sampler
      || !(heap = ak_heap_getheap(channel)))
    return NULL;

  clone = ak_heap_calloc(heap, anim, sizeof(*clone));
  if (!clone)
    return NULL;

  *clone                  = *sampler;
  clone->input            = NULL;
  clone->inputInput       = NULL;
  clone->outputInput      = NULL;
  clone->interpInput      = NULL;
  clone->inTangentInput   = NULL;
  clone->outTangentInput  = NULL;
  clone->base.next        = NULL;
  lastInput               = NULL;

  for (sourceInput = sampler->input;
       sourceInput;
       sourceInput = sourceInput->next) {
    AkAccessor *sourceAccessor, *cloneAccessor;

    cloneInput = ak_heap_calloc(heap, clone, sizeof(*cloneInput));
    if (!cloneInput) {
      ak_free(clone);
      return NULL;
    }
    *cloneInput       = *sourceInput;
    cloneInput->next  = NULL;
    sourceAccessor    = sourceInput->accessor;
    cloneAccessor     = NULL;

    if (sourceAccessor
        && !(cloneAccessor = ak_coord_doc_clone_accessor(cloneInput,
                                                         sourceAccessor))) {
      ak_free(clone);
      return NULL;
    }
    cloneInput->accessor = cloneAccessor;

    if (lastInput)
      lastInput->next = cloneInput;
    else
      clone->input = cloneInput;
    lastInput = cloneInput;

    if (sourceInput == sampler->inputInput)
      clone->inputInput = cloneInput;
    if (sourceInput == sampler->outputInput)
      clone->outputInput = cloneInput;
    if (sourceInput == sampler->interpInput)
      clone->interpInput = cloneInput;
    if (sourceInput == sampler->inTangentInput)
      clone->inTangentInput = cloneInput;
    if (sourceInput == sampler->outTangentInput)
      clone->outTangentInput = cloneInput;
  }

  clone->base.next = (AkOneWayIterBase *)anim->sampler;
  anim->sampler    = clone;
  channel->source.ptr = clone;
  return clone;
}

static
void
ak_coord_doc_sampler_channel_count_walk(AkAnimation   * __restrict anim,
                                        AkAnimSampler * __restrict sampler,
                                        uint32_t      * __restrict count) {
  for (; anim && *count < 2u; anim = anim->next) {
    AkChannel *channel;

    for (channel = anim->channel; channel; channel = channel->next) {
      if (ak_coord_doc_channel_sampler(channel) == sampler && ++*count >= 2u)
        return;
    }
    if (anim->animation)
      ak_coord_doc_sampler_channel_count_walk(anim->animation, sampler, count);
  }
}

static
bool
ak_coord_doc_sampler_is_shared(AkAnimation   * __restrict animations,
                               AkAnimSampler * __restrict sampler) {
  uint32_t count;

  count = 0u;
  ak_coord_doc_sampler_channel_count_walk(animations, sampler, &count);
  return count > 1u;
}

static
void
ak_coord_doc_accessor_ref_count_walk(AkAnimation * __restrict anim,
                                     AkAccessor  * __restrict accessor,
                                     AkBuffer    * __restrict buffer,
                                     uint32_t    * __restrict accessorRefs,
                                     uint32_t    * __restrict bufferRefs) {
  for (; anim; anim = anim->next) {
    AkAnimSampler *sampler;

    for (sampler = anim->sampler;
         sampler;
         sampler = (AkAnimSampler *)sampler->base.next) {
      AkInput *input;

      for (input = sampler->input; input; input = input->next) {
        if (input->accessor == accessor)
          (*accessorRefs)++;
        if (input->accessor && input->accessor->buffer == buffer)
          (*bufferRefs)++;
      }
    }

    if (anim->animation)
      ak_coord_doc_accessor_ref_count_walk(anim->animation,
                                           accessor,
                                           buffer,
                                           accessorRefs,
                                           bufferRefs);
  }
}

static
AkAccessor*
ak_coord_doc_unique_accessor(AkCoordDocCvt  * __restrict st,
                             AkAnimSampler  * __restrict sampler,
                             AkInputSemantic             semantic) {
  AkInput    *input;
  AkAccessor *accessor;
  uint32_t    accessorRefs, bufferRefs;

  if (!st || !sampler)
    return NULL;

  input = NULL;
  switch (semantic) {
    case AK_INPUT_OUTPUT:      input = sampler->outputInput; break;
    case AK_INPUT_IN_TANGENT:  input = sampler->inTangentInput; break;
    case AK_INPUT_OUT_TANGENT: input = sampler->outTangentInput; break;
    default: break;
  }
  if (!input) {
    for (input = sampler->input; input; input = input->next) {
      if (input->semantic == semantic)
        break;
    }
  }
  if (!input || !(accessor = input->accessor))
    return NULL;

  accessorRefs = 0u;
  bufferRefs   = 0u;
  ak_coord_doc_accessor_ref_count_walk(st->doc->lib.animations.first,
                                       accessor,
                                       accessor->buffer,
                                       &accessorRefs,
                                       &bufferRefs);
  if (accessorRefs > 1u || bufferRefs > 1u) {
    AkAccessor *clone;

    clone = ak_coord_doc_clone_accessor(input, accessor);
    if (!clone)
      return NULL;
    input->accessor = clone;
    accessor = clone;
  }

  return accessor;
}

static
void
ak_coord_doc_cvt_scalar(AkAccessor * __restrict acc,
                        uint32_t                component,
                        float                   factor) {
  if (!acc || factor == 1.0f)
    return;

  ak_coord_cvt_accessor_scalar(acc, component, factor);
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
AkAnimSampler*
ak_coord_doc_cvt_partial_transform(AkCoordDocCvt * __restrict st,
                                   AkChannel     * __restrict channel,
                                   AkAnimSampler * __restrict sampler,
                                   AkObject      * __restrict target) {
  AkResolvedTarget *resolved;
  AkTypeId          type;
  float             sourceAxis[3], targetAxis[3], factor;
  uint32_t          sourceOffset, targetOffset, i;

  resolved = channel->resolvedTarget;
  if (!resolved || !resolved->isPartial || !target)
    return sampler;

  type         = (AkTypeId)target->type;
  sourceOffset = resolved->off;
  targetOffset = UINT32_MAX;
  factor       = 1.0f;

  if (type == AKT_MATRIX && sourceOffset < 16u) {
    mat4 basis;

    /* Supported coordinate systems differ by a signed permutation basis.
       Conjugating one matrix element therefore still produces exactly one
       matrix element, possibly with its sign inverted. */
    glm_mat4_zero(basis);
    basis[sourceOffset / 4u][sourceOffset % 4u] = 1.0f;
    ak_coordCvtMatrixTo(st->oldCoordSys, basis, st->newCoordSys);
    for (i = 0; i < 16u; i++) {
      float value;

      value = basis[i / 4u][i % 4u];
      if (value != 0.0f) {
        targetOffset = i;
        factor       = value < 0.0f ? -1.0f : 1.0f;
        break;
      }
    }
  } else if ((type == AKT_TRANSLATE
              || type == AKT_SCALE
              || type == AKT_ROTATE)
             && sourceOffset < 3u) {
    memset(sourceAxis, 0, sizeof(sourceAxis));
    sourceAxis[sourceOffset] = 1.0f;
    ak_coordCvtVectorTo(st->oldCoordSys,
                        sourceAxis,
                        st->newCoordSys,
                        targetAxis);

    for (i = 0; i < 3u; i++) {
      if (targetAxis[i] != 0.0f) {
        targetOffset = i;
        factor       = targetAxis[i] < 0.0f ? -1.0f : 1.0f;
        break;
      }
    }
    if (type == AKT_ROTATE
        && (st->oldCoordSys->rotDirection + 1)
             * (st->newCoordSys->rotDirection + 1) < 0)
      factor = -factor;
  } else {
    return sampler;
  }
  if (targetOffset == UINT32_MAX)
    return sampler;

  resolved->off = targetOffset;
  if (type == AKT_SCALE || factor == 1.0f)
    return sampler;

  /* Partial OUTPUT accessors are scalar.  COLLADA Bezier tangents are
     commonly (time,value); preserve time and transform only the value. */
  ak_coord_doc_cvt_scalar(ak_coord_doc_unique_accessor(st,
                                                       sampler,
                                                       AK_INPUT_OUTPUT),
                          0u,
                          factor);
  {
    AkAccessor *tangent;

    tangent = ak_coord_doc_unique_accessor(st, sampler, AK_INPUT_IN_TANGENT);
    if (tangent && tangent->componentCount <= 2u)
      ak_coord_doc_cvt_scalar(tangent,
                              tangent->componentCount == 2u ? 1u : 0u,
                              factor);

    tangent = ak_coord_doc_unique_accessor(st, sampler, AK_INPUT_OUT_TANGENT);
    if (tangent && tangent->componentCount <= 2u)
      ak_coord_doc_cvt_scalar(tangent,
                              tangent->componentCount == 2u ? 1u : 0u,
                              factor);
  }

  return sampler;
}

static
void
ak_coord_doc_cvt_anim_channel(AkCoordDocCvt * __restrict st,
                              AkAnimation   * __restrict anim,
                              AkChannel     * __restrict channel) {
  AkAnimSampler *sampler;
  AkObject      *target;
  bool           noSign;

  sampler = ak_coord_doc_channel_sampler(channel);
  if (!sampler)
    return;

  /* One COLLADA sampler may legally feed channels with different transform
     semantics.  Give each channel stable ownership before applying a
     coordinate-specific operation; every clone remains referenced and a
     second conversion therefore does not grow the animation list. */
  if (ak_coord_doc_sampler_is_shared(st->doc->lib.animations.first, sampler)) {
    AkAnimSampler *clone;

    clone = ak_coord_doc_clone_sampler(anim, channel, sampler);
    if (!clone)
      return;
    sampler = clone;
  }

  /* A complete transform-matrix channel is represented as AK_TARGET_FLOAT
     for historical API compatibility.  Detect the resolved matrix object so
     its 16-float OUTPUT rows still receive a basis change. */
  target = channel->resolvedTarget
             ? channel->resolvedTarget->target
             : NULL;
  if (target && ak_typeid(target) == AKT_OBJECT)
    sampler = ak_coord_doc_cvt_partial_transform(st,
                                                 channel,
                                                 sampler,
                                                 target);

  if (target
      && ak_typeid(target) == AKT_OBJECT
      && target->type == AKT_MATRIX
      && !channel->resolvedTarget->isPartial) {
    ak_coord_doc_cvt_mat4(st,
                          ak_coord_doc_unique_accessor(st,
                                                       sampler,
                                                       AK_INPUT_OUTPUT));
    ak_coord_doc_cvt_mat4(st,
                          ak_coord_doc_unique_accessor(st,
                                                       sampler,
                                                       AK_INPUT_IN_TANGENT));
    ak_coord_doc_cvt_mat4(st,
                          ak_coord_doc_unique_accessor(st,
                                                       sampler,
                                                       AK_INPUT_OUT_TANGENT));
    return;
  }

  switch (channel->targetType) {
    case AK_TARGET_POSITION:
      ak_coord_doc_cvt_vec3(st,
                            ak_coord_doc_unique_accessor(st,
                                                         sampler,
                                                         AK_INPUT_OUTPUT),
                            false);
      ak_coord_doc_cvt_vec3_tangent(
        st,
        ak_coord_doc_unique_accessor(st, sampler, AK_INPUT_IN_TANGENT),
        false,
        3u);
      ak_coord_doc_cvt_vec3_tangent(
        st,
        ak_coord_doc_unique_accessor(st, sampler, AK_INPUT_OUT_TANGENT),
        false,
        3u);
      break;
    case AK_TARGET_ROTATE: {
      bool flipHandedness;

      flipHandedness = (st->oldCoordSys->rotDirection + 1)
                       * (st->newCoordSys->rotDirection + 1) < 0;
      ak_coord_doc_cvt_vec3(st,
                            ak_coord_doc_unique_accessor(st,
                                                         sampler,
                                                         AK_INPUT_OUTPUT),
                            false);
      ak_coord_doc_cvt_vec3_tangent(
        st,
        ak_coord_doc_unique_accessor(st, sampler, AK_INPUT_IN_TANGENT),
        false,
        4u);
      ak_coord_doc_cvt_vec3_tangent(
        st,
        ak_coord_doc_unique_accessor(st, sampler, AK_INPUT_OUT_TANGENT),
        false,
        4u);
      if (flipHandedness) {
        AkInputSemantic semantics[3];
        uint32_t        i;

        semantics[0] = AK_INPUT_OUTPUT;
        semantics[1] = AK_INPUT_IN_TANGENT;
        semantics[2] = AK_INPUT_OUT_TANGENT;
        for (i = 0; i < 3u; i++) {
          AkAccessor *acc;
          uint32_t    component;

          acc = ak_coord_doc_unique_accessor(st, sampler, semantics[i]);
          for (component = 0; acc && component < 3u; component++) {
            uint32_t storageComponent;

            storageComponent = acc->componentCount == 8u
                               ? component * 2u + 1u
                               : component;
            ak_coord_doc_cvt_scalar(acc, storageComponent, -1.0f);
          }
        }
      }
      break;
    }
    case AK_TARGET_SCALE:
      noSign = true;
      ak_coord_doc_cvt_vec3(st,
                            ak_coord_doc_unique_accessor(st,
                                                         sampler,
                                                         AK_INPUT_OUTPUT),
                            noSign);
      ak_coord_doc_cvt_vec3_tangent(
        st,
        ak_coord_doc_unique_accessor(st, sampler, AK_INPUT_IN_TANGENT),
        noSign,
        3u);
      ak_coord_doc_cvt_vec3_tangent(
        st,
        ak_coord_doc_unique_accessor(st, sampler, AK_INPUT_OUT_TANGENT),
        noSign,
        3u);
      break;
    case AK_TARGET_QUAT:
      ak_coord_doc_cvt_quat(st,
                            ak_coord_doc_unique_accessor(st,
                                                         sampler,
                                                         AK_INPUT_OUTPUT));
      ak_coord_doc_cvt_quat(st,
                            ak_coord_doc_unique_accessor(st,
                                                         sampler,
                                                         AK_INPUT_IN_TANGENT));
      ak_coord_doc_cvt_quat(st,
                            ak_coord_doc_unique_accessor(st,
                                                         sampler,
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
      ak_coord_doc_cvt_anim_channel(st, anim, channel);

    if (anim->animation)
      ak_coord_doc_cvt_animation(st, anim->animation);
  }
}

AK_HIDE
void
ak_coordCvtAnimationsTo(AkDoc      * __restrict doc,
                        AkCoordSys * __restrict oldCoordSys,
                        AkCoordSys * __restrict newCoordSys) {
  AkCoordDocCvt st;

  if (!doc || !oldCoordSys || !newCoordSys || oldCoordSys == newCoordSys)
    return;

  memset(&st, 0, sizeof(st));
  st.doc         = doc;
  st.oldCoordSys = oldCoordSys;
  st.newCoordSys = newCoordSys;

  ak_coord_doc_cvt_animation(&st, doc->lib.animations.first);
  ak_coord_ptrset_free(&st.accessors);
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

AK_HIDE
void
ak_coordCvtSkinsTo(AkDoc      * __restrict doc,
                   AkCoordSys * __restrict oldCoordSys,
                   AkCoordSys * __restrict newCoordSys) {
  AkSkin *skin;
  size_t  i;

  if (!doc || !oldCoordSys || !newCoordSys || oldCoordSys == newCoordSys)
    return;

  for (skin = doc->lib.skins.first; skin; skin = skin->next) {
    ak_coordCvtMatrixTo(oldCoordSys,
                        skin->bindShapeMatrix,
                        newCoordSys);
    for (i = 0; i < skin->nJoints; i++) {
      if (!skin->invBindPoses)
        break;
      ak_coordCvtMatrixTo(oldCoordSys,
                          skin->invBindPoses[i],
                          newCoordSys);
    }
  }
}

static
void
ak_coord_doc_cvt_skins(AkCoordDocCvt * __restrict st) {
  ak_coordCvtSkinsTo(st->doc, st->oldCoordSys, st->newCoordSys);
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
