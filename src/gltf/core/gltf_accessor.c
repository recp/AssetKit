/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_accessor.h"
#include "gltf_enums.h"
#include "gltf_buffer.h"
#include "../../accessor.h"

#define k_s_gltf_bufferView    1
#define k_s_gltf_byteOffset    2
#define k_s_gltf_componentType 3
#define k_s_gltf_normalized    4
#define k_s_gltf_count         5
#define k_s_gltf_type          6
#define k_s_gltf_max           7
#define k_s_gltf_min           8
#define k_s_gltf_sparse        9
#define k_s_gltf_name          10

static ak_enumpair accMap[] = {
  {_s_gltf_bufferView,    k_s_gltf_bufferView},
  {_s_gltf_byteOffset,    k_s_gltf_byteOffset},
  {_s_gltf_componentType, k_s_gltf_componentType},
  {_s_gltf_normalized,    k_s_gltf_normalized},
  {_s_gltf_count,         k_s_gltf_count},
  {_s_gltf_type,          k_s_gltf_type},
  {_s_gltf_max,           k_s_gltf_max},
  {_s_gltf_min,           k_s_gltf_min},
  {_s_gltf_sparse,        k_s_gltf_sparse},
  {_s_gltf_name,          k_s_gltf_name}
};

static size_t accMapLen = 0;

AkResult _assetkit_hide
gltf_accessors(AkGLTFState  * __restrict gst,
               const json_t * __restrict json) {
  AkHeap             *heap;
  const json_array_t *jaccessors, *jarr;
  const json_t       *jattr, *jmin, *jmax, *jitem;
  AkAccessor         *acc;
  FListItem          *buffViewItem;
  int                 componentLen, count, bufferViewIndex;

  if (!(jaccessors = json_array(json)))
    return AK_ERR;

  json = json->value;

  if (accMapLen == 0) {
    accMapLen = AK_ARRAY_LEN(accMap);
    qsort(accMap, accMapLen, sizeof(accMap[0]), ak_enumpair_cmp);
  }

  heap         = gst->heap;
  jmin         = NULL;
  jmax         = NULL;
  componentLen = 1;

  while (json) {
    acc   = ak_heap_calloc(heap, gst->doc, sizeof(*acc));
    jattr = json->value;

    ak_setypeid(acc, AKT_ACCESSOR);

    while (jattr) {
      const ak_enumpair *found;

      if (!(found = bsearch(jattr,
                            accMap,
                            accMapLen,
                            sizeof(accMap[0]),
                            ak_enumpair_json_key_cmp)))
        goto cont;

      switch (found->val) {
        case k_s_gltf_bufferView: {
          if ((bufferViewIndex = json_int32(jattr, -1)) > -1
              && (buffViewItem = flist_sp_at(&gst->doc->lib.bufferViews,
                                             bufferViewIndex)))
            acc->bufferView = buffViewItem->data;
          break;
        }
        case k_s_gltf_byteOffset:
          acc->byteOffset = json_uint64(jattr, 0);
          break;
        case k_s_gltf_componentType: {
          int componentType;
          componentType      = json_int32(jattr, -1);
          acc->componentType = gltf_componentType(componentType);
          componentLen       = gltf_componentLen(componentType);
          acc->type          = ak_typeDesc(acc->componentType);
          break;
        }
        case k_s_gltf_normalized:
          acc->normalized = json_bool(jattr, false);
          break;
        case k_s_gltf_count:
          acc->count = json_int64(jattr, 0);
          break;
        case k_s_gltf_type:
          acc->componentSize = gltf_type(json_string(jattr), jattr->valSize);
          break;
        case k_s_gltf_max:
          /* wait to get component type and size */
          jmax = jattr;
          break;
        case k_s_gltf_min:
          /* wait to get component type and size */
          jmin = jattr;
          break;

        case k_s_gltf_name:
          acc->name = json_strdup(jattr, heap, acc);
          break;
        case k_s_gltf_sparse:
          /* TODO: */
          break;
      }

    cont:
      jattr = jattr->next;
    }

    if (acc->componentSize < 5)
      acc->componentBytes = acc->componentSize * componentLen;
    else
      acc->componentBytes = (acc->componentSize >> 3) * componentLen;

    if (acc->componentSize != AK_COMPONENT_SIZE_UNKNOWN
        && acc->componentBytes > 0) {
      if (jmin && jmin->value) {
        acc->min = ak_heap_alloc(heap, acc, acc->componentBytes);

        if ((jarr = json_array(jmin))) {
          jitem = &jarr->base;
          count = jarr->count;

          while (jitem) {
            json_array_set(acc->min, acc->componentType, --count, jmin);
            jitem = jitem->next;
          }
        } else {
          json_array_set(acc->min, acc->componentType, 0, jmin);
        }
      }

      if (jmax) {
        acc->max = ak_heap_alloc(heap, acc, acc->componentBytes);

        if ((jarr = json_array(jmax))) {
          jitem = &jarr->base;
          count = jarr->count;

          while (jitem) {
            json_array_set(acc->max, acc->componentType, --count, jmax);
            jitem = jitem->next;
          }
        } else {
          json_array_set(acc->max, acc->componentType, 0, jmax);
        }
      }
    }

    flist_sp_insert(&gst->doc->lib.accessors, acc);

    jmin = NULL;
    jmax = NULL;
    json = json->next;
  }

  return AK_OK;
}
