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
#include "ext.h"
#include "../../../../utils.h"
#include "../../../../base64.h"

#include <string.h>

typedef struct AkGLTFBufferViewProps {
  const json_t *buffer;
  const json_t *byteLength;
  const json_t *byteOffset;
  const json_t *byteStride;
  const json_t *name;
  const json_t *extensions;
} AkGLTFBufferViewProps;

static inline
void
gltf_bufferViewProps(const json_t           * __restrict jbuffView,
                     AkGLTFBufferViewProps * __restrict props) {
  const json_t *it;
  char          first;

  if (!jbuffView || jbuffView->type != JSON_OBJECT)
    return;

  for (it = jbuffView->value; it; it = it->next) {
    if (!it->key)
      continue;

    first = it->key[0];
    switch (it->keysize) {
      case 4:
        if (first == 'n' && GLTF_JSON_KEY_EQ8(it, name))
          props->name = it;
        break;
      case 6:
        if (first == 'b' && GLTF_JSON_KEY_EQ8(it, buffer))
          props->buffer = it;
        break;
      case 10:
        if (first == 'e' && gltf_jsonKeyEqLen(it, _s_gltf_extensions, 10)) {
          props->extensions = it;
        } else if (first == 'b') {
          switch (it->key[4]) {
            case 'L':
              if (gltf_jsonKeyEqLen(it, _s_gltf_byteLength, 10))
                props->byteLength = it;
              break;
            case 'O':
              if (gltf_jsonKeyEqLen(it, _s_gltf_byteOffset, 10))
                props->byteOffset = it;
              break;
            case 'S':
              if (gltf_jsonKeyEqLen(it, _s_gltf_byteStride, 10))
                props->byteStride = it;
              break;
            default:
              break;
          }
        }
        break;
      default:
        break;
    }
  }
}

void
gltf_bufferViews(json_t * __restrict jbuffView,
                 void   * __restrict userdata) {
  AkGLTFState        *gst;
  const json_array_t *jbuffers;
  const json_t       *it;
  AkBufferView       *buffView;
  AkGLTFBufferViewProps props;
  size_t              buffViewIndex;
  int32_t             buffIndex;

  if (!(jbuffers = json_array(jbuffView)))
    return;

  gst = userdata;
  gst->bufferViewsCount   = jbuffers->count;
  gst->bufferViewsByIndex = ak_heap_calloc(gst->heap,
                                           gst->tmpParent,
                                           sizeof(*gst->bufferViewsByIndex)
                                           * gst->bufferViewsCount);
  buffViewIndex = gst->bufferViewsCount;

  jbuffView = jbuffers->base.value;
  while (jbuffView) {
    buffView = ak_heap_calloc(gst->heap, gst->tmpParent, sizeof(*buffView));
    memset(&props, 0, sizeof(props));

    gltf_bufferViewProps(jbuffView, &props);

    if ((it = props.buffer)
        && (buffIndex = json_int32(it, -1)) > -1)
      buffView->buffer = gltf_buffer_at(gst, buffIndex);
    if ((it = props.byteLength))
      buffView->byteLength = (size_t)json_uint64(it, 0);
    if ((it = props.byteOffset))
      buffView->byteOffset = (size_t)json_uint64(it, 0);
    if ((it = props.byteStride))
      buffView->byteStride = (size_t)json_uint64(it, 0);
    if ((it = props.name))
      buffView->name = json_strdup(it, gst->heap, buffView);

    if (!gltf_ext_bufferView(gst, buffView, props.extensions)) {
      gst->stop = true;
      return;
    }

    if (buffViewIndex > 0)
      gst->bufferViewsByIndex[--buffViewIndex] = buffView;
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
  size_t              buffIndex;
    
  if (!(jbuffers = json_array(jbuff)))
    return;

  gst   = userdata;
  heap  = gst->heap;
  jbuff = jbuffers->base.value;
  gst->buffersCount   = jbuffers->count;
  gst->buffersByIndex = ak_heap_calloc(heap,
                                       gst->tmpParent,
                                       sizeof(*gst->buffersByIndex)
                                       * gst->buffersCount);
  buffIndex = gst->buffersCount;

  while (jbuff) {
    bool foundUri;
    void *buffParent;

    buffParent = gst->borrowBufferViews ? gst->doc : gst->tmpParent;
    buff       = ak_heap_calloc(heap, buffParent, sizeof(*buff));
    jbuffVal = jbuff->value;
    foundUri = false;

    while (jbuffVal) {
      if (GLTF_JSON_KEY_EQ8(jbuffVal, uri)) {
        size_t uriLen;

        uriLen = jbuffVal->valsize;
        uri    = malloc(uriLen + 1u);
        if (!uri) {
          jbuffVal = jbuffVal->next;
          continue;
        }

        memcpy(uri, json_string(jbuffVal), uriLen);
        uri[uriLen] = '\0';

        if (jbuffVal->valsize > _s_gltf_b64d_len
            && ak_str_pack8_fast(uri, _s_gltf_b64d_len)
               == _s_gltf_b64d_u64_exact) {
          base64_buff(uri, jbuffVal->valsize, buff);
        } else {
          localurl = ak_getFileFrom(gst->doc, uri);
          if (localurl) {
            ak_readfile(localurl, buff, &buff->data, &buff->length);
            ak_free(localurl);
          }
        }

        if (uri)
          free(uri);

        foundUri = true;

        /* TODO: log if logging enabled (or by log level) */
      } else if (GLTF_JSON_KEY_EQ8(jbuffVal, name)) {
        buff->name = json_strdup(jbuffVal, heap, buff);
      }

      jbuffVal = jbuffVal->next;
    }
    
    if (!foundUri && gst->bindata) {
      buff->data   = gst->bindata;
      buff->length = gst->bindataLen;
    }

    flist_sp_insert(&gst->buffers, buff);
    if (gst->borrowBufferViews) {
      AK_LIB_PREPEND(gst->doc->lib.buffers, buff, next);
    }
    if (buffIndex > 0)
      gst->buffersByIndex[--buffIndex] = buff;
    jbuff = jbuff->next;
  }
}
