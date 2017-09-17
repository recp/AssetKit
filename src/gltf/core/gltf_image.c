/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_image.h"

void _assetkit_hide
gltf_images(AkGLTFState * __restrict gst) {
  AkHeap        *heap;
  AkDoc         *doc;
  json_t        *jimages;
  AkLibItem     *lib;
  AkImage       *last_img;
  size_t         jimageCount, i;

  heap     = gst->heap;
  doc      = gst->doc;
  lib      = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_img = NULL;

  jimages     = json_object_get(gst->root, _s_gltf_images);
  jimageCount = json_array_size(jimages);
  for (i = 0; i < jimageCount; i++) {
    AkImage    *image;
    AkInitFrom *initFrom;
    json_t     *jimage;
    const char *sval;

    jimage = json_array_get(jimages, i);
    image  = ak_heap_calloc(heap, lib, sizeof(*image));

    if ((sval = json_cstr(jimage, _s_gltf_name)))
      image->name = ak_heap_strdup(gst->heap, image, sval);

    if ((sval = json_cstr(jimage, _s_gltf_uri))) {
      initFrom        = ak_heap_calloc(heap, image, sizeof(*initFrom));
      initFrom->ref   = ak_heap_strdup(heap, initFrom, sval);
      image->initFrom = initFrom;
    }

    /* TODO: init from buffer, init from data url , mimeType */

    if (last_img)
      last_img->next = image;
    else
      lib->chld = image;

    last_img = image;
    lib->count++;
  }

  doc->lib.images = lib;
}
