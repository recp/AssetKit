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
#include "../extra.h"
#include "../../../../base64.h"

#define k_name          0
#define k_bufferView    1
#define k_uri           2
#define k_mimeType      3

static inline
bool
gltf_imageDataUriHasPrefix(const char * __restrict uri,
                           int                     len) {
  return uri
      && len > _s_gltf_b64d_len
      && ak_str_pack8_fast(uri, _s_gltf_b64d_len) == _s_gltf_b64d_u64_exact;
}

static
char*
gltf_imageDataUriMime(AkHeap      * __restrict heap,
                      void        * __restrict parent,
                      const char  * __restrict uri,
                      int                      len) {
  const char *start;
  const char *end;

  if (!gltf_imageDataUriHasPrefix(uri, len))
    return NULL;

  start = uri + _s_gltf_b64d_len;
  end   = memchr(start, ';', (size_t)len - _s_gltf_b64d_len);
  if (!end || end <= start)
    return NULL;

  return ak_heap_strndup(heap, parent, start, (size_t)(end - start));
}

AK_HIDE
void
gltf_images(json_t * __restrict jimage,
            void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  const json_array_t *jimages;
  AkImage            *image;
  json_t             *it;
  size_t              imageIndex;

  if (!(jimages = json_array(jimage)))
    return;

  gst    = userdata;
  heap   = gst->heap;
  jimage = jimages->base.value;
  gst->imagesCount   = jimages->count;
  gst->imagesByIndex = ak_heap_calloc(heap,
                                      gst->tmpParent,
                                      sizeof(*gst->imagesByIndex)
                                      * gst->imagesCount);
  imageIndex = gst->imagesCount;

  while (jimage) {
    AkInitFrom *initFrom;

    image    = ak_heap_calloc(gst->heap, gst->doc, sizeof(*image));
    initFrom = NULL;
    gltf_extra(gst,
               image,
               GLTF_JSON_GET8(jimage, extras),
               GLTF_JSON_GET(jimage, extensions));
    
    json_objmap_t imgMap[] = {
      GLTF_JSON_OBJMAP_OBJ8(name,         I2P k_name),
      JSON_OBJMAP_OBJ(_s_gltf_bufferView, I2P k_bufferView),
      GLTF_JSON_OBJMAP_OBJ8(uri,          I2P k_uri),
      GLTF_JSON_OBJMAP_OBJ8(mimeType,     I2P k_mimeType)
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
          && (buffView = gltf_bufferView_at(gst, buffViewIndex))
          && (tmpbuff = buffView->buffer)) {
        initFrom             = ak_heap_calloc(heap, image, sizeof(*initFrom));
        initFrom->buff       = ak_heap_calloc(heap,
                                              gst->doc,
                                              sizeof(*initFrom->buff));
        initFrom->buff->length = buffView->byteLength;

        if (gst->borrowBufferViews) {
          initFrom->buff->data = (char *)tmpbuff->data + buffView->byteOffset;
          initFrom->buff->name = "assetkit:gltf-buffer-view-slice";
        } else {
          initFrom->buff->data = ak_heap_alloc(heap,
                                               initFrom->buff,
                                               buffView->byteLength);
          memcpy(initFrom->buff->data,
                 (char *)tmpbuff->data + buffView->byteOffset,
                 buffView->byteLength);
        }
        if ((it = imgMap[k_mimeType].object))
          initFrom->buffMime = json_strdup(it, heap, initFrom);
        image->initFrom = initFrom;
      }
    }
    
    if (!initFrom && (it = imgMap[k_uri].object)) {
      initFrom = ak_heap_calloc(heap, image, sizeof(*initFrom));
      
      if (gltf_imageDataUriHasPrefix(it->value, it->valsize)) {
        char *uri;
        uri = it->value;

        initFrom->buff = ak_heap_calloc(heap, gst->doc, sizeof(*initFrom->buff));
        base64_buff(uri, it->valsize, initFrom->buff);
        if (imgMap[k_mimeType].object)
          initFrom->buffMime = json_strdup(imgMap[k_mimeType].object, heap, initFrom);
        else
          initFrom->buffMime = gltf_imageDataUriMime(heap, initFrom, uri, it->valsize);
      } else {
        initFrom->ref = json_strdup(it, gst->heap, initFrom);
      }
      image->initFrom = initFrom;
    }

    flist_sp_insert(&gst->doc->lib.images, image);
    if (imageIndex > 0)
      gst->imagesByIndex[--imageIndex] = image;
    jimage = jimage->next;
  }
}
