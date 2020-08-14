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

#include "buffer.h"
#include "../../../utils.h"
#include "../../../base64.h"

void
gltf_bufferViews(json_t * __restrict jbuffView,
                 void   * __restrict userdata) {
  AkGLTFState        *gst;
  const json_array_t *jbuffers;
  const json_t       *jbuffVal;
  AkBufferView       *buffView;
  int32_t             buffIndex;

  if (!(jbuffers = json_array(jbuffView)))
    return;

  gst = userdata;

  jbuffView = jbuffers->base.value;
  while (jbuffView) {
    buffView = ak_heap_calloc(gst->heap, gst->tmpParent, sizeof(*buffView));
    jbuffVal = jbuffView->value;

    while (jbuffVal) {
      if (json_key_eq(jbuffVal, _s_gltf_buffer)) {
        if ((buffIndex = json_int32(jbuffVal, -1)) > -1)
          buffView->buffer = flist_sp_at(&gst->buffers, buffIndex);
      } else if (json_key_eq(jbuffVal, _s_gltf_byteLength)) {
        buffView->byteLength = (size_t)json_uint64(jbuffVal, 0);
      } else if (json_key_eq(jbuffVal, _s_gltf_byteOffset)) {
        buffView->byteOffset = (size_t)json_uint64(jbuffVal, 0);
      } else if (json_key_eq(jbuffVal, _s_gltf_byteStride)) {
        buffView->byteStride = (size_t)json_uint64(jbuffVal, 1);
      } else if (json_key_eq(jbuffVal, _s_gltf_name)) {
        buffView->name = json_strdup(jbuffVal, gst->heap, buffView);
      }

      jbuffVal = jbuffVal->next;
    }

    flist_sp_insert(&gst->bufferViews, buffView);
    jbuffView = jbuffView->next;
  }
}

void
gltf_buffers(json_t * __restrict jbuff,
             void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  const json_array_t *jbuffers;
  const json_t       *jbuffVal;
  char               *localurl;
  char               *uri;
  AkBuffer           *buff;
    
  if (!(jbuffers = json_array(jbuff)))
    return;

  gst   = userdata;
  heap  = gst->heap;
  jbuff = jbuffers->base.value;

  while (jbuff) {
    bool foundUri;

    buff     = ak_heap_calloc(heap, gst->tmpParent, sizeof(*buff));
    jbuffVal = jbuff->value;
    foundUri = false;

    while (jbuffVal) {
      if (json_key_eq(jbuffVal, _s_gltf_uri)) {
        uri = json_string_dup(jbuffVal);

        if (strncmp(uri, _s_gltf_b64d, strlen(_s_gltf_b64d)) == 0) {
          base64_buff(uri, jbuffVal->valsize, buff);
        } else {
          localurl = ak_getFileFrom(gst->doc, uri);
          ak_readfile(localurl, &buff->data, &buff->length);
          ak_free(localurl);
        }

        if (uri)
          free(uri);

        foundUri = true;

        /* TODO: log if logging enabled (or by log level) */
      } else if (json_key_eq(jbuffVal, _s_gltf_name)) {
        buff->name = json_strdup(jbuffVal, heap, buff);
      }

      jbuffVal = jbuffVal->next;
    }
    
    if (!foundUri && gst->bindata) {
      buff->data   = gst->bindata;
      buff->length = gst->bindataLen;
    }

    flist_sp_insert(&gst->buffers, buff);
    jbuff = jbuff->next;
  }
}
