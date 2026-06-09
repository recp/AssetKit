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

#include "plan/internal.h"

void
gltf_plan(GLTFExpState * __restrict st) {
  AkScene      *scene;
  GLTFExpIndex  sceneIndex;
  bool          foundDefault;

  st->defaultSceneIndex = 0;
  foundDefault          = false;
  sceneIndex            = 0;

  if (st->doc->lib.scenes.first) {
    for (scene = st->doc->lib.scenes.first; scene; scene = scene->next) {
      if (sceneIndex == GLTF_EXP_INDEX_NONE) {
        st->failed = true;
        return;
      }

      if (scene == st->doc->scene) {
        st->defaultSceneIndex = sceneIndex;
        foundDefault = true;
      }

      if (!gltf_plan_scene(st, scene)) {
        st->failed = true;
        return;
      }

      sceneIndex++;
    }

    if (!foundDefault)
      st->defaultSceneIndex = 0;
    if (!gltf_plan_deferred_skin_joint_roots(st)) {
      st->failed = true;
    } else if (!gltf_plan_animations(st)) {
      st->failed = true;
    } else if (!gltf_plan_image_buffer_views(st)) {
      st->failed = true;
    } else if (!gltf_image_prepare_export_uris(st)) {
      st->failed = true;
    }
    return;
  }

  if (!gltf_plan_scene(st, st->doc->scene)) {
    st->failed = true;
  } else if (!gltf_plan_deferred_skin_joint_roots(st)) {
    st->failed = true;
  } else if (!gltf_plan_animations(st)) {
    st->failed = true;
  } else if (!gltf_plan_image_buffer_views(st)) {
    st->failed = true;
  } else if (!gltf_image_prepare_export_uris(st)) {
    st->failed = true;
  }
}
