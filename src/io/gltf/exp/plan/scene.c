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
gltf_indices_reserve_span(GLTFExpIndexList * __restrict list,
                          size_t                        count,
                          GLTFExpIndex     * __restrict offset) {
  size_t needed;

  *offset = 0;
  if (count == 0)
    return true;

  if (list->count >= GLTF_EXP_INDEX_NONE
      || count > (size_t)GLTF_EXP_INDEX_NONE - list->count)
    return false;

  needed = list->count + count;
  if (!gltf_indices_reserve(list, needed))
    return false;

  *offset = (GLTFExpIndex)list->count;
  list->count = needed;

  return true;
}

AK_HIDE
size_t
gltf_node_direct_child_count(AkNode * __restrict node) {
  AkInstanceNode *inst;
  AkNode         *chld;
  size_t          count;

  count = 0;
  for (chld = node ? node->chld : NULL; chld; chld = chld->next)
    count++;

  for (inst = node ? node->node : NULL; inst; inst = inst->next) {
    if (ak_instanceNodeTarget(inst))
      count++;
  }

  return count;
}

AK_HIDE
bool
gltf_plan_child_index(GLTFExpState   * __restrict st,
                      GLTFExpNodeOut * __restrict out,
                      uint32_t       * __restrict writeIndex,
                      GLTFExpIndex                 childIndex) {
  if (childIndex == GLTF_EXP_INDEX_NONE || *writeIndex >= out->childCount)
    return false;

  st->nodeChildren.items[out->childOffset + *writeIndex] = childIndex;
  *writeIndex += 1u;

  return true;
}

GLTFExpIndex
gltf_node_index(GLTFExpState * __restrict st,
                AkNode       * __restrict node) {
  uintptr_t idx;

  if (!node)
    return GLTF_EXP_INDEX_NONE;

  idx = (uintptr_t)rb_find(st->nodeMap, node);
  if (idx == 0)
    return GLTF_EXP_INDEX_NONE;

  return (GLTFExpIndex)(idx - 1);
}

AK_HIDE
AkInstanceGeometry*
gltf_node_geometry(AkNode * __restrict node, bool * __restrict ok) {
  AkInstanceGeometry *inst;
  AkInstanceGeometry *geomInst;

  geomInst = NULL;

  for (inst = node ? node->geometry : NULL;
       inst;
       inst = (AkInstanceGeometry *)inst->base.next) {
    if (!inst->base.object)
      continue;

    if (geomInst) {
      *ok = false;
      return NULL;
    }

    geomInst = inst;
  }

  return geomInst;
}

AK_HIDE
bool
gltf_plan_node_camera(GLTFExpState   * __restrict st,
                      AkNode         * __restrict node,
                      GLTFExpNodeOut * __restrict out) {
  AkInstanceBase *inst;
  AkCamera       *camera;
  uint32_t        count;

  camera = NULL;
  count  = 0;

  for (inst = node ? node->camera : NULL; inst; inst = inst->next) {
    if (!inst->object)
      continue;

    camera = (AkCamera *)inst->object;
    count++;
    if (count > 1)
      return false;
  }

  if (!camera)
    return true;

  if (!gltf_camera_supported(camera))
    return true;

  if (!gltf_plan_camera(st, camera))
    return false;

  out->cameraIndex = gltf_ptrs_index(&st->cameras, camera);
  if (out->cameraIndex == GLTF_EXP_INDEX_NONE)
    return false;

  out->hasCamera = true;

  return true;
}

AK_HIDE
bool
gltf_plan_node_light(GLTFExpState   * __restrict st,
                     AkNode         * __restrict node,
                     GLTFExpNodeOut * __restrict out) {
  AkInstanceBase *inst;
  AkLight        *light;

  light = NULL;

  for (inst = node ? node->light : NULL; inst; inst = inst->next) {
    if (!inst->object)
      continue;

    if (!gltf_light_supported((AkLight *)inst->object))
      continue;

    if (!light) {
      light = (AkLight *)inst->object;
    }
  }

  if (!light)
    return true;

  if (!gltf_plan_light(st, light))
    return false;

  out->lightIndex = gltf_ptrs_index(&st->lights, light);
  if (out->lightIndex == GLTF_EXP_INDEX_NONE)
    return false;

  out->hasLight = true;

  return true;
}

AK_HIDE
bool
gltf_gpu_instancing_accessor_ok(AkAccessor * __restrict acc,
                                uint32_t                componentCount,
                                uint32_t                count) {
  return acc
         && acc->componentType == AKT_FLOAT
         && acc->componentCount == componentCount
         && acc->count == count
         && acc->count > 0;
}

AK_HIDE
bool
gltf_plan_gpu_instancing_accessor(GLTFExpState * __restrict st,
                                  AkAccessor   * __restrict acc,
                                  uint32_t                  componentCount,
                                  uint32_t                  count) {
  if (!acc)
    return true;

  return gltf_gpu_instancing_accessor_ok(acc, componentCount, count)
         && gltf_accessors_add_accessor_target(&st->accessors,
                                               acc,
                                               GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY);
}

AK_HIDE
bool
gltf_plan_node_gpu_instancing(GLTFExpState   * __restrict st,
                              AkNode         * __restrict node,
                              GLTFExpNodeOut * __restrict out) {
  AkGpuInstancing *instancing;
  uint32_t         attrCount;

  instancing = node ? node->gpuInstancing : NULL;
  if (!instancing)
    return true;

  attrCount = (instancing->translation != NULL)
              + (instancing->rotation != NULL)
              + (instancing->scale != NULL);
  if (!out->hasMesh || instancing->count == 0 || attrCount == 0)
    return false;

  if (!gltf_plan_gpu_instancing_accessor(st,
                                         instancing->translation,
                                         3,
                                         instancing->count)
      || !gltf_plan_gpu_instancing_accessor(st,
                                            instancing->rotation,
                                            4,
                                            instancing->count)
      || !gltf_plan_gpu_instancing_accessor(st,
                                            instancing->scale,
                                            3,
                                            instancing->count))
    return false;

  st->usesGpuInstancing = true;

  return true;
}

AK_HIDE
GLTFExpIndex
gltf_plan_node(GLTFExpState * __restrict st,
               AkNode       * __restrict node,
               const char   * __restrict name) {
  GLTFExpNodeOut *out;
  AkInstanceNode *inst;
  AkInstanceGeometry *geomInst;
  AkNode         *chld;
  GLTFExpIndex    nodeIndex;
  uint32_t        childWriteIndex;
  size_t          childCount;
  bool            ok;

  if (!node)
    return GLTF_EXP_INDEX_NONE;

  if (rb_find(st->nodeStack, node)) {
    st->failed = true;
    return GLTF_EXP_INDEX_NONE;
  }

  if (st->nodes.count >= GLTF_EXP_INDEX_NONE) {
    st->failed = true;
    return GLTF_EXP_INDEX_NONE;
  }

  if (st->nodes.count == st->nodes.capacity) {
    size_t newCap;

    if (!gltf_next_capacity(st->nodes.capacity, 128, &newCap)) {
      st->failed = true;
      return GLTF_EXP_INDEX_NONE;
    }
    if (!gltf_nodes_reserve(&st->nodes, newCap)) {
      st->failed = true;
      return GLTF_EXP_INDEX_NONE;
    }
  }

  nodeIndex = (GLTFExpIndex)st->nodes.count++;
  out       = &st->nodes.items[nodeIndex];
  memset(out, 0, sizeof(*out));
  out->node        = node;
  out->name        = name ? name : node->name;
  out->meshIndex   = GLTF_EXP_INDEX_NONE;
  out->cameraIndex = GLTF_EXP_INDEX_NONE;
  out->lightIndex  = GLTF_EXP_INDEX_NONE;
  out->skinIndex   = GLTF_EXP_INDEX_NONE;
  out->childOffset = 0;
  childWriteIndex  = 0;
  childCount       = gltf_node_direct_child_count(node);
  if (childCount > UINT32_MAX
      || !gltf_indices_reserve_span(&st->nodeChildren,
                                    childCount,
                                    &out->childOffset)) {
    st->failed = true;
    return GLTF_EXP_INDEX_NONE;
  }
  out->childCount = (uint32_t)childCount;
  if (!node->visible)
    st->usesNodeVisibility = true;

  if (!rb_find(st->nodeMap, node))
    rb_insert(st->nodeMap, node, (void *)(uintptr_t)(nodeIndex + 1));

  rb_insert(st->nodeStack, node, (void *)(uintptr_t)1);

  ok   = true;
  geomInst = gltf_node_geometry(node, &ok);
  out->bakeLocalTransform = gltf_node_can_bake_local_mesh(node, geomInst);
  if (!ok
      || !gltf_plan_mesh(st,
                         geomInst,
                         out->bakeLocalTransform ? node : NULL,
                         &out->meshIndex)) {
    st->failed = true;
    goto done;
  }
  out->hasMesh = out->meshIndex != GLTF_EXP_INDEX_NONE;
  if (out->hasMesh && geomInst && geomInst->morpher)
    out->morphWeights = geomInst->morpher->overrideWeights;

  if (!gltf_plan_node_gpu_instancing(st, node, out)) {
    st->failed = true;
    goto done;
  }

  if (geomInst && geomInst->skinner) {
    if (gltf_skin_valid(geomInst->skinner)) {
      if (!gltf_skins_add(st, geomInst->skinner, &out->skinIndex)) {
        st->failed = true;
        goto done;
      }
      out->hasSkin = out->skinIndex != GLTF_EXP_INDEX_NONE;
    }
  }

  if (!gltf_plan_node_camera(st, node, out)) {
    st->failed = true;
    goto done;
  }

  if (!gltf_plan_node_light(st, node, out)) {
    st->failed = true;
    goto done;
  }

  if (!gltf_plan_extra_extensions(st,
                                  ak_extra(node),
                                  gltf_plan_skip_node_core_extension,
                                  NULL)) {
    st->failed = true;
    goto done;
  }

  for (chld = node->chld; chld; chld = chld->next) {
    GLTFExpIndex childIndex;

    childIndex = gltf_plan_node(st, chld, NULL);
    if (st->failed) {
      st->failed = true;
      goto done;
    }

    out = &st->nodes.items[nodeIndex];
    if (!gltf_plan_child_index(st, out, &childWriteIndex, childIndex)) {
      st->failed = true;
      goto done;
    }
  }

  for (inst = node->node; inst; inst = inst->next) {
    AkNode     *target;
    const char *instName;
    GLTFExpIndex childIndex;

    target = ak_instanceNodeTarget(inst);
    if (!target)
      continue;

    instName   = inst->name ? inst->name : target->name;
    childIndex = gltf_plan_node(st, target, instName);
    if (st->failed) {
      st->failed = true;
      goto done;
    }

    out = &st->nodes.items[nodeIndex];
    if (!gltf_plan_child_index(st, out, &childWriteIndex, childIndex)) {
      st->failed = true;
      goto done;
    }
  }

  if (childWriteIndex != childCount) {
    st->failed = true;
    goto done;
  }

done:
  rb_remove(st->nodeStack, node);

  return st->failed ? GLTF_EXP_INDEX_NONE : nodeIndex;
}

AK_HIDE
bool
gltf_plan_scene_root(GLTFExpState * __restrict st,
                     GLTFExpSceneOut * __restrict out,
                     GLTFExpIndex              rootIndex) {
  if (rootIndex == GLTF_EXP_INDEX_NONE)
    return true;

  if (out->rootCount == UINT32_MAX)
    return false;

  if (!gltf_indices_add(&st->sceneRoots, rootIndex))
    return false;

  out->rootCount++;

  return true;
}

AK_HIDE
void
gltf_scene_skin_joint_roots_reset(GLTFExpState * __restrict st) {
  st->sceneSkinJointRoots.count = 0;
  if (st->sceneSkinJointRoots.map) {
    rb_empty(st->sceneSkinJointRoots.map);
    st->sceneSkinJointRoots.map->count = 0;
  }
}

AK_HIDE
bool
gltf_scene_has_root_index(GLTFExpState    * __restrict st,
                          GLTFExpSceneOut * __restrict out,
                          GLTFExpIndex                  rootIndex) {
  uint32_t i;

  if (rootIndex == GLTF_EXP_INDEX_NONE)
    return true;

  for (i = 0; i < out->rootCount; i++) {
    if (st->sceneRoots.items[out->rootOffset + i] == rootIndex)
      return true;
  }

  return false;
}

AK_HIDE
bool
gltf_node_subtree_has_index(GLTFExpState * __restrict st,
                            GLTFExpIndex              nodeIndex,
                            GLTFExpIndex              targetIndex) {
  GLTFExpNodeOut *out;
  uint32_t        i;

  if (nodeIndex == GLTF_EXP_INDEX_NONE || nodeIndex >= st->nodes.count)
    return false;

  if (nodeIndex == targetIndex)
    return true;

  out = &st->nodes.items[nodeIndex];
  for (i = 0; i < out->childCount; i++) {
    GLTFExpIndex childIndex;

    childIndex = st->nodeChildren.items[out->childOffset + i];
    if (gltf_node_subtree_has_index(st, childIndex, targetIndex))
      return true;
  }

  return false;
}

AK_HIDE
bool
gltf_scene_has_node_index(GLTFExpState    * __restrict st,
                          GLTFExpSceneOut * __restrict out,
                          GLTFExpIndex                  nodeIndex) {
  uint32_t i;

  if (nodeIndex == GLTF_EXP_INDEX_NONE)
    return true;

  for (i = 0; i < out->rootCount; i++) {
    GLTFExpIndex rootIndex;

    rootIndex = st->sceneRoots.items[out->rootOffset + i];
    if (gltf_node_subtree_has_index(st, rootIndex, nodeIndex))
      return true;
  }

  return false;
}

AK_HIDE
bool
gltf_plan_scene_skin_joint_roots(GLTFExpState    * __restrict st,
                                 GLTFExpSceneOut * __restrict out) {
  size_t i;

  for (i = 0; i < st->sceneSkinJointRoots.count; i++) {
    AkNode *root;
    GLTFExpIndex rootIndex;

    root = st->sceneSkinJointRoots.items[i];
    if (!root)
      continue;

    rootIndex = gltf_node_index(st, root);
    if (rootIndex == GLTF_EXP_INDEX_NONE)
      rootIndex = gltf_plan_node(st, root, NULL);
    if (st->failed)
      return false;

    if (!gltf_scene_has_node_index(st, out, rootIndex)
        && !gltf_scene_has_root_index(st, out, rootIndex)
        && !gltf_plan_scene_root(st, out, rootIndex))
      return false;
  }

  return true;
}

AK_HIDE
bool
gltf_scene_entrypoint_needed(AkNode * __restrict node) {
  return node
         && (node->transform
             || node->geometry
             || node->camera
             || node->light);
}

AK_HIDE
bool
gltf_plan_scene(GLTFExpState * __restrict st,
                AkScene      * __restrict scene) {
  GLTFExpSceneOut *out;
  AkInstanceNode  *inst;
  AkNode          *root;
  GLTFExpIndex     sceneIndex;

  if (st->scenes.count >= GLTF_EXP_INDEX_NONE)
    return false;
  if (st->sceneRoots.count >= GLTF_EXP_INDEX_NONE)
    return false;

  if (st->scenes.count == st->scenes.capacity) {
    size_t newCap;

    if (!gltf_next_capacity(st->scenes.capacity, 8, &newCap))
      return false;
    if (!gltf_scenes_reserve(&st->scenes, newCap))
      return false;
  }

  sceneIndex = (GLTFExpIndex)st->scenes.count++;
  out        = &st->scenes.items[sceneIndex];
  memset(out, 0, sizeof(*out));
  out->scene      = scene;
  out->rootOffset = (GLTFExpIndex)st->sceneRoots.count;
  gltf_scene_skin_joint_roots_reset(st);

  if (!gltf_plan_extra_extensions(st, ak_extra(scene), NULL, NULL))
    return false;

  if (!scene || !scene->node)
    return true;

  if (gltf_scene_entrypoint_needed(scene->node)) {
    GLTFExpIndex rootIndex;

    rootIndex = gltf_plan_node(st, scene->node, NULL);
    return !st->failed
           && gltf_plan_scene_root(st, out, rootIndex)
           && gltf_plan_scene_skin_joint_roots(st, out);
  }

  for (root = scene->node->chld; root; root = root->next) {
    GLTFExpIndex rootIndex;

    rootIndex = gltf_plan_node(st, root, NULL);
    if (st->failed || !gltf_plan_scene_root(st, out, rootIndex))
      return false;
  }

  for (inst = scene->node->node; inst; inst = inst->next) {
    AkNode     *target;
    const char *instName;
    GLTFExpIndex rootIndex;

    target = ak_instanceNodeTarget(inst);
    if (!target)
      continue;

    instName  = inst->name ? inst->name : target->name;
    rootIndex = gltf_plan_node(st, target, instName);
    if (st->failed || !gltf_plan_scene_root(st, out, rootIndex))
      return false;
  }

  return gltf_plan_scene_skin_joint_roots(st, out);
}
