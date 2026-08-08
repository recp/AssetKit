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

#ifndef ak_src_image_export_h
#define ak_src_image_export_h

#include "../common.h"

typedef struct AkImageExportRequest {
  AkDoc      *doc;
  AkImage    *image;
  const char *targetMimeType;
} AkImageExportRequest;

typedef struct AkImageExportPayload {
  void       *data;
  size_t      byteLength;
  const char *mimeType;
} AkImageExportPayload;

AK_HIDE
bool
ak_imageCanLoad(AkImage * __restrict image);

AK_INLINE
AkImageSource*
ak_imageSource(AkImage * __restrict image) {
  if (!image)
    return NULL;

  if (image->source)
    return image->source;

  return image->image ? image->image->source : NULL;
}

AK_HIDE
bool
ak_imageExportPNG(AkImageExportRequest * __restrict req,
                  AkImageExportPayload * __restrict payload);

/* Copies an already encoded PNG or JPEG source without decoding it. */
AK_HIDE
bool
ak_imageExportPreserved(AkImageExportRequest * __restrict req,
                        AkImageExportPayload * __restrict payload);

AK_HIDE
void
ak_imageExportPayloadRelease(AkImageExportPayload * __restrict payload);

#endif /* ak_src_image_export_h */
