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

#include "skin.h"
#include "plan.h"
#include "../strpool.h"

static
AkNode**
gltf_skin_joints(GLTFExpSkinOut * __restrict out, size_t * __restrict count) {
  AkSkin *skin;

  *count = 0;
  if (!out || !(skin = out->skin))
    return NULL;

  *count = skin->nJoints;
  return out->instance && out->instance->overrideJoints
         ? out->instance->overrideJoints
         : skin->joints;
}

static
bool
gltf_node_is_ancestor(AkNode * __restrict ancestor,
                      AkNode * __restrict node) {
  while (node) {
    if (node == ancestor)
      return true;

    node = node->parent;
  }

  return false;
}

static
bool
gltf_skin_skeleton_valid(GLTFExpSkinOut * __restrict out) {
  AkNode **joints;
  AkNode  *skeleton;
  size_t   count;
  size_t   i;

  skeleton = out && out->instance && out->instance->overrideSkeleton
               ? out->instance->overrideSkeleton
               : (out && out->skin ? out->skin->skeleton : NULL);
  if (!skeleton)
    return false;

  joints = gltf_skin_joints(out, &count);
  if (!joints || count == 0)
    return false;

  for (i = 0; i < count; i++) {
    if (!gltf_node_is_ancestor(skeleton, joints[i]))
      return false;
  }

  return true;
}

static
bool
gltf_write_skin_joints(GLTFExpWriter  * __restrict w,
                       GLTFExpState   * __restrict st,
                       GLTFExpSkinOut * __restrict out) {
  AkNode **joints;
  size_t   count;
  size_t   i;

  joints = gltf_skin_joints(out, &count);
  if (!joints || count == 0)
    return false;

  gltf_w_key(w, _s_gltf_joints, _s_gltf_joints_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < count; i++) {
    GLTFExpIndex nodeIndex;

    nodeIndex = gltf_node_index(st, joints[i]);
    if (nodeIndex == GLTF_EXP_INDEX_NONE)
      return false;

    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_w_uint(w, nodeIndex);
  }

  gltf_w_ch(w, ']');

  return true;
}

static
void
gltf_write_skin(GLTFExpWriter  * __restrict w,
                GLTFExpState   * __restrict st,
                GLTFExpSkinOut * __restrict out) {
  bool comma;

  comma = false;
  gltf_w_ch(w, '{');

  if (out->inverseBindAccessorIndex != GLTF_EXP_INDEX_NONE) {
    gltf_w_key_uint(w,
                    _s_gltf_inverseBindMatrices,
                    _s_gltf_inverseBindMatrices_len,
                    out->inverseBindAccessorIndex);
    comma = true;
  }

  if (gltf_skin_skeleton_valid(out)) {
    GLTFExpIndex skeletonIndex;

    AkNode *skeleton;

    skeleton = out->instance && out->instance->overrideSkeleton
                 ? out->instance->overrideSkeleton
                 : out->skin->skeleton;
    skeletonIndex = gltf_node_index(st, skeleton);
    if (skeletonIndex == GLTF_EXP_INDEX_NONE) {
      w->result = AK_ERR;
      return;
    }

    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key_uint(w, _s_gltf_skeleton, _s_gltf_skeleton_len, skeletonIndex);
    comma = true;
  }

  if (comma)
    gltf_w_ch(w, ',');
  if (!gltf_write_skin_joints(w, st, out))
    w->result = AK_ERR;

  gltf_w_ch(w, '}');
}

void
gltf_write_skins(GLTFExpWriter * __restrict w,
                 GLTFExpState  * __restrict st) {
  size_t i;

  if (st->skins.count == 0)
    return;

  gltf_w_key(w, _s_gltf_skins, _s_gltf_skins_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->skins.count; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_write_skin(w, st, &st->skins.items[i]);
  }

  gltf_w_ch(w, ']');
}
