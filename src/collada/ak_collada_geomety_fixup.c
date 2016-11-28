/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_geomety_fixup.h"
#include "../ak_memory_common.h"

_assetkit_hide
AkResult
ak_dae_meshFixupInterleave(AkHeap          *heap,
                           AkMeshPrimitive *primitive,
                           AkInput         *vertInput,
                           size_t           indicesCount,
                           size_t           indexCount) {
  AkURL           *positionsURL;
  AkSource        *source, *positionsSource;
  AkInputBasic    *inputb;
  AkInput         *input;
  AkUIntArray     *indices;
  AkFloatArrayN   *positions, *intrArray;
  AkAccessor      *acc;
  AkObject        *intrData;
  AkFloat         *intrItems;
  AkDataParam     *dparam;
  size_t           newcount;
  uint32_t         stride;
  uint32_t         vertOffset, offset;

  stride          = 0;
  offset          = 0;
  newcount        = 0;
  positionsSource = NULL;
  positionsURL    = NULL;
  positions       = NULL;
  dparam          = NULL;
  vertOffset      = vertInput->offset;
  inputb          = primitive->vertices->input;
  indices         = primitive->indices;

  while (inputb) {
    source = ak_getObjectByUrl(&inputb->source);

    if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION) {
      positions       = ak_objGet(source->data);
      positionsSource = source;
      acc             = positionsSource->techniqueCommon;
      positionsURL    = &acc->source;
      newcount        = acc->count;
    }

    /* currently only floats */
    if (source->data->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
      acc = source->techniqueCommon;
      stride += acc->bound;
    }

    inputb = inputb->next;
  }

  if (!positionsSource || !positions || !positionsURL)
    return AK_ERR;

  inputb = &primitive->input->base;

  while (inputb) {
    if (inputb->semantic != AK_INPUT_SEMANTIC_VERTEX) {
      source    = ak_getObjectByUrl(&inputb->source);
      acc       = source->techniqueCommon;
      stride   += acc->bound;
    }

    inputb = inputb->next;
  }

  newcount *= stride;
  intrData = ak_objAlloc(heap,
                         positionsSource,
                         sizeof(*intrArray)
                         + newcount * sizeof(float),
                         AK_SOURCE_ARRAY_TYPE_FLOAT,
                         true);
  intrArray            = ak_objGet(intrData);
  intrArray->count     = newcount;
  intrArray->digits    = positions->digits;
  intrArray->magnitude = positions->magnitude;
  intrItems            = intrArray->items;

  /* fix indices for vertices->inputs */
  inputb = primitive->vertices->input;
  while (inputb) {
    source = ak_getObjectByUrl(&inputb->source);

    if (!source)
      return AK_ERR;

    /* currently only floats */
    if (source->data->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
      AkFloatArrayN *oldArray;
      AkObject      *obj;
      size_t         i, j;

      acc      = source->techniqueCommon;
      obj      = ak_getObjectByUrl(&acc->source);
      oldArray = ak_objGet(obj);

      if (inputb->semantic == AK_INPUT_SEMANTIC_POSITION) {
        /* move items to interleaved array */
        for (i = 0; i < positionsSource->techniqueCommon->count; i++) {
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

      /* offset for next input */
      offset += acc->bound;

      ak_url_init(acc,
                  (char *)positionsURL->url,
                  &acc->source);
      ak_url_unref(&acc->source);
      acc->stride = stride;
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
      AkFloatArrayN *oldArray;
      size_t         i, j;

      acc      = source->techniqueCommon;
      oldArray = ak_objGet(source->data);

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

      ak_free(source->data);
      source->data = NULL;

      ak_url_init(acc,
                  (char *)positionsURL->url,
                  &acc->source);
      ak_url_unref(&acc->source);
      acc->stride = stride;
      acc->offset = offset;

      /* offset for next input */
      offset += acc->bound;
    }

    input = (AkInput *)input->base.next;
  }

  ak_moveId(positionsSource->data, intrData);
  ak_free(positionsSource->data);
  positionsSource->data = intrData;

  return AK_OK;
}

AK_INLINE
_assetkit_hide
AkResult
ak_dae_meshFixupPrimitive(AkHeap          *heap,
                          AkMesh          *mesh,
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
    ak_dae_meshFixupInterleave(heap,
                               primitive,
                               vertInput,
                               indicesCount,
                               indexCount);
  } else {
    /* fix indices for other inputs */
    input = primitive->input;
    while (input) {
      AkSource *source;

      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX) {
        input = (AkInput *)input->base.next;
        continue;
      }

      source = ak_getObjectByUrl(&input->base.source);
      if (!source) {
        input = (AkInput *)input->base.next;
        continue;
      }

      /* TODO: INT, DOUBLE.. */
      if (source->data->type == AK_SOURCE_ARRAY_TYPE_FLOAT) {
        AkObject      *obj;
        AkFloatArrayN *oldArray;
        AkFloatArrayN *newArray;
        AkFloat       *oldItems;
        AkFloat       *newItems;
        size_t         j;

        /* TODO: replace 3 with vertex count */
#define vertc 3
        obj = ak_objAlloc(heap,
                          source,
                          sizeof(*newArray)
                          + sizeof(AkFloat) * indicesCount * vertc,
                          AK_SOURCE_ARRAY_TYPE_FLOAT,
                          true);

        newArray = ak_objGet(obj);
        oldArray = ak_objGet(source->data);

        oldItems = oldArray->items;
        newItems = newArray->items;

        ak_moveId(source->data, obj);

        newArray->count     = indicesCount * vertc;
        newArray->digits    = oldArray->digits;
        newArray->magnitude = oldArray->magnitude;

        if (oldArray->name) {
          newArray->name = oldArray->name;
          ak_heap_setpm(heap, (void *)oldArray->name, newArray);
        }

        /* fix indices */
        for (i = 0; i < indicesCount; i++) {
          AkUInt index;
          AkUInt index2;

          index  = indices->items[i * primitive->inputCount + vertOffset];
          index2 = indices->items[i * primitive->inputCount + input->offset];

          for (j = 0; j < vertc; j++)
            newItems[index * vertc + j] = oldItems[index2 * vertc + j];
        }
        
        ak_free(source->data);
        source->data = obj;
      }
      
      input = (AkInput *)input->base.next;
    }
  }

  newIndices = ak_heap_alloc(heap,
                             primitive,
                             sizeof(*newIndices)
                             + sizeof(AkUInt) * indicesCount);

  newIndices->count = indicesCount;

  /* keep only vertex indices in primitives */
  for (i = 0; i < indicesCount; i++)
    newIndices->items[i] = indices->items[i * primitive->inputCount + vertOffset];

  ak_free(indices);

  primitive->indices = newIndices;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_meshFixup(AkMesh * mesh) {
  AkHeap *heap;
  AkMeshPrimitive *primitive;

  heap      = ak_heap_getheap(mesh->vertices);
  primitive = mesh->primitive;

  while (primitive) {
    ak_dae_meshFixupPrimitive(heap, mesh, primitive);
    primitive = primitive->next;
  }
  
  /* fixup coord system */
  ak_changeCoordSysMesh(mesh, (void *)ak_opt_get(AK_OPT_COORD));
  
  return AK_OK;
}
