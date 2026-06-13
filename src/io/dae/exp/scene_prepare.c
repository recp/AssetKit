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

#include "scene.h"
#include "camera.h"
#include "controller.h"
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "source.h"

static
bool
dae_instancing_accessor_supported(AkAccessor * __restrict acc,
                                  uint32_t                componentCount,
                                  uint32_t                count) {
  return !acc
         || (acc->componentType == AKT_FLOAT
             && acc->componentCount == componentCount
             && acc->count == count
             && io_accessor_float_direct(acc));
}

static
bool
dae_gpu_instancing_supported(AkNode * __restrict node) {
  AkGpuInstancing *instancing;
  uint32_t         attrCount;

  instancing = node ? node->gpuInstancing : NULL;
  if (!instancing)
    return true;

  attrCount = (instancing->translation != NULL)
              + (instancing->rotation != NULL)
              + (instancing->scale != NULL);

  return node->geometry
         && instancing->count > 0
         && attrCount > 0
         && dae_instancing_accessor_supported(instancing->translation,
                                              3,
                                              instancing->count)
         && dae_instancing_accessor_supported(instancing->rotation,
                                              4,
                                              instancing->count)
         && dae_instancing_accessor_supported(instancing->scale,
                                              3,
                                              instancing->count);
}

static
bool
dae_node_has_unsupported_features(AkNode * __restrict node,
                                  RBTree * __restrict visited,
                                  uint32_t             depth) {
  AkInstanceGeometry *geomInst;
  AkInstanceNode     *nodeInst;
  AkNode             *child;

  if (!node)
    return false;

  if (depth > DAE_EXP_MAX_NODE_DEPTH)
    return true;

  if (rb_find(visited, node))
    return false;

  rb_insert(visited, node, (void *)(uintptr_t)1);

  if (!dae_gpu_instancing_supported(node))
    return true;

  for (geomInst = node->geometry; geomInst; geomInst = (void *)geomInst->base.next) {
    AkGeometry *geom;

    if (((geom = dae_instance_geometry_object(geomInst)) && !dae_geometry_supported(geom)) 
        || (geom && geomInst->skinner && !dae_instance_skin_supported(geomInst)) 
        || (geom && geomInst->morpher && !dae_instance_morph_supported(geomInst)))
      return true;
  }

  for (child = node->chld; child; child = child->next) {
    if (dae_node_has_unsupported_features(child, visited, depth + 1u))
      return true;
  }

  for (nodeInst = node->node; nodeInst; nodeInst = nodeInst->next) {
    AkNode *target;

    target = ak_instanceNodeTarget(nodeInst);
    if (target
        && dae_node_has_unsupported_features(target, visited, depth + 1u))
      return true;
  }

  return false;
}

AK_HIDE
bool
dae_doc_has_unsupported_features(AkDoc * __restrict doc) {
  AkGeometry *geom;
  AkScene    *scene;
  AkNode     *node;
  RBTree     *visited;
  bool        unsupported;

  if (!doc)
    return true;

  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    if (!dae_geometry_supported(geom))
      return true;
  }

  if (!(visited = rb_newtree_ptr()))
    return true;

  unsupported = false;

  for (scene = doc->lib.scenes.first; scene; scene = scene->next) {
    if (dae_node_has_unsupported_features(scene->node, visited, 0)) {
      unsupported = true;
      goto done;
    }
  }

  if (doc->scene
      && dae_node_has_unsupported_features(doc->scene->node, visited, 0)) {
    unsupported = true;
    goto done;
  }

  for (node = doc->lib.nodes.first; node; node = node->docNext) {
    if (dae_node_has_unsupported_features(node, visited, 0)) {
      unsupported = true;
      goto done;
    }
  }

done:
  rb_destroy(visited);
  return unsupported;
}

static
void
dae_prepare_node_transform_refs(DAEExpState * __restrict st,
                                AkNode      * __restrict node) {
  AkObject *item;

  if (!node || !node->transform)
    return;

  for (item = node->transform->base; item; item = item->next)
    rb_insert(st->nodeTransforms, item, node);

  for (item = node->transform->item; item; item = item->next)
    rb_insert(st->nodeTransforms, item, node);
}

static
void
dae_prepare_node_source_id(DAEExpState * __restrict st,
                           AkNode      * __restrict node) {
  const char *id;
  void       *found;

  id = node ? ak_getId(node) : NULL;
  if (!id || !*id)
    return;

  found = rb_find(st->nodeIds, (void *)id);
  if (!found) {
    rb_insert(st->nodeIds, (void *)id, node);
  } else if (found != node) {
    rb_insert(st->nodeIds, (void *)id, DAE_EXP_DUPLICATE_NODE_ID);
  }
}

static
void
dae_prepare_node_maps(DAEExpState * __restrict st,
                      AkNode      * __restrict node,
                      RBTree      * __restrict visited,
                      uint32_t                 depth,
                      bool                     sceneReachable) {
  AkInstanceGeometry *geomInst;
  AkInstanceBase     *baseInst;
  AkInstanceNode     *nodeInst;
  AkNode             *child;
  bool                alreadyVisited;

  if (!node)
    return;

  if (depth > DAE_EXP_MAX_NODE_DEPTH) {
    st->prepareOk = false;
    return;
  }

  alreadyVisited = rb_find(visited, node) != NULL;
  if (sceneReachable) {
    if (rb_find(st->sceneNodes, node)) {
      if (alreadyVisited)
        return;
    } else {
      rb_insert(st->sceneNodes, node, (void *)(uintptr_t)1);
    }
  } else if (alreadyVisited) {
    return;
  }

  if (!alreadyVisited)
    rb_insert(visited, node, (void *)(uintptr_t)1);

  dae_prepare_node_source_id(st, node);
  dae_prepare_node_transform_refs(st, node);

  if (dae_map_index(st->nodes, node) == UINT32_MAX
      && dae_map_index(st->visualNodes, node) == UINT32_MAX) {
    rb_insert(st->visualNodes,
              node,
              (void *)(uintptr_t)(++st->visualNodeCount));
  }

  for (geomInst = node->geometry;
       geomInst;
       geomInst = (void *)geomInst->base.next) {
    AkSkin     *skin;
    AkGeometry *geom;
    AkMorph    *morph;
    bool        morphSupported;

    skin = geomInst->skinner ? geomInst->skinner->skin : NULL;
    geom = dae_instance_geometry_object(geomInst);
    if (geom
        && dae_map_index(st->geometries, geom) == UINT32_MAX
        && !dae_prepare_extra_geometry(st, geom)) {
      st->prepareOk = false;
      return;
    }
    if (geom && !dae_prepare_instance_materials(st, geom, geomInst)) {
      st->prepareOk = false;
      return;
    }
    morph = geomInst->morpher ? geomInst->morpher->morph : NULL;
    morphSupported = dae_instance_morph_supported(geomInst);
    if (morphSupported && morph && !dae_prepare_morph(st, morph)) {
      st->prepareOk = false;
      return;
    }
    if (skin
        && geom
        && dae_map_index(st->skins, skin) != UINT32_MAX
        && !rb_find(st->skinGeometries, skin)) {
      rb_insert(st->skinGeometries, skin, geom);
      rb_insert(st->skinInstances, skin, geomInst->skinner);
      if (morphSupported
          && geomInst->morpher
          && geomInst->morpher->morph
          && dae_map_index(st->morphs, geomInst->morpher->morph) != UINT32_MAX)
        rb_insert(st->skinMorphs, skin, geomInst->morpher->morph);
    }

    if (morphSupported
        && geomInst->morpher
        && morph
        && geom
        && dae_map_index(st->morphs, morph) != UINT32_MAX
        && !rb_find(st->morphGeometries, morph)) {
      rb_insert(st->morphGeometries, morph, geom);
      rb_insert(st->morphInstances, morph, geomInst->morpher);
      dae_mark_morph_vertex_geometry(st, geom);
    }

    if (morphSupported
        && geomInst->morpher
        && morph
        && dae_map_index(st->morphs, morph) != UINT32_MAX
        && !dae_prepare_morph_target_geometries(st, morph)) {
      st->prepareOk = false;
      return;
    }
  }

  for (baseInst = node->camera; baseInst; baseInst = baseInst->next) {
    if (!dae_prepare_extra_camera(st, dae_instance_camera_object(baseInst))) {
      st->prepareOk = false;
      return;
    }
  }

  for (baseInst = node->light; baseInst; baseInst = baseInst->next) {
    if (!dae_prepare_extra_light(st, dae_instance_light_object(baseInst))) {
      st->prepareOk = false;
      return;
    }
  }

  for (child = node->chld; child; child = child->next) {
    dae_prepare_node_maps(st, child, visited, depth + 1u, sceneReachable);
    if (!st->prepareOk)
      return;
  }

  for (nodeInst = node->node; nodeInst; nodeInst = nodeInst->next) {
    AkNode *target;

    target = ak_instanceNodeTarget(nodeInst);
    dae_prepare_node_maps(st,
                          target,
                          visited,
                          depth + 1u,
                          sceneReachable
                            && target
                            && target->gpuInstancing);
    if (!st->prepareOk)
      return;
  }
}

static
void
dae_prepare_visual_node_maps(DAEExpState * __restrict st) {
  AkScene *scene;
  AkNode  *node;
  RBTree  *visited;

  visited = rb_newtree_ptr();
  if (!visited) {
    st->prepareOk = false;
    return;
  }

  for (scene = st->doc->lib.scenes.first; scene; scene = scene->next) {
    dae_prepare_node_maps(st, scene->node, visited, 0, true);
    if (!st->prepareOk)
      goto done;
  }

  if (st->doc->scene) {
    dae_prepare_node_maps(st, st->doc->scene->node, visited, 0, true);
    if (!st->prepareOk)
      goto done;
  }

  for (node = st->doc->lib.nodes.first; node; node = node->docNext) {
    dae_prepare_node_maps(st, node, visited, 0, false);
    if (!st->prepareOk)
      goto done;
  }

done:
  rb_destroy(visited);
}

AK_HIDE
bool
dae_prepare_maps(DAEExpState * __restrict st) {
  AkGeometry *geom;
  AkMaterial *mat;
  AkCamera   *camera;
  AkLight    *light;
  AkNode     *node;
  AkSkin     *skin;
  AkMorph    *morph;
  uint32_t    idx;

  idx = 0;
  for (geom = st->doc->lib.geometries.first; geom; geom = geom->next) {
    if (!dae_prepare_geometry_index_mode(st, geom))
      return false;
    rb_insert(st->geometries, geom, (void *)(uintptr_t)(++idx));
  }
  st->geometryCount = idx;

  idx = 0;
  for (mat = st->doc->lib.materials.first; mat; mat = mat->next)
    rb_insert(st->materials, mat, (void *)(uintptr_t)(++idx));
  st->materialCount = idx;

  for (mat = st->doc->lib.materials.first; mat; mat = mat->next) {
    if (!dae_prepare_material_dependencies(st, mat))
      return false;
  }

  idx = 0;
  for (camera = st->doc->lib.cameras.first; camera; camera = camera->next)
    rb_insert(st->cameras, camera, (void *)(uintptr_t)(++idx));
  st->cameraCount = idx;

  idx = 0;
  for (light = st->doc->lib.lights.first; light; light = light->next)
    rb_insert(st->lights, light, (void *)(uintptr_t)(++idx));
  st->lightCount = idx;

  idx = 0;
  for (node = st->doc->lib.nodes.first; node; node = node->docNext)
    rb_insert(st->nodes, node, (void *)(uintptr_t)(++idx));
  st->nodeCount = idx;

  idx = 0;
  for (skin = st->doc->lib.skins.first; skin; skin = skin->next)
    rb_insert(st->skins, skin, (void *)(uintptr_t)(++idx));
  st->skinCount = idx;

  idx = 0;
  for (morph = st->doc->lib.morphs.first; morph; morph = morph->next)
    rb_insert(st->morphs, morph, (void *)(uintptr_t)(++idx));
  st->morphCount = idx;

  dae_prepare_visual_node_maps(st);
  if (!st->prepareOk)
    return false;

  st->imageRefsOnly = st->imageCount > 0;
  if (!st->imageRefsOnly) {
    AkImage *image;

    idx = 0;
    for (image = st->doc->lib.images.first; image; image = image->next)
      rb_insert(st->images, image, (void *)(uintptr_t)(++idx));
    st->imageCount = idx;
  }

  return st->prepareOk;
}
