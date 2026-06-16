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
gltf_float_positive(float val) {
  return isfinite(val) && val > 0.0f;
}

AK_HIDE
bool
gltf_float_nonnegative(float val) {
  return isfinite(val) && val >= 0.0f;
}

AK_HIDE
void*
gltf_realloc_array(void *ptr, size_t count, size_t elemSize) {
  if (elemSize != 0 && count > SIZE_MAX / elemSize)
    return NULL;

  return realloc(ptr, count * elemSize);
}

AK_HIDE
bool
gltf_next_capacity(size_t capacity, size_t initial, size_t * __restrict out) {
  if (capacity == 0) {
    *out = initial;
    return initial > 0;
  }

  if (capacity > SIZE_MAX / 2u)
    return false;

  *out = capacity * 2u;

  return true;
}

AK_HIDE
bool
gltf_nodes_reserve(GLTFExpNodeTable * __restrict table, size_t capacity) {
  GLTFExpNodeOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_indices_reserve(GLTFExpIndexList * __restrict list, size_t capacity) {
  GLTFExpIndex *items;

  if (capacity <= list->capacity)
    return true;

  items = gltf_realloc_array(list->items, capacity, sizeof(*items));
  if (!items)
    return false;

  list->items    = items;
  list->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_indices_add(GLTFExpIndexList * __restrict list, GLTFExpIndex index) {
  size_t newCap;

  if (index == GLTF_EXP_INDEX_NONE || list->count >= GLTF_EXP_INDEX_NONE)
    return false;

  if (list->count == list->capacity) {
    if (!gltf_next_capacity(list->capacity, 128, &newCap))
      return false;
    if (!gltf_indices_reserve(list, newCap))
      return false;
  }

  list->items[list->count++] = index;

  return true;
}

AK_HIDE
bool
gltf_scenes_reserve(GLTFExpSceneTable * __restrict table, size_t capacity) {
  GLTFExpSceneOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_materials_reserve(GLTFExpMaterialTable * __restrict table,
                       size_t                            capacity) {
  GLTFExpMaterialOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_materials_add(GLTFExpMaterialTable * __restrict table,
                   AkMaterial           * __restrict material,
                   AkMeshPrimitive      * __restrict prim,
                   AkInstanceGeometry   * __restrict inst) {
  GLTFExpMaterialOut *entry;
  size_t              i;
  size_t              newCap;

  if (!material || gltf_material_is_default_noop(material))
    return true;

  if (table->count >= GLTF_EXP_INDEX_NONE)
    return false;

  for (i = 0; i < table->count; i++) {
    entry = &table->items[i];
    if (entry->material == material
        && entry->primitive == prim
        && entry->instance == inst)
      return true;
  }

  if (table->count == table->capacity) {
    if (!gltf_next_capacity(table->capacity, 64, &newCap))
      return false;
    if (!gltf_materials_reserve(table, newCap))
      return false;
  }

  entry            = &table->items[table->count++];
  entry->material  = material;
  entry->primitive = prim;
  entry->instance  = inst;

  return true;
}

AK_HIDE
bool
gltf_ptrs_reserve(GLTFExpPtrTable * __restrict table, size_t capacity) {
  void **items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

bool
gltf_ptrs_add(GLTFExpPtrTable * __restrict table, void * __restrict ptr) {
  uintptr_t idx;
  size_t    newCap;

  if (!ptr)
    return true;

  if (rb_find(table->map, ptr))
    return true;

  if (table->count >= GLTF_EXP_INDEX_NONE)
    return false;

  if (table->count == table->capacity) {
    if (!gltf_next_capacity(table->capacity, 64, &newCap))
      return false;
    if (!gltf_ptrs_reserve(table, newCap))
      return false;
  }

  idx = (uintptr_t)table->count + 1;
  table->items[table->count++] = ptr;
  rb_insert(table->map, ptr, (void *)idx);

  return true;
}

GLTFExpIndex
gltf_ptrs_index(GLTFExpPtrTable * __restrict table, void * __restrict ptr) {
  uintptr_t idx;

  if (!ptr)
    return GLTF_EXP_INDEX_NONE;

  idx = (uintptr_t)rb_find(table->map, ptr);
  if (idx == 0)
    return GLTF_EXP_INDEX_NONE;

  return (GLTFExpIndex)(idx - 1);
}

AK_HIDE
bool
gltf_strings_reserve(GLTFExpStringTable * __restrict table, size_t capacity) {
  GLTFExpStringOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_strings_add(GLTFExpStringTable * __restrict table,
                 const char         * __restrict name,
                 size_t                          nameLen) {
  size_t i;
  size_t newCap;

  if (!name || nameLen == 0)
    return true;

  for (i = 0; i < table->count; i++) {
    if (table->items[i].nameLen == nameLen
        && memcmp(table->items[i].name, name, nameLen) == 0)
      return true;
  }

  if (table->count == table->capacity) {
    if (!gltf_next_capacity(table->capacity, 16, &newCap))
      return false;
    if (!gltf_strings_reserve(table, newCap))
      return false;
  }

  table->items[table->count].name    = name;
  table->items[table->count].nameLen = nameLen;
  table->count++;

  return true;
}

AK_HIDE
bool
gltf_plan_extra_extensions(GLTFExpState            * __restrict st,
                           AkTreeNode              * __restrict extra,
                           GLTFExtraExtensionSkipFn             skip,
                           void                    * __restrict userdata) {
  AkTreeNode *extensions;
  AkTreeNode *child;

  extensions = gltf_extra_extensions_node(extra);
  for (child = extensions ? extensions->chld : NULL; child; child = child->next) {
    size_t nameLen;

    if (!child->name)
      continue;

    nameLen = strlen(child->name);
    if (skip && skip(child->name, nameLen, userdata))
      continue;

    if (!gltf_strings_add(&st->preservedExtensions, child->name, nameLen))
      return false;
  }

  return true;
}

AK_HIDE
bool
gltf_meshes_reserve(GLTFExpMeshTable * __restrict table, size_t capacity) {
  GLTFExpMeshOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_skins_reserve(GLTFExpSkinTable * __restrict table, size_t capacity) {
  GLTFExpSkinOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_skin_attrs_reserve(GLTFExpSkinAttrTable * __restrict table,
                        size_t                            capacity) {
  GLTFExpSkinAttrOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_skin_attrs_reserve_span(GLTFExpSkinAttrTable * __restrict table,
                             size_t                            count,
                             GLTFExpIndex        * __restrict offset) {
  size_t needed;

  *offset = GLTF_EXP_INDEX_NONE;
  if (count == 0)
    return true;

  if (table->count >= GLTF_EXP_INDEX_NONE
      || count > (size_t)GLTF_EXP_INDEX_NONE - table->count)
    return false;

  needed = table->count + count;
  if (!gltf_skin_attrs_reserve(table, needed))
    return false;

  *offset = (GLTFExpIndex)table->count;
  memset(&table->items[table->count], 0, sizeof(*table->items) * count);
  for (size_t i = 0; i < count; i++) {
    table->items[table->count + i].jointsAccessorIndex  = GLTF_EXP_INDEX_NONE;
    table->items[table->count + i].weightsAccessorIndex = GLTF_EXP_INDEX_NONE;
  }
  table->count = needed;

  return true;
}

AK_HIDE
bool
gltf_morph_attrs_reserve(GLTFExpMorphAttrTable * __restrict table,
                         size_t                             capacity) {
  GLTFExpMorphAttrOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_morph_attrs_reserve_span(GLTFExpMorphAttrTable * __restrict table,
                              size_t                             count,
                              GLTFExpIndex         * __restrict offset) {
  size_t needed;

  *offset = GLTF_EXP_INDEX_NONE;
  if (count == 0)
    return true;

  if (table->count >= GLTF_EXP_INDEX_NONE
      || count > (size_t)GLTF_EXP_INDEX_NONE - table->count)
    return false;

  needed = table->count + count;
  if (!gltf_morph_attrs_reserve(table, needed))
    return false;

  *offset = (GLTFExpIndex)table->count;
  memset(&table->items[table->count], 0, sizeof(*table->items) * count);
  for (size_t i = 0; i < count; i++) {
    table->items[table->count + i].positionAccessorIndex = GLTF_EXP_INDEX_NONE;
    table->items[table->count + i].normalAccessorIndex   = GLTF_EXP_INDEX_NONE;
    table->items[table->count + i].tangentAccessorIndex  = GLTF_EXP_INDEX_NONE;
  }
  table->count = needed;

  return true;
}

AK_HIDE
bool
gltf_position_attrs_reserve(GLTFExpPositionAttrTable * __restrict table,
                            size_t                                capacity) {
  GLTFExpPositionAttrOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
GLTFExpPositionAttrOut*
gltf_position_attrs_add(GLTFExpPositionAttrTable * __restrict table,
                        AkMeshPrimitive          * __restrict prim) {
  GLTFExpPositionAttrOut *entry;
  size_t                  newCap;

  if (!prim)
    return NULL;

  if (table->count == table->capacity) {
    if (!gltf_next_capacity(table->capacity, 16, &newCap))
      return NULL;
    if (!gltf_position_attrs_reserve(table, newCap))
      return NULL;
  }

  entry = &table->items[table->count++];
  memset(entry, 0, sizeof(*entry));
  entry->primitive     = prim;
  entry->accessorIndex = GLTF_EXP_INDEX_NONE;

  return entry;
}

AK_HIDE
bool
gltf_baked_attrs_reserve(GLTFExpBakedAttrTable * __restrict table,
                         size_t                             capacity) {
  GLTFExpBakedPrimAttrOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
GLTFExpBakedPrimAttrOut*
gltf_baked_attrs_add(GLTFExpBakedAttrTable * __restrict table,
                     AkNode                * __restrict node,
                     AkMeshPrimitive       * __restrict prim) {
  GLTFExpBakedPrimAttrOut *entry;
  size_t                   newCap;

  if (!node || !prim)
    return NULL;

  if (table->count == table->capacity) {
    if (!gltf_next_capacity(table->capacity, 16, &newCap))
      return NULL;
    if (!gltf_baked_attrs_reserve(table, newCap))
      return NULL;
  }

  entry = &table->items[table->count++];
  memset(entry, 0, sizeof(*entry));
  entry->node                  = node;
  entry->primitive             = prim;
  entry->positionAccessorIndex = GLTF_EXP_INDEX_NONE;
  entry->normalAccessorIndex   = GLTF_EXP_INDEX_NONE;

  return entry;
}

AK_HIDE
bool
gltf_anims_reserve(GLTFExpAnimTable * __restrict table, size_t capacity) {
  GLTFExpAnimOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_anim_samplers_reserve(GLTFExpAnimSamplerTable * __restrict table,
                           size_t                               capacity) {
  GLTFExpAnimSamplerOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
bool
gltf_anim_channels_reserve(GLTFExpAnimChannelTable * __restrict table,
                           size_t                               capacity) {
  GLTFExpAnimChannelOut *items;

  if (capacity <= table->capacity)
    return true;

  items = gltf_realloc_array(table->items, capacity, sizeof(*items));
  if (!items)
    return false;

  table->items    = items;
  table->capacity = capacity;

  return true;
}

AK_HIDE
void*
gltf_mesh_key(AkGeometry         * __restrict geom,
              AkInstanceGeometry * __restrict inst) {
  return inst && (inst->objectBindings || inst->morpher || inst->skinner)
         ? (void *)inst
         : (void *)geom;
}

AK_HIDE
GLTFExpIndex
gltf_mesh_index(GLTFExpMeshTable * __restrict table,
                void             * __restrict key) {
  uintptr_t idx;

  if (!key)
    return GLTF_EXP_INDEX_NONE;

  idx = (uintptr_t)rb_find(table->map, key);
  if (idx == 0)
    return GLTF_EXP_INDEX_NONE;

  return (GLTFExpIndex)(idx - 1);
}

AK_HIDE
bool
gltf_meshes_add(GLTFExpMeshTable  * __restrict table,
                void              * __restrict key,
                AkGeometry        * __restrict geom,
                AkInstanceGeometry * __restrict inst,
                AkNode            * __restrict bakeNode,
                GLTFExpIndex                    skinAttrOffset,
                uint32_t                        skinAttrCount,
                GLTFExpIndex                    morphAttrOffset,
                uint32_t                        morphAttrCount,
                uint32_t                        morphAttrPrimCount) {
  uintptr_t idx;
  size_t    newCap;

  if (!key)
    return true;

  if (rb_find(table->map, key))
    return true;

  if (table->count >= GLTF_EXP_INDEX_NONE)
    return false;

  if (table->count == table->capacity) {
    if (!gltf_next_capacity(table->capacity, 64, &newCap))
      return false;
    if (!gltf_meshes_reserve(table, newCap))
      return false;
  }

  idx = (uintptr_t)table->count + 1;
  table->items[table->count].geom           = geom;
  table->items[table->count].instance       = inst;
  table->items[table->count].bakeNode       = bakeNode;
  table->items[table->count].skinAttrOffset = skinAttrOffset;
  table->items[table->count].skinAttrCount  = skinAttrCount;
  table->items[table->count].morphAttrOffset = morphAttrOffset;
  table->items[table->count].morphAttrCount  = morphAttrCount;
  table->items[table->count].morphAttrPrimCount = morphAttrPrimCount;
  table->count++;
  rb_insert(table->map, key, (void *)idx);

  return true;
}

AK_HIDE
void*
gltf_skin_key(AkInstanceSkin * __restrict skinner) {
  if (!skinner || !skinner->skin)
    return NULL;

  return skinner->overrideJoints ? (void *)skinner : (void *)skinner->skin;
}

AK_HIDE
size_t
gltf_component_type_size(AkTypeId type) {
  switch (type) {
    case AKT_BYTE:
    case AKT_UBYTE:
      return 1;
    case AKT_SHORT:
    case AKT_USHORT:
      return 2;
    case AKT_INT:
    case AKT_UINT:
    case AKT_FLOAT:
      return 4;
    case AKT_DOUBLE:
    case AKT_INT64:
    case AKT_UINT64:
      return 8;
    case AKT_HALF:
      return 2;
    default:
      break;
  }

  return 0;
}

AK_HIDE
uint32_t
gltf_component_size_count(AkComponentSize componentSize) {
  switch (componentSize) {
    case AK_COMPONENT_SIZE_SCALAR: return 1;
    case AK_COMPONENT_SIZE_VEC2:   return 2;
    case AK_COMPONENT_SIZE_VEC3:   return 3;
    case AK_COMPONENT_SIZE_VEC4:   return 4;
    case AK_COMPONENT_SIZE_MAT2:   return 4;
    case AK_COMPONENT_SIZE_MAT3:   return 9;
    case AK_COMPONENT_SIZE_MAT4:   return 16;
    default:
      break;
  }

  return 0;
}
