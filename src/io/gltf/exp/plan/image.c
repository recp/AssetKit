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

#include "internal.h"

AK_HIDE
bool
gltf_plan_image_payload(GLTFExpState * __restrict st,
                        AkImage      * __restrict image,
                        GLTFExpIndex              imageIndex) {
  AkImageExportPayload *payload;
  AkImageExportRequest  req;

  if (!st->imagePayloads || imageIndex >= st->images.count)
    return false;

  payload             = &st->imagePayloads[imageIndex];
  req.doc             = st->doc;
  req.image           = image;
  req.targetMimeType  = _s_gltf_mime_image_png;

  if (!ak_imageExportPNG(&req, payload)
      || !payload->data
      || payload->byteLength == 0)
    return false;

  if (!gltf_accessors_add_raw_view(&st->accessors,
                                   image,
                                   payload->data,
                                   payload->byteLength))
    return false;

  st->imageBufferViews[imageIndex] = gltf_raw_buffer_view_index(&st->accessors, image);
  if (st->imageBufferViews[imageIndex] == GLTF_EXP_INDEX_NONE)
    return false;

  st->imageMimeTypes[imageIndex] = payload->mimeType;

  return st->imageMimeTypes[imageIndex] != NULL;
}

AK_HIDE
bool
gltf_plan_image_buffer_views(GLTFExpState * __restrict st) {
  AkImageExportPayload *payloads;
  GLTFExpIndex         *items;
  const char          **mimeTypes;
  size_t                i;
  size_t                payloadBytes;

  if (st->images.count == 0)
    return true;

  items = gltf_realloc_array(st->imageBufferViews,
                             st->images.count,
                             sizeof(*items));
  if (!items)
    return false;

  st->imageBufferViews = items;

  mimeTypes = gltf_realloc_array(st->imageMimeTypes,
                                 st->images.count,
                                 sizeof(*mimeTypes));
  if (!mimeTypes)
    return false;

  st->imageMimeTypes = mimeTypes;

  payloads = gltf_realloc_array(st->imagePayloads,
                                st->images.count,
                                sizeof(*payloads));
  if (!payloads)
    return false;

  payloadBytes      = st->images.count * sizeof(*payloads);
  st->imagePayloads = payloads;

  memset(st->imagePayloads, 0, payloadBytes);

  for (i = 0; i < st->images.count; i++) {
    AkImage       *image;
    AkImageSource *source;
    const char    *mimeType;

    image  = (AkImage *)st->images.items[i];
    source = gltf_image_source(image);
    st->imageBufferViews[i] = GLTF_EXP_INDEX_NONE;
    st->imageMimeTypes[i]   = NULL;

    mimeType = gltf_image_mime(source);
    st->imageMimeTypes[i] = mimeType;

    if (!gltf_image_source_supported(source) || !mimeType) {
      if (!gltf_plan_image_payload(st, image, i))
        return false;
      continue;
    }

    if (source->type == AK_IMAGE_SOURCE_BUFFER) {
      if (!gltf_accessors_add_raw_view(&st->accessors,
                                       image,
                                       source->buffer->data,
                                       source->buffer->length))
        return false;

      st->imageBufferViews[i] = gltf_raw_buffer_view_index(&st->accessors, image);
      if (st->imageBufferViews[i] == GLTF_EXP_INDEX_NONE)
        return false;

      continue;
    }

    if (st->glb && source->type == AK_IMAGE_SOURCE_URI && mimeType) {
      char        pathbuf[PATH_MAX];
      const char *path;
      struct stat stFile;

      path = gltf_image_source_path(st, source, pathbuf);
      if (!path) {
        if (image->data && !gltf_plan_image_payload(st, image, i))
          return false;
        continue;
      }

      if (stat(path, &stFile) != 0
          || stFile.st_size <= 0
          || (uintmax_t)stFile.st_size > SIZE_MAX)
        return false;

      if (!gltf_accessors_add_file_view(&st->accessors,
                                        image,
                                        path,
                                        (size_t)stFile.st_size))
        return false;

      st->imageBufferViews[i] = gltf_raw_buffer_view_index(&st->accessors, image);
      if (st->imageBufferViews[i] == GLTF_EXP_INDEX_NONE)
        return false;
    }
  }

  return true;
}
