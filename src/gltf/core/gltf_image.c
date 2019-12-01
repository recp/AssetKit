/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_image.h"
#include "base64.h"

void _assetkit_hide
gltf_images(json_t * __restrict jimage,
            void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  const json_array_t *jimages;
  const json_t       *jimVal;
  AkImage            *image;
  AkBuffer           *buff;

  if (!(jimages = json_array(jimage)))
    return;

  gst    = userdata;
  heap   = gst->heap;
  jimage = jimages->base.value;

  while (jimage) {
    bool foundUri;

    image    = ak_heap_calloc(gst->heap, gst->doc, sizeof(*image));
    jimVal   = jimage->value;
    foundUri = false;

    while (jimVal) {
      if (json_key_eq(jimVal, _s_gltf_uri)) {
        AkInitFrom *initFrom;
        initFrom = ak_heap_calloc(heap, image, sizeof(*initFrom));
        
        if (strncmp(jimVal->value, _s_gltf_b64d, strlen(_s_gltf_b64d)) == 0) {
          char *uri;
          uri                  = jimVal->value;
          uri[jimVal->valSize] = '\0';
          
          initFrom->buff = ak_heap_calloc(heap, gst->doc, sizeof(*buff));
          base64_buff(uri, jimVal->valSize, initFrom->buff);
        } else {
          initFrom->ref = json_strdup(jimVal, gst->heap, initFrom);
        }
        image->initFrom = initFrom;
      } else if (json_key_eq(jimVal, _s_gltf_name)) {
        image->name = json_strdup(jimVal, gst->heap, image);
      }

      jimVal = jimVal->next;
    }

    flist_sp_insert(&gst->doc->lib.images, image);
    jimage = jimage->next;
  }
}
