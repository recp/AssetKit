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

#include "image.h"
#include "../../../../base64.h"

#define k_name          0
#define k_bufferView    1
#define k_uri           2

AK_HIDE
void
gltf_images(json_t * __restrict jimage,
            void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  const json_array_t *jimages;
  AkImage            *image;
  json_t             *it;

  if (!(jimages = json_array(jimage)))
    return;

  gst    = userdata;
  heap   = gst->heap;
  jimage = jimages->base.value;

  while (jimage) {
    AkInitFrom *initFrom;

    image    = ak_heap_calloc(gst->heap, gst->doc, sizeof(*image));
    initFrom = NULL;
    
    json_objmap_t imgMap[] = {
      JSON_OBJMAP_OBJ(_s_gltf_name,       I2P k_name),
      JSON_OBJMAP_OBJ(_s_gltf_bufferView, I2P k_bufferView),
      JSON_OBJMAP_OBJ(_s_gltf_uri,        I2P k_uri)
    };

    json_objmap(jimage, imgMap, JSON_ARR_LEN(imgMap));

    if ((it = imgMap[k_name].object)) {
      image->name = json_strdup(it, gst->heap, image);
    }
    
    if ((it = imgMap[k_bufferView].object)) {
      AkBuffer     *tmpbuff;
      AkBufferView *buffView;
      int32_t       buffViewIndex;
      
      if ((buffViewIndex = json_int32(it, -1)) > -1
          && (buffView = flist_sp_at(&gst->bufferViews, buffViewIndex))
          && (tmpbuff = buffView->buffer)) {
        initFrom             = ak_heap_calloc(heap, image, sizeof(*initFrom));
        initFrom->buff       = ak_heap_calloc(heap,
                                              gst->doc,
                                              sizeof(*initFrom->buff));
        initFrom->buff->data = ak_heap_alloc(heap,
                                             initFrom->buff,
                                             buffView->byteLength);
        initFrom->buff->length = buffView->byteLength;
        
        memcpy(initFrom->buff->data,
               (char *)tmpbuff->data + buffView->byteOffset,
               buffView->byteLength);
        image->initFrom = initFrom;
      }
    }
    
    if (!initFrom && (it = imgMap[k_uri].object)) {
      initFrom = ak_heap_calloc(heap, image, sizeof(*initFrom));
      
      if (!strncmp(it->value, _s_gltf_b64d, strlen(_s_gltf_b64d))) {
        char *uri;
        uri              = it->value;
        uri[it->valsize] = '\0';
        
        initFrom->buff = ak_heap_calloc(heap, gst->doc, sizeof(*initFrom->buff));
        base64_buff(uri, it->valsize, initFrom->buff);
      } else {
        initFrom->ref = json_strdup(it, gst->heap, initFrom);
      }
      image->initFrom = initFrom;
    }

    flist_sp_insert(&gst->doc->lib.images, image);
    jimage = jimage->next;
  }
}
