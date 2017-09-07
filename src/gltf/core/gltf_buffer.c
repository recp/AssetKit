/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_buffer.h"
#include "../../ak_utils.h"

AkBuffer*
gltf_buffer(AkGLTFState * __restrict gst,
            int32_t                  bufferView,
            size_t                  *byteStride) {
  AkHeap    *heap;
  json_t    *jbuffViews, *jbuffView, *jbuffs,  *jbuff;
  AkBuffer  *buff;
  FListItem *buffers;
  size_t     jbuffsSize;
  int32_t    bufferIndex;

  buff = rb_find(gst->bufferViews, (void *)(intptr_t)bufferView);
  if (buff) {
    *byteStride = buff->reserved;
    return buff;
  }

  jbuffViews = json_object_get(gst->root, _s_gltf_bufferViews);
  jbuffView  = json_array_get(jbuffViews, bufferView);
  if (!jbuffView)
    return NULL;

  bufferIndex = json_int32(jbuffView, _s_gltf_bufferView);

  jbuffs      = json_object_get(gst->root, _s_gltf_buffers);
  jbuffsSize  = json_array_size(jbuffs);
  if (jbuffsSize < 1)
    return NULL;

  heap    = gst->heap;
  buffers = gst->doc->lib.buffers;
  buff    = ak_heap_calloc(heap,
                           buffers,
                           sizeof(*buff));

  /* read buffer  */
  if ((jbuff = json_object_get(jbuffView, _s_gltf_buffer))) {
    FListItem *buffIter;

    buffIter = gst->buffers;
    while (bufferIndex > 0)
      buffIter = buffIter->next;

    if (buffIter) {
      AkBuffer   *gltfbuff;
      void       *data;
      const char *sval;
      size_t      byteLength, _byteStride, byteOffset;

      gltfbuff = buffIter->data;

      /* we will return bufferView data as AkBuffer,
         because we don't what is in gltf buffer (for now)
       */
      byteLength  = json_int64(jbuffView, _s_gltf_byteLength);
      _byteStride = json_int32(jbuffView, _s_gltf_byteStride);
      byteOffset  = json_int64(jbuffView, _s_gltf_byteOffset);

      if (byteStride)
        *byteStride = _byteStride;

      buff->data = data = ak_heap_alloc(heap, buff, byteLength);
      memcpy(data,
             (char *)gltfbuff->data + byteOffset,
             byteLength);

      if ((sval = json_cstr(jbuffView, _s_gltf_name)))
        buff->name = ak_heap_strdup(gst->heap, buff, sval);

      buff->reserved = (size_t)byteStride;
      buff->length   = byteLength;

      rb_insert(gst->bufferViews,
                (void *)(uintptr_t)bufferView,
                buff);
    }
  }

  return buff;
}

AkResult _assetkit_hide
gltf_buffers(AkGLTFState * __restrict gst) {
  json_t     *jbuffers;
  const char *sval;
  int32_t     i, jbuffersSize;

  jbuffers     = json_object_get(gst->root, _s_gltf_buffers);
  jbuffersSize = (int32_t)json_array_size(jbuffers);

  for (i = jbuffersSize - 1; i >= 0; i--) {
    AkBuffer *buff;
    json_t   *jbuffer;

    jbuffer = json_array_get(jbuffers, i);
    buff    = ak_heap_calloc(gst->heap,
                             gst->doc,
                             sizeof(*buff));

    /* load buffer into memory */
    if ((sval = json_cstr(jbuffer, _s_gltf_uri))) {
      const char *localurl;
      AkResult    ret;

      localurl = ak_getFileFrom(gst->doc, sval);
      ret      = ak_readfile(localurl,
                             "rb",
                             &buff->data,
                             &buff->length);
    }

    if ((sval = json_cstr(jbuffer, _s_gltf_name)))
      buff->name = ak_heap_strdup(gst->heap, buff, sval);

    flist_sp_insert(&gst->buffers, buff);
  }
  
  return AK_OK;
}
