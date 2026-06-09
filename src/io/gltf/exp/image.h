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

#ifndef assetkit_gltf_exp_image_h
#define assetkit_gltf_exp_image_h

#include "common.h"

#include <stdbool.h>

AkImageSource*
gltf_image_source(AkImage * __restrict image);

bool
gltf_image_source_supported(AkImageSource * __restrict source);

bool
gltf_image_exportable(GLTFExpState * __restrict st,
                      AkImage      * __restrict image);

bool
gltf_image_uri_is_data(const char * __restrict uri);

bool
gltf_image_uri_has_scheme(const char * __restrict uri);

bool
gltf_image_uri_is_file_scheme(const char * __restrict uri);

bool
gltf_image_path_is_abs(const char * __restrict path);

const char*
gltf_image_source_path(GLTFExpState  * __restrict st,
                       AkImageSource * __restrict source,
                       char          * __restrict pathbuf);

bool
gltf_image_mime_supported(const char * __restrict mimeType);

const char*
gltf_image_mime(AkImageSource * __restrict source);

bool
gltf_image_mime_or_uri_is(AkImageSource * __restrict source,
                          const char    * __restrict mimeType,
                          size_t                     mimeTypeLen,
                          const char    * __restrict ext,
                          size_t                     extLen);

bool
gltf_image_prepare_export_uris(GLTFExpState * __restrict st);

bool
gltf_image_copy_export_uri(GLTFExpState  * __restrict st,
                           AkImageSource * __restrict source,
                           const char    * __restrict exportUri);

#endif /* assetkit_gltf_exp_image_h */
