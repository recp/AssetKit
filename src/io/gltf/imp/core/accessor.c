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

#include "accessor.h"
#include "enum.h"
#include "buffer.h"
#include "../../../../accessor.h"

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

AK_HIDE
void
gltf_accessors(json_t * __restrict json,
               void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkDoc              *doc;
  AkHeap             *heap;
  const json_array_t *jaccessors, *jarr;
  const json_t       *jitem, *it;
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
    acc = ak_heap_calloc(heap, doc, sizeof(*acc));

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
    
    /*  merge bufferView with acessor and buffer */
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

        flist_sp_insert(&doc->lib.buffers, buff);
      }
    }

    if ((it = accMap[k_gltf_byteOffset].object)) {
      acc->byteOffset = json_uint64(it, 0);
    }
    
    if ((it = accMap[k_gltf_componentType].object)) {
      int componentType;
      componentType      = json_int32(it, -1);
      acc->componentType = gltf_componentType(componentType);
      componentLen       = gltf_componentLen(componentType);
    }
    
    if ((it = accMap[k_gltf_normalized].object)) {
      acc->normalized = json_bool(it, false);
    }
    
    if ((it = accMap[k_gltf_count].object)) {
      acc->count = json_uint32(it, 0);
    }
    
    if ((it = accMap[k_gltf_type].object)) {
      acc->componentSize = gltf_type(it);
    }

    /* prepare for min and max */
    if (acc->componentSize < 5) {
      bound                  = acc->componentSize;
      acc->bytesPerComponent = componentLen;
      acc->componentCount    = bound;
    } else {
      bound                  = acc->componentSize >> 3;
      acc->bytesPerComponent = componentLen;
      acc->componentCount    = bound;
    }
    
    acc->byteLength   = acc->bytesPerComponent * acc->count;
    acc->fillByteSize = acc->bytesPerComponent * bound;

    if (acc->componentSize != AK_COMPONENT_SIZE_UNKNOWN
        && acc->fillByteSize > 0) {
      if ((it = accMap[k_gltf_min].object) && it->value) {
        acc->min = ak_heap_alloc(heap, acc, acc->fillByteSize);

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
        acc->max = ak_heap_alloc(heap, acc, acc->fillByteSize);

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
