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

#include "document.h"
#include "anim.h"
#include "asset.h"
#include "camera.h"
#include "controller.h"
#include "image.h"
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "scene.h"

static
bool
dae_geometry_requires_collada_150(AkGeometry * __restrict geom) {
  return geom
         && geom->gdata
         && geom->gdata->type == AK_GEOMETRY_BREP;
}

static
bool
dae_doc_requires_collada_150(DAEExpState * __restrict st) {
  DAEExpGeometryRef *geomRef;
  AkGeometry        *geom;

  if (!st || !st->doc)
    return false;

  for (geom = st->doc->lib.geometries.first; geom; geom = geom->next) {
    if (dae_geometry_requires_collada_150(geom))
      return true;
  }

  for (geomRef = st->extraGeometries; geomRef; geomRef = geomRef->next) {
    if (dae_geometry_requires_collada_150(geomRef->geom))
      return true;
  }

  return false;
}

static
bool
dae_doc_scene_in_library(AkDoc * __restrict doc) {
  AkScene *scene;

  if (!doc || !doc->scene)
    return false;

  for (scene = doc->lib.scenes.first; scene; scene = scene->next) {
    if (scene == doc->scene)
      return true;
  }

  return false;
}

static
uint32_t
dae_doc_scene_count(AkDoc * __restrict doc) {
  AkScene *scene;
  uint32_t count;

  count = 0;
  if (!doc)
    return 0;

  for (scene = doc->lib.scenes.first; scene; scene = scene->next)
    count++;

  return count;
}

AK_HIDE
bool
dae_select_collada_version(DAEExpState * __restrict st) {
  bool requires150;

  if (!st)
    return false;

  requires150 = dae_doc_requires_collada_150(st);
  switch (st->versionMode) {
    case AK_DAE_EXPORT_VERSION_AUTO:
      st->useCollada150 = requires150;
      return true;
    case AK_DAE_EXPORT_VERSION_1_4:
      if (requires150)
        return false;
      st->useCollada150 = false;
      return true;
    case AK_DAE_EXPORT_VERSION_1_5:
      st->useCollada150 = true;
      return true;
    default:
      break;
  }

  return false;
}

AK_HIDE
void
dae_write_doc(DAEExpState * __restrict st) {
  DAEExpWriter *w;
  DAEExpMorphRef *morphRef;
  AkImage      *image;
  AkMaterial   *mat;
  AkGeometry   *geom;
  AkCamera     *camera;
  AkLight      *light;
  DAEExpObjectRef *objRef;
  AkMorph      *morph;
  AkScene      *scene;
  uint32_t      idx;
  uint32_t      activeScene;
  bool          wroteScene;
  bool          activeSceneInLibrary;

  w = &st->w;

  dae_w_lit(w, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  if (st->useCollada150) {
    /* 1.5.1 is a specification maintenance revision. Its schema revision
       and instance-document version remain 1.5.0. */
    dae_w_lit(w, "<COLLADA xmlns=\"http://www.collada.org/2008/03/COLLADASchema\" version=\"1.5.0");
  } else {
    dae_w_lit(w, "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1");
  }
  dae_w_lit(w, "\">\n");
  dae_write_asset(st);

  if (st->imageCount > 0) {
    dae_w_lit(w, "<library_images>");
    idx = 0;
    if (st->imageRefsOnly) {
      for (objRef = st->extraImages; objRef; objRef = objRef->next)
        dae_write_image(st, objRef->object, idx++);
    } else {
      for (image = st->doc->lib.images.first; image; image = image->next)
        dae_write_image(st, image, idx++);
    }
    dae_w_lit(w, "</library_images>\n");
  }

  if (st->cameraCount > 0) {
    dae_w_lit(w, "<library_cameras>");
    idx = 0;
    for (camera = st->doc->lib.cameras.first; camera; camera = camera->next)
      dae_write_camera(st, camera, idx++);
    for (objRef = st->extraCameras; objRef; objRef = objRef->next)
      dae_write_camera(st, objRef->object, idx++);
    dae_w_lit(w, "</library_cameras>\n");
  }

  if (st->lightCount > 0) {
    dae_w_lit(w, "<library_lights>");
    idx = 0;
    for (light = st->doc->lib.lights.first; light; light = light->next)
      dae_write_light(st, light, idx++);
    for (objRef = st->extraLights; objRef; objRef = objRef->next)
      dae_write_light(st, objRef->object, idx++);
    dae_w_lit(w, "</library_lights>\n");
  }

  if (st->materialCount > 0) {
    dae_w_lit(w, "<library_effects>");
    idx = 0;
    for (mat = st->doc->lib.materials.first; mat; mat = mat->next)
      dae_write_effect(st, mat, idx++);
    for (objRef = st->extraMaterials; objRef; objRef = objRef->next)
      dae_write_effect(st, objRef->object, idx++);
    dae_w_lit(w, "</library_effects>\n<library_materials>");
    idx = 0;
    for (mat = st->doc->lib.materials.first; mat; mat = mat->next)
      dae_write_material(st, mat, idx++);
    for (objRef = st->extraMaterials; objRef; objRef = objRef->next)
      dae_write_material(st, objRef->object, idx++);
    dae_w_lit(w, "</library_materials>\n");
  }

  dae_write_library_animations(st);

  if (st->geometryCount > 0) {
    dae_w_lit(w, "<library_geometries>");
    idx = 0;
    for (geom = st->doc->lib.geometries.first; geom; geom = geom->next) {
      if (!dae_write_geometry(st, geom, idx++)) {
        if (w->result == AK_OK)
          w->result = AK_EINVAL;
        return;
      }
    }
    for (DAEExpGeometryRef *geomRef = st->extraGeometries;
         geomRef;
         geomRef = geomRef->next) {
      uint32_t geomIdx;

      geomIdx = dae_map_index(st->geometries, geomRef->geom);
      if (geomIdx == UINT32_MAX
          || !dae_write_geometry(st, geomRef->geom, geomIdx)) {
        if (w->result == AK_OK)
          w->result = AK_EINVAL;
        return;
      }
    }
    for (morph = st->doc->lib.morphs.first; morph; morph = morph->next) {
      idx  = dae_map_index(st->morphs, morph);
      geom = rb_find(st->morphGeometries, morph);
      if (idx != UINT32_MAX
          && geom
          && !dae_write_morphable_target_geometries(st, morph, geom, idx)) {
        if (w->result == AK_OK)
          w->result = AK_EINVAL;
        return;
      }
    }
    for (morphRef = st->extraMorphs; morphRef; morphRef = morphRef->next) {
      morph = morphRef->morph;
      idx   = dae_map_index(st->morphs, morph);
      geom  = rb_find(st->morphGeometries, morph);
      if (idx != UINT32_MAX
          && geom
          && !dae_write_morphable_target_geometries(st, morph, geom, idx)) {
        if (w->result == AK_OK)
          w->result = AK_EINVAL;
        return;
      }
    }
    dae_w_lit(w, "</library_geometries>\n");
  }

  dae_write_library_controllers(st);

  dae_write_library_nodes(st);

  dae_w_lit(w, "<library_visual_scenes>");
  wroteScene           = false;
  activeSceneInLibrary = dae_doc_scene_in_library(st->doc);
  idx                  = 0;
  for (scene = st->doc->lib.scenes.first; scene; scene = scene->next) {
    dae_write_visual_scene(st, scene, idx++);
    wroteScene = true;
  }
  if (st->doc->scene && !activeSceneInLibrary) {
    dae_write_visual_scene(st, st->doc->scene, idx++);
    wroteScene = true;
  }
  if (!wroteScene)
    dae_write_visual_scene(st, st->doc->scene, 0);
  dae_w_lit(w, "</library_visual_scenes>\n");

  activeScene = !st->doc->scene
                ? 0
                : activeSceneInLibrary
                  ? dae_active_scene_index(st->doc)
                  : dae_doc_scene_count(st->doc);
  dae_w_lit(w, "<scene><instance_visual_scene url=\"#");
  dae_w_id(w, DAE_EXP_NAME(scene), activeScene);
  dae_w_lit(w, "\"/>");
  dae_write_extra(w, st->doc ? st->doc->extra : NULL);
  dae_w_lit(w, "</scene>\n</COLLADA>\n");
}
