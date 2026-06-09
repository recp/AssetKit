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
gltf_primitive_has_exportable_skin_inputs(AkMeshPrimitive * __restrict prim,
                                          AkInput         * __restrict posInput) {
  AkInput *input;
  bool     hasJoint;
  bool     hasWeight;

  hasJoint  = false;
  hasWeight = false;

  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if (!input->accessor
        || !gltf_input_supported(input)
        || !gltf_input_count_valid(prim, input, posInput))
      continue;

    if (input->semantic == AK_INPUT_JOINT)
      hasJoint = true;
    else if (input->semantic == AK_INPUT_WEIGHT)
      hasWeight = true;
  }

  return hasJoint && hasWeight;
}

AK_HIDE
bool
gltf_mesh_needs_generated_skin_attrs(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;

  for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next) {
    AkInput *posInput;

    posInput = gltf_primitive_position_input(prim);
    if (!gltf_primitive_has_exportable_skin_inputs(prim, posInput))
      return true;
  }

  return false;
}

AK_HIDE
bool
gltf_skin_attr_empty(GLTFExpSkinAttrOut * __restrict attr) {
  return !attr || attr->jointsAccessorIndex == GLTF_EXP_INDEX_NONE
         || attr->weightsAccessorIndex == GLTF_EXP_INDEX_NONE;
}

AK_HIDE
bool
gltf_plan_generated_skin_attrs(GLTFExpState       * __restrict st,
                               AkInstanceSkin     * __restrict skinner,
                               AkMeshPrimitive    * __restrict prim,
                               AkInput            * __restrict posInput,
                               uint32_t                         primIndex,
                               GLTFExpSkinAttrOut * __restrict attr) {
  AkSkin   *skin;
  void     *data;
  uint16_t *joints;
  float    *weights;
  size_t    vertexCount;
  size_t    jointBytes;
  size_t    weightBytes;

  if (!attr)
    return false;

  attr->data                 = NULL;
  attr->joints               = NULL;
  attr->weights              = NULL;
  attr->jointsAccessorIndex  = GLTF_EXP_INDEX_NONE;
  attr->weightsAccessorIndex = GLTF_EXP_INDEX_NONE;
  attr->vertexCount          = 0;

  if (!skinner || !(skin = skinner->skin) || !prim || !posInput || !posInput->accessor)
    return false;

  vertexCount = posInput->accessor->count;
  if (vertexCount == 0
      || vertexCount > UINT32_MAX
      || vertexCount > (size_t)-1 / (sizeof(uint16_t) * 4u)
      || vertexCount > (size_t)-1 / (sizeof(float) * 4u))
    return false;

  jointBytes  = vertexCount * sizeof(uint16_t) * 4u;
  weightBytes = vertexCount * sizeof(float) * 4u;
  if (jointBytes > (size_t)-1 - weightBytes)
    return false;

  data = malloc(jointBytes + weightBytes);
  if (!data)
    return false;

  joints  = data;
  weights = (float *)((char *)data + jointBytes);

  if (ak_skinFillWeights(skin,
                         prim,
                         primIndex,
                         4,
                         joints,
                         weights) != vertexCount) {
    free(data);
    return false;
  }

  if (!gltf_accessors_add_raw_target(&st->accessors,
                                     joints,
                                     joints,
                                     jointBytes,
                                     (uint32_t)vertexCount,
                                     AKT_USHORT,
                                     AK_COMPONENT_SIZE_VEC4,
                                     4,
                                     GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY)
      || !gltf_accessors_add_raw_target(&st->accessors,
                                        weights,
                                        weights,
                                        weightBytes,
                                        (uint32_t)vertexCount,
                                        AKT_FLOAT,
                                        AK_COMPONENT_SIZE_VEC4,
                                        4,
                                        GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY)) {
    free(data);
    return false;
  }

  attr->data                 = data;
  attr->joints               = joints;
  attr->weights              = weights;
  attr->jointsAccessorIndex  = gltf_raw_accessor_index(&st->accessors, joints);
  attr->weightsAccessorIndex = gltf_raw_accessor_index(&st->accessors, weights);
  attr->vertexCount          = (uint32_t)vertexCount;

  return !gltf_skin_attr_empty(attr);
}

AK_HIDE
AkNode**
gltf_skin_joints(AkInstanceSkin * __restrict skinner, size_t * __restrict count) {
  AkSkin *skin;

  *count = 0;
  if (!skinner || !(skin = skinner->skin))
    return NULL;

  *count = skin->nJoints;
  return skinner->overrideJoints ? skinner->overrideJoints : skin->joints;
}

AK_HIDE
bool
gltf_skin_valid(AkInstanceSkin * __restrict skinner) {
  AkNode **joints;
  size_t   count;
  size_t   i;

  joints = gltf_skin_joints(skinner, &count);

  if (!skinner || !skinner->skin || !joints || count == 0 || count > UINT32_MAX)
    return false;

  for (i = 0; i < count; i++) {
    if (!joints[i])
      return false;
  }

  return true;
}

AK_HIDE
AkNode*
gltf_skin_unscened_joint_root(AkNode * __restrict node) {
  void   *parent;
  AkNode *root;

  if (!node)
    return NULL;

  root = node;
  while (root->parent)
    root = root->parent;

  parent = ak_mem_parent(root);
  if (parent && ak_typeid(parent) == AKT_SCENE)
    return NULL;

  return root;
}

AK_HIDE
bool
gltf_collect_skin_joint_roots(GLTFExpState   * __restrict st,
                              AkInstanceSkin * __restrict skinner) {
  AkNode **joints;
  size_t   count;
  size_t   i;

  joints = gltf_skin_joints(skinner, &count);
  if (!joints || count == 0)
    return false;

  for (i = 0; i < count; i++) {
    AkNode *root;

    root = gltf_skin_unscened_joint_root(joints[i]);
    if (!root)
      continue;

    if (!rb_find(st->nodeMap, root)
        && !gltf_ptrs_add(&st->skinJointRoots, root))
      return false;
    if (!gltf_ptrs_add(&st->sceneSkinJointRoots, root))
      return false;
  }

  return true;
}

AK_HIDE
bool
gltf_plan_deferred_skin_joint_roots(GLTFExpState * __restrict st) {
  size_t i;

  for (i = 0; i < st->skinJointRoots.count; i++) {
    AkNode *root;

    root = st->skinJointRoots.items[i];
    if (!root || rb_find(st->nodeMap, root))
      continue;

    if (gltf_plan_node(st, root, NULL) == GLTF_EXP_INDEX_NONE || st->failed)
      return false;
  }

  return true;
}

AK_HIDE
bool
gltf_skins_add(GLTFExpState    * __restrict st,
               AkInstanceSkin  * __restrict skinner,
               GLTFExpIndex    * __restrict skinIndex) {
  GLTFExpSkinTable *table;
  GLTFExpSkinOut   *out;
  AkSkin           *skin;
  void             *key;
  uintptr_t         idx;
  size_t            newCap;

  *skinIndex = GLTF_EXP_INDEX_NONE;

  if (!gltf_skin_valid(skinner))
    return false;

  if (!gltf_collect_skin_joint_roots(st, skinner))
    return false;

  table = &st->skins;
  skin  = skinner->skin;
  key   = gltf_skin_key(skinner);
  idx   = (uintptr_t)rb_find(table->map, key);
  if (idx > 0) {
    *skinIndex = (GLTFExpIndex)(idx - 1);
    return true;
  }

  if (table->count >= GLTF_EXP_INDEX_NONE)
    return false;

  if (table->count == table->capacity) {
    if (!gltf_next_capacity(table->capacity, 16, &newCap))
      return false;
    if (!gltf_skins_reserve(table, newCap))
      return false;
  }

  idx = (uintptr_t)table->count + 1;
  out = &table->items[table->count];
  memset(out, 0, sizeof(*out));
  out->skin                     = skin;
  out->instance                 = skinner;
  out->inverseBindAccessorIndex = GLTF_EXP_INDEX_NONE;

  if (skin->invBindPoses) {
    if (skin->nJoints > SIZE_MAX / sizeof(AkFloat4x4))
      return false;

    if (!gltf_accessors_add_raw(&st->accessors,
                                skin->invBindPoses,
                                skin->invBindPoses,
                                skin->nJoints * sizeof(AkFloat4x4),
                                (uint32_t)skin->nJoints,
                                AKT_FLOAT,
                                AK_COMPONENT_SIZE_MAT4,
                                16))
      return false;

    out->inverseBindAccessorIndex = gltf_raw_accessor_index(&st->accessors,
                                                            skin->invBindPoses);
    if (out->inverseBindAccessorIndex == GLTF_EXP_INDEX_NONE)
      return false;
  }

  table->count++;
  rb_insert(table->map, key, (void *)idx);
  *skinIndex = (GLTFExpIndex)(idx - 1);

  return true;
}

GLTFExpIndex
gltf_skin_index(GLTFExpState * __restrict st,
                AkInstanceSkin * __restrict skinner) {
  uintptr_t idx;
  void     *key;

  key = gltf_skin_key(skinner);
  if (!key)
    return GLTF_EXP_INDEX_NONE;

  idx = (uintptr_t)rb_find(st->skins.map, key);
  if (idx == 0)
    return GLTF_EXP_INDEX_NONE;

  return (GLTFExpIndex)(idx - 1);
}

AK_HIDE
bool
gltf_camera_supported(AkCamera * __restrict camera) {
  AkProjection *proj;

  if (!camera || !camera->optics || !camera->optics->proj)
    return false;

  proj = camera->optics->proj;
  switch (proj->type) {
    case AK_PROJECTION_PERSPECTIVE: {
      AkPerspective *persp;

      persp = (AkPerspective *)proj;
      if (!gltf_float_positive(persp->yfov)
          || persp->yfov >= GLTF_EXP_PI
          || !gltf_float_positive(persp->znear)
          || (persp->aspectRatio != 0.0f
              && !gltf_float_positive(persp->aspectRatio))
          || (persp->zfar != 0.0f
              && (!gltf_float_positive(persp->zfar)
                  || persp->zfar <= persp->znear)))
        return false;
      break;
    }
    case AK_PROJECTION_ORTHOGRAPHIC: {
      AkOrthographic *ortho;

      ortho = (AkOrthographic *)proj;
      if (!gltf_float_positive(ortho->xmag)
          || !gltf_float_positive(ortho->ymag)
          || !gltf_float_positive(ortho->znear)
          || !gltf_float_positive(ortho->zfar)
          || ortho->zfar <= ortho->znear)
        return false;
      break;
    }
    default:
      return false;
  }

  return true;
}

AK_HIDE
bool
gltf_plan_camera(GLTFExpState * __restrict st,
                 AkCamera     * __restrict camera) {
  if (!gltf_camera_supported(camera))
    return false;

  return gltf_ptrs_add(&st->cameras, camera)
         && gltf_plan_extra_extensions(st, ak_extra(camera), NULL, NULL);
}

AK_HIDE
bool
gltf_light_supported(AkLight * __restrict light) {
  AkLightBase *base;

  if (!light || !light->data)
    return false;

  base = light->data;
  if (!gltf_float_nonnegative(base->color.rgba.R)
      || !gltf_float_nonnegative(base->color.rgba.G)
      || !gltf_float_nonnegative(base->color.rgba.B)
      || !gltf_float_nonnegative(base->intensity)
      || (base->range != 0.0f && !gltf_float_positive(base->range)))
    return false;

  switch (base->type) {
    case AK_LIGHT_TYPE_DIRECTIONAL:
    case AK_LIGHT_TYPE_POINT:
      break;
    case AK_LIGHT_TYPE_SPOT: {
      AkSpotLight *spot;

      spot = (AkSpotLight *)base;
      if (!gltf_float_nonnegative(spot->innerConeAngle)
          || !gltf_float_positive(spot->outerConeAngle)
          || spot->innerConeAngle >= spot->outerConeAngle
          || spot->outerConeAngle > GLTF_EXP_HALF_PI)
        return false;
      break;
    }
    default:
      return false;
  }

  return true;
}

AK_HIDE
bool
gltf_plan_light(GLTFExpState * __restrict st,
                AkLight      * __restrict light) {
  if (!gltf_light_supported(light))
    return false;

  return gltf_ptrs_add(&st->lights, light)
         && gltf_plan_extra_extensions(st, ak_extra(light), NULL, NULL);
}
