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
#include "extra.h"
#include "plan.h"
#include "../strpool.h"

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/quat.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

static
void
gltf_w_matrix(GLTFExpWriter * __restrict w,
              AkNode        * __restrict node) {
  AkMatrix       matrix;
  const AkFloat *val;
  int            i;

  ak_transformCombine(node->transform, matrix.val[0]);
  val = &matrix.val[0][0];

  gltf_w_key(w, _s_gltf_matrix, _s_gltf_matrix_len);
  gltf_w_ch(w, '[');
  for (i = 0; i < 16; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_w_float(w, val[i]);
  }
  gltf_w_ch(w, ']');
}

static
void
gltf_w_float_array(GLTFExpWriter * __restrict w,
                   const char    * __restrict key,
                   size_t                     keyLen,
                   const float   * __restrict val,
                   uint32_t                   count) {
  uint32_t i;

  gltf_w_key(w, key, keyLen);
  gltf_w_ch(w, '[');
  for (i = 0; i < count; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_w_float(w, val[i]);
  }
  gltf_w_ch(w, ']');
}

static
bool
gltf_float_close(float a, float b) {
  float diff;
  float scale;

  diff  = fabsf(a - b);
  scale = fmaxf(fmaxf(fabsf(a), fabsf(b)), 1.0f);

  return diff <= scale * 1.0e-4f;
}

static
bool
gltf_node_core_extension_skip(const char * __restrict name,
                              size_t                  nameLen,
                              void * __restrict       userdata) {
  (void)userdata;
  return ak_str_eq_fast(name,
                        nameLen,
                        _s_gltf_KHR_lights_punctual,
                        _s_gltf_KHR_lights_punctual_len)
         || ak_str_eq_fast(name,
                           nameLen,
                           _s_gltf_EXT_mesh_gpu_instancing,
                           _s_gltf_EXT_mesh_gpu_instancing_len)
         || ak_str_eq_fast(name,
                           nameLen,
                           _s_gltf_KHR_node_visibility,
                           _s_gltf_KHR_node_visibility_len);
}

static
bool
gltf_matrix_to_trs(AkNode * __restrict node,
                   vec3    translation,
                   versor  rotation,
                   vec3    scale) {
  AkMatrix matrix;
  mat4     m;
  mat4     rotm;
  mat4     recon;
  vec4     t4;
  int      c, r;

  ak_transformCombine(node->transform, matrix.val[0]);
  memcpy(m, matrix.val, sizeof(m));

  if (!gltf_float_close(m[0][3], 0.0f)
      || !gltf_float_close(m[1][3], 0.0f)
      || !gltf_float_close(m[2][3], 0.0f)
      || !gltf_float_close(m[3][3], 1.0f))
    return false;

  glm_decompose(m, t4, rotm, scale);
  if (!isfinite(t4[0]) || !isfinite(t4[1]) || !isfinite(t4[2])
      || !isfinite(scale[0]) || !isfinite(scale[1]) || !isfinite(scale[2]))
    return false;

  glm_mat4_quat(rotm, rotation);
  glm_quat_normalize(rotation);
  if (!isfinite(rotation[0]) || !isfinite(rotation[1])
      || !isfinite(rotation[2]) || !isfinite(rotation[3]))
    return false;

  glm_quat_mat4(rotation, recon);
  for (r = 0; r < 3; r++) {
    recon[0][r] *= scale[0];
    recon[1][r] *= scale[1];
    recon[2][r] *= scale[2];
  }
  recon[3][0] = t4[0];
  recon[3][1] = t4[1];
  recon[3][2] = t4[2];
  recon[3][3] = 1.0f;

  for (c = 0; c < 4; c++) {
    for (r = 0; r < 4; r++) {
      if (!gltf_float_close(m[c][r], recon[c][r]))
        return false;
    }
  }

  translation[0] = t4[0];
  translation[1] = t4[1];
  translation[2] = t4[2];

  return true;
}

static
bool
gltf_node_trs(AkNode      * __restrict node,
              AkTranslate ** __restrict translation,
              AkQuaternion ** __restrict rotation,
              AkScale      ** __restrict scale) {
  AkObject *obj;
  bool      any;

  *translation = NULL;
  *rotation    = NULL;
  *scale       = NULL;
  any          = false;

  if (!node || !node->transform)
    return false;

  for (obj = node->transform->item; obj; obj = obj->next) {
    switch (obj->type) {
      case AKT_TRANSLATE:
        if (*translation)
          return false;
        *translation = ak_objGet(obj);
        any = true;
        break;
      case AKT_QUATERNION:
        if (*rotation)
          return false;
        *rotation = ak_objGet(obj);
        any = true;
        break;
      case AKT_SCALE:
        if (*scale)
          return false;
        *scale = ak_objGet(obj);
        any = true;
        break;
      default:
        return false;
    }
  }

  return any;
}

static
void
gltf_write_node_transform(GLTFExpWriter * __restrict w,
                          AkNode        * __restrict node,
                          GLTFExpNodeOut * __restrict out,
                          bool          * __restrict comma) {
  AkTranslate   *translation;
  AkQuaternion  *rotation;
  AkScale       *scale;
  vec3           trsTranslation;
  versor         trsRotation;
  vec3           trsScale;

  if (!node->transform)
    return;

  if (out->bakeLocalTransform)
    return;

  if (gltf_node_trs(node, &translation, &rotation, &scale)) {
    if (translation) {
      if (*comma)
        gltf_w_ch(w, ',');
      gltf_w_float_array(w,
                         _s_gltf_translation,
                         _s_gltf_translation_len,
                         translation->val,
                         3);
      *comma = true;
    }

    if (rotation) {
      if (*comma)
        gltf_w_ch(w, ',');
      gltf_w_float_array(w,
                         _s_gltf_rotation,
                         _s_gltf_rotation_len,
                         rotation->val,
                         4);
      *comma = true;
    }

    if (scale) {
      if (*comma)
        gltf_w_ch(w, ',');
      gltf_w_float_array(w,
                         _s_gltf_scale,
                         _s_gltf_scale_len,
                         scale->val,
                         3);
      *comma = true;
    }
    return;
  }

  if (gltf_matrix_to_trs(node, trsTranslation, trsRotation, trsScale)) {
    if (!gltf_float_close(trsTranslation[0], 0.0f)
        || !gltf_float_close(trsTranslation[1], 0.0f)
        || !gltf_float_close(trsTranslation[2], 0.0f)) {
      if (*comma)
        gltf_w_ch(w, ',');
      gltf_w_float_array(w,
                         _s_gltf_translation,
                         _s_gltf_translation_len,
                         trsTranslation,
                         3);
      *comma = true;
    }

    if (!gltf_float_close(trsRotation[0], 0.0f)
        || !gltf_float_close(trsRotation[1], 0.0f)
        || !gltf_float_close(trsRotation[2], 0.0f)
        || !gltf_float_close(trsRotation[3], 1.0f)) {
      if (*comma)
        gltf_w_ch(w, ',');
      gltf_w_float_array(w,
                         _s_gltf_rotation,
                         _s_gltf_rotation_len,
                         trsRotation,
                         4);
      *comma = true;
    }

    if (!gltf_float_close(trsScale[0], 1.0f)
        || !gltf_float_close(trsScale[1], 1.0f)
        || !gltf_float_close(trsScale[2], 1.0f)) {
      if (*comma)
        gltf_w_ch(w, ',');
      gltf_w_float_array(w,
                         _s_gltf_scale,
                         _s_gltf_scale_len,
                         trsScale,
                         3);
      *comma = true;
    }
    return;
  }

  if (*comma)
    gltf_w_ch(w, ',');
  gltf_w_matrix(w, node);
  *comma = true;
}

static
bool
gltf_write_children(GLTFExpWriter * __restrict w,
                    GLTFExpState  * __restrict st,
                    GLTFExpNodeOut * __restrict out) {
  uint32_t i;

  if (out->childCount == 0)
    return false;

  gltf_w_key(w, _s_gltf_children, _s_gltf_children_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < out->childCount; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_w_uint(w, st->nodeChildren.items[out->childOffset + i]);
  }

  gltf_w_ch(w, ']');

  return true;
}

static
void
gltf_write_index_span(GLTFExpWriter * __restrict w,
                      const GLTFExpIndex * __restrict items,
                      uint32_t                   count) {
  uint32_t i;

  gltf_w_ch(w, '[');

  for (i = 0; i < count; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_w_uint(w, items[i]);
  }

  gltf_w_ch(w, ']');
}

static
void
gltf_write_node_weights(GLTFExpWriter * __restrict w,
                        AkFloatArray  * __restrict weights) {
  uint32_t i;

  gltf_w_key(w, _s_gltf_weights, _s_gltf_weights_len);
  gltf_w_ch(w, '[');
  for (i = 0; weights && i < weights->count; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_w_float(w, weights->items[i]);
  }
  gltf_w_ch(w, ']');
}

static
bool
gltf_write_instancing_attribute(GLTFExpWriter * __restrict w,
                                GLTFExpState  * __restrict st,
                                const char    * __restrict key,
                                size_t                     keyLen,
                                AkAccessor    * __restrict accessor) {
  GLTFExpIndex index;

  index = gltf_accessor_index(&st->accessors, accessor);
  if (index == GLTF_EXP_INDEX_NONE) {
    w->result = AK_ERR;
    return false;
  }

  gltf_w_key_uint(w, key, keyLen, index);

  return true;
}

static
void
gltf_write_node_extensions(GLTFExpWriter  * __restrict w,
                           GLTFExpState   * __restrict st,
                           GLTFExpNodeOut * __restrict out,
                           AkNode         * __restrict node) {
  AkGpuInstancing *instancing;
  bool comma;

  instancing = node->gpuInstancing;
  comma = false;
  gltf_w_key(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '{');

  if (out->hasLight) {
    gltf_w_key(w,
               _s_gltf_KHR_lights_punctual,
               _s_gltf_KHR_lights_punctual_len);
    gltf_w_ch(w, '{');
    gltf_w_key_uint(w, _s_gltf_light, _s_gltf_light_len, out->lightIndex);
    gltf_w_ch(w, '}');
    comma = true;
  }

  if (instancing) {
    bool attrComma;

    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w,
               _s_gltf_EXT_mesh_gpu_instancing,
               _s_gltf_EXT_mesh_gpu_instancing_len);
    gltf_w_ch(w, '{');
    gltf_w_key(w, _s_gltf_attributes, _s_gltf_attributes_len);
    gltf_w_ch(w, '{');
    attrComma = false;

    if (instancing->translation) {
      if (!gltf_write_instancing_attribute(w,
                                           st,
                                           _s_gltf_TRANSLATION,
                                           _s_gltf_TRANSLATION_len,
                                           instancing->translation))
        return;
      attrComma = true;
    }

    if (instancing->rotation) {
      if (attrComma)
        gltf_w_ch(w, ',');
      if (!gltf_write_instancing_attribute(w,
                                           st,
                                           _s_gltf_ROTATION,
                                           _s_gltf_ROTATION_len,
                                           instancing->rotation))
        return;
      attrComma = true;
    }

    if (instancing->scale) {
      if (attrComma)
        gltf_w_ch(w, ',');
      if (!gltf_write_instancing_attribute(w,
                                           st,
                                           _s_gltf_SCALE,
                                           _s_gltf_SCALE_len,
                                           instancing->scale))
        return;
    }

    gltf_w_ch(w, '}');
    gltf_w_ch(w, '}');
    comma = true;
  }

  if (!node->visible || out->forceVisibilityExtension) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w,
               _s_gltf_KHR_node_visibility,
               _s_gltf_KHR_node_visibility_len);
    gltf_w_ch(w, '{');
    gltf_w_key_bool(w, _s_gltf_visible, _s_gltf_visible_len, node->visible);
    gltf_w_ch(w, '}');
    comma = true;
  }

  gltf_write_extra_extension_entries(w,
                                     ak_extra(node),
                                     gltf_node_core_extension_skip,
                                     NULL,
                                     &comma);
  gltf_w_ch(w, '}');
}

void
gltf_write_nodes(GLTFExpWriter * __restrict w,
                 GLTFExpState  * __restrict st) {
  size_t i;

  gltf_w_key(w, _s_gltf_nodes, _s_gltf_nodes_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->nodes.count; i++) {
    GLTFExpNodeOut *out;
    AkNode         *node;
    bool            comma;

    if (i > 0)
      gltf_w_ch(w, ',');

    out   = &st->nodes.items[i];
    node  = out->node;
    comma = false;

    gltf_w_ch(w, '{');

    if (out->name) {
      gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, out->name);
      comma = true;
    }

    gltf_write_node_transform(w, node, out, &comma);

    if (out->hasMesh) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w, _s_gltf_mesh, _s_gltf_mesh_len, out->meshIndex);
      comma = true;
    }

    if (out->morphWeights && out->morphWeights->count > 0) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_write_node_weights(w, out->morphWeights);
      comma = true;
    }

    if (out->hasSkin) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w, _s_gltf_skin, _s_gltf_skin_len, out->skinIndex);
      comma = true;
    }

    if (out->hasCamera) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w,
                      _s_gltf_camera,
                      _s_gltf_camera_len,
                      out->cameraIndex);
      comma = true;
    }

    if (out->hasLight
        || node->gpuInstancing
        || !node->visible
        || out->forceVisibilityExtension
        || gltf_extra_has_extensions(ak_extra(node),
                                     gltf_node_core_extension_skip,
                                     NULL)) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_write_node_extensions(w, st, out, node);
      comma = true;
    }

    if (out->childCount > 0) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_write_children(w, st, out);
      comma = true;
    }

    if (gltf_extra_has_json_extras(ak_extra(node))) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key(w, _s_gltf_extras, _s_gltf_extras_len);
      gltf_write_extra_json_extras(w, ak_extra(node));
    }

    gltf_w_ch(w, '}');
  }

  gltf_w_ch(w, ']');
}

static
void
gltf_write_scene(GLTFExpWriter  * __restrict w,
                 GLTFExpState   * __restrict st,
                 GLTFExpSceneOut * __restrict out) {
  AkScene *scene;
  bool     comma;

  scene = out->scene;
  comma = false;
  gltf_w_ch(w, '{');

  if (scene && scene->name) {
    gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, scene->name);
    comma = true;
  }

  if (out->rootCount > 0) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_nodes, _s_gltf_nodes_len);
    gltf_write_index_span(w,
                          st->sceneRoots.items + out->rootOffset,
                          out->rootCount);
    comma = true;
  }

  gltf_write_extra_extensions_member(w, &comma, ak_extra(scene), NULL, NULL);

  if (gltf_extra_has_json_extras(ak_extra(scene))) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_extras, _s_gltf_extras_len);
    gltf_write_extra_json_extras(w, ak_extra(scene));
  }

  gltf_w_ch(w, '}');
}

void
gltf_write_scenes(GLTFExpWriter * __restrict w,
                  GLTFExpState  * __restrict st) {
  size_t i;

  gltf_w_key(w, _s_gltf_scenes, _s_gltf_scenes_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->scenes.count; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_write_scene(w, st, &st->scenes.items[i]);
  }

  gltf_w_ch(w, ']');

  if (st->scenes.count > 0) {
    gltf_w_ch(w, ',');
    gltf_w_key_uint(w, _s_gltf_scene, _s_gltf_scene_len,
                    st->defaultSceneIndex);
  }
}
