/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_mesh_fixup.h"
#include "../ak_memory_common.h"

_assetkit_hide
AkResult
ak_dae_meshFixSrcIntr(AkHeap          *heap,
                      AkMeshPrimitive *primitive,
                      AkInput         *vertInput,
                      size_t           indicesCount,
                      size_t           indexCount) {
  AkSourceFloatArray *positions, *intrArray;
  AkURL        *positionsURL;
  AkSource     *source, *positionsSrc;
  AkInputBasic *inputb;
  AkInput      *input;
  AkUIntArray  *indices;
  AkAccessor   *acc;
  AkObject     *intrData;
  AkFloat      *intrItems;
  AkDataParam  *dparam;
  size_t        newcount;
  uint32_t      stride, vertOffset, offset;

  stride       = 0;
  offset       = 0;
  newcount     = 0;
  positionsSrc = NULL;
  positionsURL = NULL;
  positions    = NULL;
  dparam       = NULL;
  vertOffset   = vertInput->offset;
  inputb       = primitive->vertices->input;
  indices      = primitive->indices;

  while (inputb) {
    AkObject *obj;

    source = ak_getObjectByUrl(&inputb->source);
    acc    = source->techniqueCommon;
    obj    = ak_getObjectByUrl(&acc->source);

    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION) {
      positions    = ak_objGet(obj);
      positionsSrc = source;
      positionsURL = &acc->source;
      newcount     = acc->count;
    }

    /* currently only floats */
    if (obj->type == AK_SOURCE_ARRAY_TYPE_FLOAT)
      stride += acc->bound;

    inputb = inputb->next;
  }

  if (!positionsSrc || !positions || !positionsURL)
    return AK_ERR;

  inputb = &primitive->input->base;

  while (inputb) {
    if (inputb->semantic != AK_INPUT_SEMANTIC_VERTEX) {
      source  = ak_getObjectByUrl(&inputb->source);
      acc     = source->techniqueCommon;
      stride += acc->bound;
    }

    inputb = inputb->next;
  }

  intrData = ak_objAlloc(heap,
                         positionsSrc,
                         sizeof(*intrArray)
                            + newcount * stride * sizeof(float),
                         AK_SOURCE_ARRAY_TYPE_FLOAT,
                         true);
  intrArray            = ak_objGet(intrData);
  intrArray->count     = newcount * stride;
  intrArray->digits    = positions->digits;
  intrArray->magnitude = positions->magnitude;
  intrItems            = intrArray->items;

  /* fix indices for vertices->inputs */
  inputb = primitive->vertices->input;
  while (inputb) {
    AkObject *obj;

    source = ak_getObjectByUrl(&inputb->source);

    if (!source)
      return AK_ERR;

    acc = source->techniqueCommon;
    obj = ak_getObjectByUrl(&acc->source);

    /* currently only floats */
    if (obj->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
      AkSourceFloatArray *oldArray;
      size_t i, j;

      oldArray = ak_objGet(obj);

      if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION) {
        /* move items to interleaved array */
        for (i = 0; i < acc->count; i++) {
          j      = 0;
          dparam = acc->param;

          /* TODO: must unbound params be zero? */
          while (dparam) {
            if (!dparam->name)
              continue;

            intrItems[i * stride + j++]
               = oldArray->items[i * acc->stride + dparam->offset];

            dparam = dparam->next;
          }
        }
      } else {
        /* TODO: */
      }

      ak_url_init(acc,
                  (char *)positionsURL->url,
                  &acc->source);
      ak_url_unref(&acc->source);

      acc->stride = stride;
      acc->offset = offset;
      acc->count  = newcount;

      /* offset for next input */
      offset += acc->bound;
    }

    inputb = inputb->next;
  }

  /* fix indices for other inputs */
  input = primitive->input;
  while (input) {
    AkSource *source;

    if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
      input = (AkInput *)input->base.next;
      continue;
    }

    source = ak_getObjectByUrl(&input->base.source);
    if (!source)
      return AK_ERR;

    /* currently only floats */
    if (source->data->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
      AkSourceFloatArray *oldArray;
      AkObject *obj;
      size_t i, j;

      acc      = source->techniqueCommon;
      obj      = ak_getObjectByUrl(&acc->source);
      oldArray = ak_objGet(obj);

      /* move items to interleaved array */
      for (i = 0; i < indicesCount; i++) {
        AkUInt index, index2;

        index  = indices->items[i * indexCount + vertOffset];
        index2 = indices->items[i * indexCount + input->offset];

        j      = 0;
        dparam = acc->param;

        /* TODO: must unbound params be zero? */
        while (dparam) {
          if (!dparam->name)
            continue;

          intrItems[index * stride + offset + j++]
             = oldArray->items[index2 * acc->stride + dparam->offset];

          dparam = dparam->next;
        }
      }

      /* add unbound params */
      for (i = 0; i < offset; i++) {
        AkDataParam *dparam_u;

        dparam_u = ak_heap_calloc(heap,
                                  acc,
                                  sizeof(AkDataParam));
        dparam_u->next = acc->param;
        acc->param     = dparam_u;
      }

      if (obj != positionsSrc->data)
        ak_free(obj);

      source->data = NULL;

      ak_url_init(acc,
                  (char *)positionsURL->url,
                  &acc->source);
      ak_url_unref(&acc->source);

      acc->stride = stride;
      acc->offset = offset;
      acc->count  = newcount;

      /* offset for next input */
      offset += acc->bound;
    }

    input = (AkInput *)input->base.next;
  }

  ak_moveId(positionsSrc->data, intrData);
  ak_free(positionsSrc->data);
  positionsSrc->data = intrData;

  return AK_OK;
}

_assetkit_hide
AkResult
ak_dae_meshFixSrc(AkHeap          *heap,
                  AkMeshPrimitive *primitive,
                  AkInput         *vertInput,
                  size_t           indicesCount,
                  size_t           indexCount) {
  AkSource     *source, *positionsSrc;
  AkInputBasic *inputb;
  AkInput      *input;
  AkAccessor   *acc;
  AkUIntArray  *indices;
  AkDataParam  *dparam;
  uint32_t      vertOffset;

  inputb       = primitive->vertices->input;
  indices      = primitive->indices;
  vertOffset   = vertInput->offset;
  positionsSrc = NULL;

  while (inputb) {
    source = ak_getObjectByUrl(&inputb->source);

    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION)
      positionsSrc = source;

    inputb = inputb->next;
  }

  /* fix indices for other inputs */
  input = primitive->input;
  while (input) {
    AkSource *source;

    if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
      input = (AkInput *)input->base.next;
      continue;
    }

    source = ak_getObjectByUrl(&input->base.source);
    if (!source)
      return AK_ERR;

    /* currently only floats */
    if (source->data->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
      AkSourceFloatArray *oldArray, *newArray;
      AkObject *obj, *newData;
      size_t i, j;

      acc      = source->techniqueCommon;
      obj      = ak_getObjectByUrl(&acc->source);
      oldArray = ak_objGet(obj);

      if (oldArray->count < indicesCount * acc->stride) {
        newData = ak_objAlloc(heap,
                              source,
                              sizeof(*newArray)
                              + indicesCount * acc->stride * sizeof(float),
                              AK_SOURCE_ARRAY_TYPE_FLOAT,
                              true);
        newArray            = ak_objGet(newData);
        newArray->count     = indicesCount * acc->stride;
        newArray->digits    = oldArray->digits;
        newArray->magnitude = oldArray->magnitude;

        if (oldArray->name) {
          newArray->name = oldArray->name;
          ak_heap_setpm(heap, (void *)oldArray->name, newArray);
        }
      } else {
        newData  = obj;
        newArray = ak_objGet(newData);
      }

      /* move items to interleaved array */
      for (i = 0; i < indicesCount; i++) {
        AkUInt index, index2;

        index  = indices->items[i * indexCount + vertOffset];
        index2 = indices->items[i * indexCount + input->offset];

        j      = 0;
        dparam = acc->param;

        /* TODO: must unbound params be zero? */
        while (dparam) {
          if (!dparam->name)
            continue;

          newArray->items[index * acc->stride
                          + acc->offset
                          + j++]
               = oldArray->items[index2 * acc->stride
                                 + acc->offset
                                 + dparam->offset];

          dparam = dparam->next;
        }
      }

      ak_moveId(obj, newData);
      source->data = newData;

      if (oldArray->count < indicesCount * acc->stride
          && obj != positionsSrc->data)
        ak_free(obj);
    }

    input = (AkInput *)input->base.next;
  }

  return 0;
}

AK_INLINE
_assetkit_hide
AkResult
ak_dae_meshFixupPrimitive(AkHeap *heap,
                          AkMesh *mesh,
                          AkMeshPrimitive *primitive) {
  AkUIntArray *indices;
  AkUIntArray *newIndices;
  AkInput     *vertInput;
  AkInput     *input;
  uint32_t     vertOffset;
  size_t       indicesCount;
  size_t       indexCount;
  size_t       i;

  if (!(indices = primitive->indices))
    return AK_OK;

  indexCount = 0;
  vertInput  = NULL;
  input      = primitive->input;

  /* index count */
  while (input) {
    if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX)
      vertInput = input;

    if (input->offset > indexCount)
      indexCount = input->offset;

    input = (AkInput *)input->base.next;
  }

  indexCount++;

  if (!vertInput)
    return AK_ERR;

  vertOffset   = vertInput->offset;
  indicesCount = indices->count / indexCount;

  /* currently automatically interlaving allowed only 1 primitve */
  if (ak_opt_get(AK_OPT_INDICES_SINGLE_INTERLEAVED)
      && mesh->primitiveCount == 1) {
    ak_dae_meshFixSrcIntr(heap,
                          primitive,
                          vertInput,
                          indicesCount,
                          indexCount);
  } else {
    ak_dae_meshFixSrc(heap,
                      primitive,
                      vertInput,
                      indicesCount,
                      indexCount);
  }

  newIndices = ak_heap_alloc(heap,
                             primitive,
                             sizeof(*newIndices)
                               + sizeof(AkUInt) * indicesCount);

  newIndices->count = indicesCount;

  /* keep only vertex indices in primitives */
  for (i = 0; i < indicesCount; i++)
    newIndices->items[i] = indices->items[i * primitive->inputCount
                                          + vertOffset];

  ak_free(indices);

  primitive->indices = newIndices;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_mesh_fixup(AkMesh * mesh) {
  AkHeap *heap;
  AkDoc  *doc;
  AkMeshPrimitive *primitive;

  heap      = ak_heap_getheap(mesh->vertices);
  doc       = ak_heap_data(heap);
  primitive = mesh->primitive;

  while (primitive) {
    ak_dae_meshFixupPrimitive(heap, mesh, primitive);
    primitive = primitive->next;
  }

  /* fixup coord system */
  if ((void *)ak_opt_get(AK_OPT_COORD) != doc->coordSys)
    ak_changeCoordSysMesh(mesh, (void *)ak_opt_get(AK_OPT_COORD));

  return AK_OK;
}
