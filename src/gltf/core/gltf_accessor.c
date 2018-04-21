/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_accessor.h"
#include "gltf_enums.h"
#include "gltf_buffer.h"
#include "../../ak_accessor.h"

AkAccessor* _assetkit_hide
gltf_accessor(AkGLTFState     * __restrict gst,
              void            * __restrict memParent,
              json_t          * __restrict jacc) {
  AkHeap      *heap;
  AkSource    *src;
  AkAccessor  *acc;
  json_t      *jbuffView, *min, *max;
  AkDataParam *dp, *last_dp;
  uint32_t     bound, i;

  heap = gst->heap;

  src  = ak_heap_calloc(heap, memParent, sizeof(*src));
  acc  = ak_heap_calloc(heap, src, sizeof(*acc));

  acc->itemTypeId = gltf_componentType(jsn_i32(jacc, _s_gltf_componentType));
  acc->type       = ak_typeDesc(acc->itemTypeId);
  acc->byteOffset = jsn_i64(jacc, _s_gltf_byteOffset);
  acc->normalized = json_boolean_value(json_object_get(jacc,
                                                       _s_gltf_normalized));
  bound      = gltf_type(json_cstr(jacc, _s_gltf_type));
  acc->count = jsn_i32(jacc, _s_gltf_count) * bound;

  if ((jbuffView = json_object_get(jacc, _s_gltf_bufferView))) {
    AkBuffer *buff;

    acc->byteOffset += jsn_i64(jbuffView, _s_gltf_byteOffset);
    acc->stride      = (uint32_t)(acc->byteStride / acc->type->size);

    buff = gltf_buffer(gst,
                       (int32_t)json_integer_value(jbuffView),
                       &acc->byteStride);

    acc->byteLength = buff->length;
    acc->source.ptr = buff;
  }

  dp          = last_dp = NULL;
  acc->bound  = bound;

  if (bound > 10)
    acc->bound >>= 3;

  acc->stride = acc->bound;
  if (acc->byteStride == 0)
    acc->byteStride = acc->type->size * acc->stride;

  /* prepare accessor params */
  for (i = 0; i < acc->bound; i++) {
    char        paramName[8];
    const char *paramNames = "XYZW";

    memset(paramName, '\0', sizeof(paramName));

    /* vector */
    if (bound < 10) {
      paramName[0] = paramNames[i];
    }

    /* matrix */
    else {
      uint32_t s, m, n;

      s = (bound << (32 - 3)) >> (32 - 3);

      m = i % s;
      n = i / s;

      /* M01, M02, ... M33 */
      sprintf(paramName, "M%d%d", m, n);
    }

    dp         = ak_heap_calloc(heap, acc, sizeof(*dp));
    dp->offset = i;
    dp->name   = ak_heap_strdup(heap,
                                dp,
                                paramName);
    memcpy(&dp->type,
           acc->type,
           sizeof(dp->type));

    if (!last_dp)
      acc->param = dp;
    else
      last_dp->next = dp;
    last_dp = dp;
  }

  src->tcommon = acc;

  min = json_object_get(jacc, _s_gltf_min);
  max = json_object_get(jacc, _s_gltf_max);

  if (min && json_is_array(min)) {
    size_t arrSize;

    arrSize  = json_array_size(min);
    acc->min = ak_heap_alloc(heap,
                             acc,
                             sizeof(acc->type->size) * acc->bound);

    if (arrSize > acc->bound)
      arrSize = acc->bound;

    for (i = 0; i < arrSize; i++) {
      json_t *ival;

      ival = json_array_get(min, i);
      switch (acc->itemTypeId) {
        case AKT_FLOAT:
          ((float *)acc->min)[i] = json_number_value(ival);
          break;
        case AKT_INT:
        case AKT_UINT:
          ((int32_t *)acc->min)[i] = (int32_t)json_integer_value(ival);
          break;
        case AKT_SHORT:
        case AKT_USHORT:
          ((short *)acc->min)[i] = (short)json_integer_value(ival);
          break;
        case AKT_BYTE:
        case AKT_UBYTE:
          ((char *)acc->min)[i] = (char)json_integer_value(ival);
          break;
        default:
          break;
      }
    }
  }

  if (max && json_is_array(max)) {
    size_t arrSize;

    arrSize  = json_array_size(max);
    acc->max = ak_heap_alloc(heap,
                             acc,
                             sizeof(acc->type->size) * acc->bound);

    if (arrSize > acc->bound)
      arrSize = acc->bound;

    for (i = 0; i < arrSize; i++) {
      json_t *ival;

      ival = json_array_get(max, i);
      switch (acc->itemTypeId) {
        case AKT_FLOAT:
          ((float *)acc->max)[i] = json_number_value(ival);
          break;
        case AKT_INT:
        case AKT_UINT:
          ((int32_t *)acc->max)[i] = (int32_t)json_integer_value(ival);
          break;
        case AKT_SHORT:
        case AKT_USHORT:
          ((short *)acc->max)[i] = (short)json_integer_value(ival);
          break;
        case AKT_BYTE:
        case AKT_UBYTE:
          ((char *)acc->max)[i] = (char)json_integer_value(ival);
          break;
        default:
          break;
      }
    }
  }

  return acc;
}
