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

#define k_gltf_bufferView    0
#define k_gltf_byteOffset    1
#define k_gltf_componentType 2
#define k_gltf_normalized    3
#define k_gltf_count         4
#define k_gltf_type          5
#define k_gltf_max           6
#define k_gltf_min           7
#define k_gltf_sparse        8
#define k_gltf_name          9

void _assetkit_hide
gltf_accessors(json_t * __restrict json,
               void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkDoc              *doc;
  AkHeap             *heap;
  const json_array_t *jaccessors, *jarr;
  const json_t       *jattr, *jitem, *it;
  AkAccessor         *acc;
  int                 componentLen, count, bound;

  if (!(jaccessors = json_array(json)))
    return;

  gst          = userdata;
  doc          = gst->doc;
  heap         = gst->heap;
  json         = jaccessors->base.value;
  componentLen = 1;

  while (json) {
    acc   = ak_heap_calloc(heap, doc, sizeof(*acc));
    jattr = json->value;

    ak_setypeid(acc, AKT_ACCESSOR);

    json_objmap_t accMap[] = {
      JSON_OBJMAP_OBJ(_s_gltf_bufferView,    I2P k_gltf_bufferView),
      JSON_OBJMAP_OBJ(_s_gltf_byteOffset,    I2P k_gltf_byteOffset),
      JSON_OBJMAP_OBJ(_s_gltf_componentType, I2P k_gltf_componentType),
      JSON_OBJMAP_OBJ(_s_gltf_normalized,    I2P k_gltf_normalized),
      JSON_OBJMAP_OBJ(_s_gltf_count,         I2P k_gltf_count),
      JSON_OBJMAP_OBJ(_s_gltf_type,          I2P k_gltf_type),
      JSON_OBJMAP_OBJ(_s_gltf_max,           I2P k_gltf_max),
      JSON_OBJMAP_OBJ(_s_gltf_min,           I2P k_gltf_min),
      JSON_OBJMAP_OBJ(_s_gltf_sparse,        I2P k_gltf_sparse),
      JSON_OBJMAP_OBJ(_s_gltf_name,          I2P k_gltf_name)
    };

    json_objmap(json, accMap, JSON_ARR_LEN(accMap));
  
    if ((it = accMap[k_gltf_name].object)) {
      acc->name = json_strdup(it, heap, acc);
    }
    
    /* convert merge bufferView with acessor and buffer */
    if ((it = accMap[k_gltf_bufferView].object)) {
      AkBuffer     *buff, *tmpbuff;
      AkBufferView *buffView;
      int32_t       buffViewIndex;
      
      if ((buffViewIndex = json_int32(it, -1)) > -1
          && (buffView = flist_sp_at(&gst->bufferViews, buffViewIndex))
          && (tmpbuff = buffView->buffer)) {
        if (!(buff = rb_find(gst->bufferMap, buffView))) {
          buff         = ak_heap_calloc(heap, doc, sizeof(*buff));
          buff->data   = ak_heap_alloc(heap, buff, buffView->byteLength);
          buff->length = buffView->byteLength;
          
          memcpy(buff->data,
                 (char *)tmpbuff->data + buffView->byteOffset,
                 buffView->byteLength);
          
          rb_insert(gst->bufferMap, buffView, buff);
        }
        
        acc->byteStride = buffView->byteStride;
        acc->buffer     = buff;
        acc->source.ptr = buff;
        
        flist_sp_insert(&doc->lib.buffers, buff);
      }
    }
    
    assert(acc->buffer);
    
    if ((it = accMap[k_gltf_byteOffset].object)) {
      acc->byteOffset = json_uint64(it, 0);
    }
    
    if ((it = accMap[k_gltf_componentType].object)) {
      int componentType;
      componentType      = json_int32(it, -1);
      acc->componentType = gltf_componentType(componentType);
      componentLen       = gltf_componentLen(componentType);
      acc->type          = ak_typeDesc(acc->componentType);
    }
    
    if ((it = accMap[k_gltf_normalized].object)) {
      acc->normalized = json_bool(it, false);
    }
    
    if ((it = accMap[k_gltf_count].object)) {
      acc->count = json_int64(it, 0);
    }
    
    if ((it = accMap[k_gltf_type].object)) {
      acc->componentSize = gltf_type(json_string(it), it->valSize);
    }

    /* prepare for min and max */
    if (acc->componentSize < 5) {
      bound               = acc->componentSize;
      acc->componentBytes = bound * componentLen;
      acc->bound          = bound;
    } else {
      bound               = acc->componentSize >> 3;
      acc->componentBytes = bound * componentLen;
      acc->bound          = bound;
    }

    if (acc->componentSize != AK_COMPONENT_SIZE_UNKNOWN
        && acc->componentBytes > 0) {
      if ((it = accMap[k_gltf_min].object) && it->value) {
        acc->min = ak_heap_alloc(heap, acc, acc->componentBytes);

        if ((jarr = json_array(it))) {
          jitem = jarr->base.value;
          count = jarr->count;

          while (jitem) {
            json_array_set(acc->min, acc->componentType, --count, it);
            jitem = jitem->next;
          }
        } else {
          json_array_set(acc->min, acc->componentType, 0, it);
        }
      }

      if ((it = accMap[k_gltf_max].object) && it->value) {
        acc->max = ak_heap_alloc(heap, acc, acc->componentBytes);

        if ((jarr = json_array(it))) {
          jitem = jarr->base.value;
          count = jarr->count;

          while (jitem) {
            json_array_set(acc->max, acc->componentType, --count, it);
            jitem = jitem->next;
          }
        } else {
          json_array_set(acc->max, acc->componentType, 0, it);
        }
      }
    }

    /* TODO: */
    /*
    if ((it = accMap[k_gltf_sparse].object)) {
     
    }
     */
    
    flist_sp_insert(&gst->doc->lib.accessors, acc);

    json = json->next;
  }
}
