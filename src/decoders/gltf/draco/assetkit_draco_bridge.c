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

#include "assetkit_draco_bridge.h"
#include "../../../io/gltf/common.h"

AkHeap*
ak_draco_gltf_heap(struct AkGLTFState *gst) {
  return gst ? gst->heap : NULL;
}

AkDoc*
ak_draco_gltf_doc(struct AkGLTFState *gst) {
  return gst ? gst->doc : NULL;
}

AkAccessor*
ak_draco_gltf_accessor_at(struct AkGLTFState *gst, int32_t index) {
  return gst ? gltf_accessor_at(gst, index) : NULL;
}

bool
ak_draco_gltf_buffer_view_at(struct AkGLTFState *gst,
                             int32_t             index,
                             AkDracoBufferView  *out) {
  AkBufferView *bv;

  if (!gst || !out || !(bv = gltf_bufferView_at(gst, index)))
    return false;

  out->buffer     = bv->buffer;
  out->byteOffset = bv->byteOffset;
  out->byteLength = bv->byteLength;

  return true;
}

void
ak_draco_gltf_prepend_buffer(struct AkGLTFState *gst, AkBuffer *buffer) {
  if (!gst || !gst->doc || !buffer)
    return;

  AK_LIB_PREPEND(gst->doc->lib.buffers, buffer, next);
}

const struct json_t*
ak_draco_json_get(const struct json_t *object, const char *key) {
  return json_get(object, key);
}

const struct json_t*
ak_draco_json_get_len(const struct json_t *object,
                      const char          *key,
                      size_t               keysize) {
  return gltf_jsonGetLen(object, key, keysize);
}

int32_t
ak_draco_json_int32(const struct json_t *object, int32_t def) {
  return json_int32(object, def);
}

const struct json_t*
ak_draco_json_first_child(const struct json_t *object) {
  if (!object || object->type != JSON_OBJECT)
    return NULL;

  return object->value;
}

const struct json_t*
ak_draco_json_next(const struct json_t *object) {
  return object ? object->next : NULL;
}

const char*
ak_draco_json_key(const struct json_t *object) {
  return object ? object->key : NULL;
}

size_t
ak_draco_json_keysize(const struct json_t *object) {
  return object && object->keysize > 0 ? (size_t)object->keysize : 0;
}
