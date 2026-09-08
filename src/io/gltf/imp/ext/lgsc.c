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

#include "lgsc.h"
#include "../../../lgsc/lgsc.h"

static
bool
gltf_lgsc_uint(const json_t *node, uint32_t *value) {
  const char *p, *end;
  uint32_t    n, digit;

  if (!(p = json__num_begin(node, &end)) || p == end)
    return false;

  n = 0;

  for (; p < end; p++) {
    digit = (uint32_t)(*p - '0');
    if (digit > 9 || n > (UINT32_MAX - digit) / 10)
      return false;

    n = n * 10 + digit;
  }

  *value = n;
  return true;
}

static
int
gltf_lgsc_slot(const json_t *attribute) {
  const char *name;
  size_t      size;
  uint32_t    degree, coefficient;

  name = attribute->key;
  size = (size_t)attribute->keysize;

  if (size == 8 && !memcmp(name, "POSITION", 8))
    return 0;

  if (size <= sizeof("KHR_gaussian_splatting:") - 1
      || memcmp(name, "KHR_gaussian_splatting:", sizeof("KHR_gaussian_splatting:") - 1))
    return -1;

  name += sizeof("KHR_gaussian_splatting:") - 1;
  size -= sizeof("KHR_gaussian_splatting:") - 1;

  if (size == 8 && !memcmp(name, "ROTATION", 8))
    return 1;
  if (size == 5 && !memcmp(name, "SCALE", 5))
    return 2;
  if (size == 7 && !memcmp(name, "OPACITY", 7))
    return 3;

  if (size != 18 || memcmp(name, "SH_DEGREE_", 10) || memcmp(name + 11, "_COEF_", 6))
    return -1;

  degree      = (uint32_t)(name[10] - '0');
  coefficient = (uint32_t)(name[17] - '0');

  return degree <= 3 && coefficient <= degree * 2 ? (int)(4 + degree * degree + coefficient) : -1;
}

static
bool
gltf_lgsc_declared(const json_t *array) {
  const json_array_t *list;
  const json_t       *item;

  if (!(list = json_array(array)))
    return false;

  for (item = list->base.value; item; item = item->next)
    if (item->type == JSON_STRING && item->valsize == sizeof(AK_GLTF_LGSC_EXT) - 1
        && !memcmp(json_string(item), AK_GLTF_LGSC_EXT, sizeof(AK_GLTF_LGSC_EXT) - 1))
      return true;

  return false;
}

AK_HIDE
bool
gltf_lgsc_prepare(AkGLTFState *gst, const json_t *root) {
  const json_array_t *meshes, *primitives, *accessors, *views;
  const json_t       *mesh, *primitive, *extension, *mapping, *item, *base;
  uint32_t           index, baseIndex, degree, mask;
  int                slot;

  if (!gltf_lgsc_declared(GLTF_JSON_GET(root, extensionsUsed))
      && !gltf_lgsc_declared(GLTF_JSON_GET(root, extensionsRequired)))
    return true;

  if (!(accessors = json_array(GLTF_JSON_GET(root, accessors))) || accessors->count <= 0
      || !(meshes = json_array(GLTF_JSON_GET8(root, meshes)))
      || !(views = json_array(GLTF_JSON_GET(root, bufferViews))) || views->count <= 0
      || !(gst->lgscAccessors = ak_heap_calloc(gst->heap, gst->tmpParent, (size_t)accessors->count))
      || !(gst->lgscDegrees = ak_heap_calloc(gst->heap, gst->tmpParent, (size_t)views->count)))
    return false;

  for (mesh = meshes->base.value; mesh; mesh = mesh->next) {
    if (!(primitives = json_array(GLTF_JSON_GET(mesh, primitives))))
      continue;

    for (primitive = primitives->base.value; primitive; primitive = primitive->next) {
      extension = GLTF_JSON_GET(GLTF_JSON_GET(primitive, extensions), KHR_gaussian_splatting);
      extension = gltf_jsonGetLen(GLTF_JSON_GET(extension, extensions),
                                   AK_GLTF_LGSC_EXT, sizeof(AK_GLTF_LGSC_EXT) - 1);
      if (!extension)
        continue;

      mapping = GLTF_JSON_GET(extension, attributes);
      base    = GLTF_JSON_GET(primitive, attributes);
      mask    = 0;

      if (!mapping || mapping->type != JSON_OBJECT || !base || base->type != JSON_OBJECT
          || !gltf_lgsc_uint(gltf_jsonGetLen(extension, "shDegree", 8), &degree) || degree > 3
          || !gltf_lgsc_uint(GLTF_JSON_GET(extension, bufferView), &index) || index >= (uint32_t)views->count)
        return false;

      if (gst->lgscDegrees[index] < degree + 1)
        gst->lgscDegrees[index] = (uint8_t)(degree + 1);

      for (item = mapping->value; item; item = item->next) {
        slot = gltf_lgsc_slot(item);

        if (slot < 0 || (uint32_t)slot >= 4 + (degree + 1) * (degree + 1)
            || (mask & (1u << slot))
            || !gltf_lgsc_uint(item, &index) || index >= (uint32_t)accessors->count
            || !gltf_lgsc_uint(gltf_jsonGetLen(base, item->key, (size_t)item->keysize), &baseIndex)
            || index != baseIndex
            || (gst->lgscAccessors[index] && gst->lgscAccessors[index] != slot + 1))
          return false;

        gst->lgscAccessors[index] = (uint8_t)(slot + 1);
        mask |= 1u << slot;
      }

      if ((mask & 31u) != 31u)
        return false;
    }
  }

  return true;
}

AK_HIDE
bool
gltf_lgsc_primitive(AkGLTFState *gst, AkMeshPrimitive *prim, const json_t *extension) {
  const json_t    *mapping, *item, *option;
  AkBufferView    *view;
  AkAccessor      *accessors[20], *acc;
  AkLGSCData      *data;
  uint32_t        index, viewIndex, count, degree, slot, components;
  int             field;

  if (!gst->lgscAccessors || extension->type != JSON_OBJECT
      || !gltf_lgsc_uint(GLTF_JSON_GET(extension, bufferView), &viewIndex) || viewIndex > INT32_MAX
      || !(view = gltf_bufferView_at(gst, (int32_t)viewIndex))
      || !view->buffer || !view->buffer->data || !view->byteLength
      || view->byteOffset > view->buffer->length
      || view->byteLength > view->buffer->length - view->byteOffset
      || !gltf_lgsc_uint(gltf_jsonGetLen(extension, "numPoints", 9), &count) || !count
      || !gltf_lgsc_uint(gltf_jsonGetLen(extension, "shDegree", 8), &degree) || degree > 3)
    return false;

  option = gltf_jsonGetLen(extension, "compLevel", 9);
  if (option && (!gltf_lgsc_uint(option, &index) || index > 2))
    return false;

  option = gltf_jsonGetLen(extension, "flags", 5);
  if (option && (!gltf_lgsc_uint(option, &index) || index))
    return false;

  memset(accessors, 0, sizeof(accessors));
  mapping = GLTF_JSON_GET(extension, attributes);

  for (item = mapping->value; item; item = item->next) {
    field = gltf_lgsc_slot(item);

    if (field < 0 || !gltf_lgsc_uint(item, &index) || index > INT32_MAX
        || !(acc = gltf_accessor_at(gst, (int32_t)index)))
      return false;

    slot       = (uint32_t)field;
    components = slot == 1 ? 4 : slot == 3 ? 1 : 3;

    if (acc->componentType != AKT_FLOAT || acc->componentCount != components
        || acc->componentSize != (AkComponentSize)components
        || acc->count != count || acc->normalized)
      return false;

    accessors[slot] = acc;
  }

  if (!gst->lgscBuffers && !(gst->lgscBuffers = rb_newtree_ptr()))
    return false;

  if (!(data = rb_find(gst->lgscBuffers, view))) {
    if (!(data = ak_heap_calloc(gst->heap, gst->tmpParent, sizeof(*data))))
      return false;

    /* Decode each view once at the highest degree its consumers request.
       The container supplies glTF coordinates; standalone .lgsc uses RDF. */
    if (!lgsc_decode(gst->heap, gst->doc,
                       (const uint8_t *)view->buffer->data + view->byteOffset,
                       view->byteLength, gst->lgscDegrees[viewIndex] - 1, count, false, data))
      return false;

    AK_LIB_PREPEND(gst->doc->lib.buffers, data->buffer, next);
    rb_insert(gst->lgscBuffers, view, data);
  }

  if (data->count != count)
    return false;

  for (slot = 0; slot < 4 + (degree + 1) * (degree + 1); slot++) {
    if (!(acc = accessors[slot]))
      continue;

    /* Shared accessor identities must not be rebound to another payload. */
    if (acc->buffer && (acc->buffer != data->buffer || acc->byteOffset != data->offsets[slot]))
      return false;

    lgsc_accessor(acc, data, slot);
  }

  prim->gsplat->decodedCount = count;
  prim->gsplat->shDegree     = (uint8_t)degree;
  prim->nPolygons            = count;
  return true;
}
