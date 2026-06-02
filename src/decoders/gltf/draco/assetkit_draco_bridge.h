/*
 * Copyright (C) 2026 Recep Aslantas
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

#ifndef assetkit_draco_bridge_h
#define assetkit_draco_bridge_h

#include <ak/assetkit.h>

#ifdef __cplusplus
extern "C" {
#endif

struct AkGLTFState;
struct json_t;

typedef struct AkDracoBufferView {
  AkBuffer *buffer;
  size_t    byteOffset;
  size_t    byteLength;
} AkDracoBufferView;

AkHeap*
ak_draco_gltf_heap(struct AkGLTFState *gst);

AkDoc*
ak_draco_gltf_doc(struct AkGLTFState *gst);

AkAccessor*
ak_draco_gltf_accessor_at(struct AkGLTFState *gst, int32_t index);

bool
ak_draco_gltf_buffer_view_at(struct AkGLTFState *gst,
                             int32_t             index,
                             AkDracoBufferView  *out);

void
ak_draco_gltf_prepend_buffer(struct AkGLTFState *gst, AkBuffer *buffer);

const struct json_t*
ak_draco_json_get(const struct json_t *object, const char *key);

const struct json_t*
ak_draco_json_get_len(const struct json_t *object,
                      const char          *key,
                      size_t               keysize);

int32_t
ak_draco_json_int32(const struct json_t *object, int32_t def);

const struct json_t*
ak_draco_json_first_child(const struct json_t *object);

const struct json_t*
ak_draco_json_next(const struct json_t *object);

const char*
ak_draco_json_key(const struct json_t *object);

size_t
ak_draco_json_keysize(const struct json_t *object);

#ifdef __cplusplus
}
#endif

#endif /* assetkit_draco_bridge_h */
