/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_buffer.h"
#include "../../ak_utils.h"

/*
  glTF buffer                -> AkSource
  glTF accessor + bufferView -> AkAccessor
 */

AkResult _assetkit_hide
gltf_buffers(AkGLTFState * __restrict gst,
             void        * __restrict memParent,
             AkSource    * __restrict dest) {
  json_t     *jbuffers;
  AkSource   *source;
  const char *sval;
  FListItem  *buffers;
  size_t      i, jbuffersSize;

  source = ak_heap_calloc(gst->heap, memParent, sizeof(*source));

  if (!dest) {
    dest = ak_heap_calloc(gst->heap,
                          memParent,
                          sizeof(*dest));
  }

  jbuffers     = json_object_get(gst->root, _s_gltf_buffers);
  jbuffersSize = json_array_size(jbuffers);
  buffers      = gst->doc->lib.buffers;

  for (i = 0; i < jbuffersSize; i++) {
    AkBuffer *buff;
    json_t   *jbuffer;

    jbuffer = json_array_get(jbuffers, i);
    buff    = ak_heap_calloc(gst->heap,
                             buffers,
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

    flist_sp_insert(&buffers, buff);
  }
  
  return AK_OK;
}
