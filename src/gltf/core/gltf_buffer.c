/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_buffer.h"
#include "../../utils.h"

AkResult _assetkit_hide
gltf_bufferViews(AkGLTFState  * __restrict gst,
                 const json_t * __restrict json) {
  const json_array_t *jbuffers;
  const json_t       *jbuffAttrib;
  AkBufferView       *buffView;
  FListItem          *buffItem;
  int32_t             buffIndex;

  if (!(jbuffers = json_array(json)))
    return AK_ERR;

  json = &jbuffers->base;
  while (json) {
    buffView    = ak_heap_calloc(gst->heap, gst->doc, sizeof(*buffView));
    jbuffAttrib = json->value;

    while (jbuffAttrib) {
      if (json_key_eq(jbuffAttrib, _s_gltf_buffer)) {
        if ((buffIndex = json_int32(jbuffAttrib, -1)) > 0
            && (buffItem = flist_sp_at(&gst->doc->lib.buffers, buffIndex)))
          buffView->buffer = buffItem->data;
      } else if (json_key_eq(jbuffAttrib, _s_gltf_byteLength)) {
        buffView->byteLength = json_uint64(jbuffAttrib, 0);
      } else if (json_key_eq(jbuffAttrib, _s_gltf_byteOffset)) {
        buffView->byteOffset = json_uint64(jbuffAttrib, 0);
      } else if (json_key_eq(jbuffAttrib, _s_gltf_byteStride)) {
        buffView->byteStride = json_uint64(jbuffAttrib, 1);
      } else if (json_key_eq(jbuffAttrib, _s_gltf_name)) {
        buffView->name = json_strdup(jbuffAttrib, gst->heap, buffView);
      }

      jbuffAttrib = jbuffAttrib->next;
    }

    flist_sp_insert(&gst->doc->lib.bufferViews, buffView);
    json = json->next;
  }
  
  return AK_OK;
}

AkResult _assetkit_hide
gltf_buffers(AkGLTFState  * __restrict gst,
             const json_t * __restrict json) {
  AkHeap             *heap;
  const json_array_t *jbuffers;
  const json_t       *jbuffAttrib;
  const char         *localurl;
  char               *uri_tmp;
  AkBuffer           *buff;

  if (!(jbuffers = json_array(json)))
    return AK_ERR;

  heap = gst->heap;
  json = &jbuffers->base;

  while (json) {
    buff        = ak_heap_calloc(heap, gst->doc, sizeof(*buff));
    jbuffAttrib = json->value;

    while (jbuffAttrib) {
      if (json_key_eq(jbuffAttrib, _s_gltf_uri)) {
        uri_tmp  = json_string_dup(jbuffAttrib);
        localurl = ak_getFileFrom(gst->doc, uri_tmp);

        ak_readfile(localurl, "rb", &buff->data, &buff->length);
        free(uri_tmp);

        /* TODO: log if logging enabled (or by log level) */
      } else if (json_key_eq(jbuffAttrib, _s_gltf_name)) {
        buff->name = json_strdup(jbuffAttrib, heap, buff);
      }

      jbuffAttrib = jbuffAttrib->next;
    }

    flist_sp_insert(&gst->doc->lib.buffers, buff);
    json = json->next;
  }
  
  return AK_OK;
}
