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
#include "controller.h"
#include "material.h"
#include "source.h"

#include <cglm/cglm.h>

#include <float.h>
#include <math.h>

static
void
dae_write_node(DAEExpState * __restrict st,
               AkNode      * __restrict node,
               uint32_t                 libNodeIdx);

static
void
dae_write_instance_geometry(DAEExpState        * __restrict st,
                            AkInstanceGeometry * __restrict inst) {
  DAEExpWriter *w;
  AkGeometry   *geom;
  uint32_t      geomIdx;

  geom = dae_instance_geometry_object(inst);
  geomIdx = geom ? dae_map_index(st->geometries, geom) : UINT32_MAX;
  if (geomIdx == UINT32_MAX)
    return;

  w = &st->w;
  dae_w_lit(w, "<instance_geometry url=\"#");
  dae_w_geom_id(w, geomIdx);
  dae_w_lit(w, "\">");
  dae_write_instance_materials(st, geom, inst);
  dae_write_extra(w, inst ? inst->base.extra : NULL);
  dae_w_lit(w, "</instance_geometry>");
}

static
void
dae_write_instance_camera(DAEExpState   * __restrict st,
                          AkInstanceBase * __restrict inst) {
  DAEExpWriter *w;
  AkCamera     *camera;
  uint32_t      cameraIdx;

  camera    = dae_instance_camera_object(inst);
  cameraIdx = camera ? dae_map_index(st->cameras, camera) : UINT32_MAX;
  if (cameraIdx == UINT32_MAX)
    return;

  w = &st->w;
  dae_w_lit(w, "<instance_camera url=\"#");
  dae_w_id(w, DAE_EXP_NAME(camera), cameraIdx);
  if (inst && inst->extra) {
    dae_w_lit(w, "\">");
    dae_write_extra(w, inst->extra);
    dae_w_lit(w, "</instance_camera>");
  } else {
    dae_w_lit(w, "\"/>");
  }
}

static
void
dae_write_instance_light(DAEExpState   * __restrict st,
                         AkInstanceBase * __restrict inst) {
  DAEExpWriter *w;
  AkLight      *light;
  uint32_t      lightIdx;

  light    = dae_instance_light_object(inst);
  lightIdx = light ? dae_map_index(st->lights, light) : UINT32_MAX;
  if (lightIdx == UINT32_MAX)
    return;

  w = &st->w;
  dae_w_lit(w, "<instance_light url=\"#");
  dae_w_id(w, DAE_EXP_NAME(light), lightIdx);
  if (inst && inst->extra) {
    dae_w_lit(w, "\">");
    dae_write_extra(w, inst->extra);
    dae_w_lit(w, "</instance_light>");
  } else {
    dae_w_lit(w, "\"/>");
  }
}

static
void
dae_write_instance_node_ref(DAEExpState    * __restrict st,
                            AkInstanceNode * __restrict nodeInst) {
  DAEExpWriter *w;
  AkNode       *target;
  uint32_t      targetIdx;

  if (!nodeInst)
    return;

  target    = ak_instanceNodeTarget(nodeInst);
  targetIdx = target ? dae_map_index(st->nodes, target) : UINT32_MAX;
  if (targetIdx == UINT32_MAX) {
    dae_write_node(st, target, UINT32_MAX);
    return;
  }

  w = &st->w;
  dae_w_lit(w, "<instance_node");
  if (nodeInst->name) {
    dae_w_lit(w, " name=\"");
    dae_w_xml(w, nodeInst->name, true);
    dae_w_ch(w, '"');
  }
  dae_w_lit(w, " url=\"#");
  if (!dae_w_node_id_ref(st, target))
    dae_w_node_id(w, targetIdx);
  dae_w_lit(w, "\"/>");
}

static
void
dae_write_node_start(DAEExpState * __restrict st,
                     AkNode      * __restrict node,
                     uint32_t                 libNodeIdx,
                     bool                     forceGeneratedId) {
  DAEExpWriter *w;

  w = &st->w;
  dae_w_lit(w, "<node id=\"");
  if (forceGeneratedId || !dae_w_node_id_ref(st, node)) {
    if (!forceGeneratedId && libNodeIdx != UINT32_MAX) {
      dae_w_node_id(w, libNodeIdx);
    } else {
      dae_w_vnode_id(w, st->visualNodeCount++);
    }
  }
  if (node && node->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, node->name, true);
  }
  if (node && node->nodeType == AK_NODE_TYPE_JOINT)
    dae_w_lit(w, "\" type=\"JOINT");
  dae_w_lit(w, "\">");
}

static
void
dae_write_transform_sid_attr(DAEExpWriter * __restrict w,
                             AkObject     * __restrict obj) {
  const char *sid;

  sid = dae_transform_sid(obj);
  if (!sid)
    return;

  dae_w_lit(w, " sid=\"");
  dae_w_xml(w, sid, true);
  dae_w_ch(w, '"');
}

static
void
dae_write_transform_item(DAEExpState * __restrict st,
                         AkObject    * __restrict obj) {
  DAEExpWriter *w;

  if (!obj)
    return;

  w = &st->w;
  switch ((AkTypeId)obj->type) {
    case AKT_MATRIX: {
      AkMatrix *matrix;

      matrix = ak_objGet(obj);
      dae_w_lit(w, "<matrix");
      dae_write_transform_sid_attr(w, obj);
      dae_w_ch(w, '>');
      dae_w_matrix4x4_dae(w, matrix->val);
      dae_w_lit(w, "</matrix>");
      break;
    }
    case AKT_LOOKAT: {
      AkLookAt *lookAt;
      uint32_t i;

      lookAt = ak_objGet(obj);
      dae_w_lit(w, "<lookat");
      dae_write_transform_sid_attr(w, obj);
      dae_w_ch(w, '>');
      for (i = 0; i < 9; i++) {
        if (i > 0)
          dae_w_ch(w, ' ');
        dae_w_float_fast(w, ((float *)lookAt->val)[i]);
      }
      dae_w_lit(w, "</lookat>");
      break;
    }
    case AKT_ROTATE: {
      AkRotate *rotate;

      rotate = ak_objGet(obj);
      dae_w_lit(w, "<rotate");
      dae_write_transform_sid_attr(w, obj);
      dae_w_ch(w, '>');
      dae_w_float_fast(w, rotate->val[0]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, rotate->val[1]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, rotate->val[2]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, glm_deg(rotate->val[3]));
      dae_w_lit(w, "</rotate>");
      break;
    }
    case AKT_QUATERNION: {
      AkQuaternion *quat;
      vec3          axis;
      float         angleDeg;

      quat = ak_objGet(obj);
      dae_quat_axis_angle_deg(quat, axis, &angleDeg);
      dae_w_lit(w, "<rotate");
      dae_write_transform_sid_attr(w, obj);
      dae_w_ch(w, '>');
      dae_w_float_fast(w, axis[0]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, axis[1]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, axis[2]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, angleDeg);
      dae_w_lit(w, "</rotate>");
      break;
    }
    case AKT_SCALE: {
      AkScale *scale;

      scale = ak_objGet(obj);
      dae_w_lit(w, "<scale");
      dae_write_transform_sid_attr(w, obj);
      dae_w_ch(w, '>');
      dae_w_float_fast(w, scale->val[0]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, scale->val[1]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, scale->val[2]);
      dae_w_lit(w, "</scale>");
      break;
    }
    case AKT_SKEW: {
      AkSkew *skew;

      skew = ak_objGet(obj);
      dae_w_lit(w, "<skew");
      dae_write_transform_sid_attr(w, obj);
      dae_w_ch(w, '>');
      dae_w_float_fast(w, glm_deg(skew->angle));
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, skew->rotateAxis[0]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, skew->rotateAxis[1]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, skew->rotateAxis[2]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, skew->aroundAxis[0]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, skew->aroundAxis[1]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, skew->aroundAxis[2]);
      dae_w_lit(w, "</skew>");
      break;
    }
    case AKT_TRANSLATE: {
      AkTranslate *translate;

      translate = ak_objGet(obj);
      dae_w_lit(w, "<translate");
      dae_write_transform_sid_attr(w, obj);
      dae_w_ch(w, '>');
      dae_w_float_fast(w, translate->val[0]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, translate->val[1]);
      dae_w_ch(w, ' ');
      dae_w_float_fast(w, translate->val[2]);
      dae_w_lit(w, "</translate>");
      break;
    }
    default:
      break;
  }
}

static
void
dae_write_node_transform(DAEExpState  * __restrict st,
                         AkNode       * __restrict node,
                         const AkFloat             matrixOverride[4][4]) {
  AkObject *item;

  if (matrixOverride) {
    dae_w_lit(&st->w, "<matrix>");
    dae_w_matrix4x4_dae(&st->w, matrixOverride);
    dae_w_lit(&st->w, "</matrix>");
    return;
  }

  if (!node || !node->transform)
    return;

  for (item = node->transform->base; item; item = item->next)
    dae_write_transform_item(st, item);

  for (item = node->transform->item; item; item = item->next)
    dae_write_transform_item(st, item);
}

static
void
dae_write_node_payload(DAEExpState * __restrict st,
                       AkNode      * __restrict node,
                       const AkFloat            matrixOverride[4][4]) {
  AkInstanceGeometry *geomInst;
  AkInstanceBase     *baseInst;
  AkInstanceNode     *nodeInst;
  AkNode             *child;
  dae_write_node_transform(st, node, matrixOverride);

  for (geomInst = node->geometry; geomInst; geomInst = (void *)geomInst->base.next) {
    if (dae_instance_controller_exportable(st, geomInst))
      dae_write_instance_controller(st, geomInst);
    else
      dae_write_instance_geometry(st, geomInst);
  }

  for (baseInst = node->camera; baseInst; baseInst = baseInst->next)
    dae_write_instance_camera(st, baseInst);

  for (baseInst = node->light; baseInst; baseInst = baseInst->next)
    dae_write_instance_light(st, baseInst);

  for (child = node->chld; child; child = child->next)
    dae_write_node(st, child, dae_map_index(st->nodes, child));

  for (nodeInst = node->node; nodeInst; nodeInst = nodeInst->next) {
    AkNode *target;

    target = ak_instanceNodeTarget(nodeInst);
    if (target && target->gpuInstancing)
      dae_write_node(st, target, UINT32_MAX);
    else
      dae_write_instance_node_ref(st, nodeInst);
  }

  dae_write_extra(&st->w, node->extra);
}

static
void
dae_instancing_read_vec3(AkAccessor * __restrict acc,
                         uint32_t                index,
                         float                   def,
                         vec3                    out) {
  const float *row;

  if (!acc) {
    out[0] = def;
    out[1] = def;
    out[2] = def;
    return;
  }

  row    = io_accessor_float_row(acc, index);
  out[0] = row[0];
  out[1] = row[1];
  out[2] = row[2];
}

static
void
dae_instancing_read_quat(AkAccessor * __restrict acc,
                         uint32_t                index,
                         versor                  out) {
  const float *row;

  if (!acc) {
    glm_quat_identity(out);
    return;
  }

  row    = io_accessor_float_row(acc, index);
  out[0] = row[0];
  out[1] = row[1];
  out[2] = row[2];
  out[3] = row[3];

  if (!isfinite(out[0]) || !isfinite(out[1]) || !isfinite(out[2])
      || !isfinite(out[3])
      || (out[0] * out[0] + out[1] * out[1]
          + out[2] * out[2] + out[3] * out[3]) <= FLT_EPSILON) {
    glm_quat_identity(out);
    return;
  }

  glm_quat_normalize(out);
}

static
void
dae_instancing_matrix(AkNode          * __restrict node,
                      AkGpuInstancing * __restrict instancing,
                      uint32_t                     index,
                      AkFloat                      out[4][4]) {
  mat4   local = GLM_MAT4_IDENTITY_INIT;
  mat4   instance;
  mat4   translateMat;
  mat4   rotateMat;
  mat4   scaleMat;
  vec3   translation;
  vec3   scale;
  versor rotation;

  dae_instancing_read_vec3(instancing->translation, index, 0.0f, translation);
  dae_instancing_read_quat(instancing->rotation, index, rotation);
  dae_instancing_read_vec3(instancing->scale, index, 1.0f, scale);

  glm_translate_make(translateMat, translation);
  glm_quat_mat4(rotation, rotateMat);
  glm_scale_make(scaleMat, scale);
  glm_mat4_mul(translateMat, rotateMat, instance);
  glm_mat4_mul(instance, scaleMat, instance);

  if (node->transform) {
    ak_transformCombine(node->transform, local[0]);
    glm_mat4_mul(local, instance, (vec4 *)out);
  } else {
    glm_mat4_copy(instance, (vec4 *)out);
  }
}

static
void
dae_write_gpu_instanced_node(DAEExpState * __restrict st,
                             AkNode      * __restrict node,
                             uint32_t                 libNodeIdx) {
  AkGpuInstancing *instancing;
  uint32_t         i;

  instancing = node->gpuInstancing;
  for (i = 0; i < instancing->count; i++) {
    AkFloat matrix[4][4];

    dae_instancing_matrix(node, instancing, i, matrix);
    dae_write_node_start(st, node, i == 0 ? libNodeIdx : UINT32_MAX, i > 0);
    dae_write_node_payload(st, node, matrix);
    dae_w_lit(&st->w, "</node>");
  }
}

static
void
dae_write_node(DAEExpState * __restrict st,
               AkNode      * __restrict node,
               uint32_t                 libNodeIdx) {
  if (!node)
    return;

  if (node->gpuInstancing) {
    dae_write_gpu_instanced_node(st, node, libNodeIdx);
    return;
  }

  dae_write_node_start(st, node, libNodeIdx, false);
  dae_write_node_payload(st, node, NULL);
  dae_w_lit(&st->w, "</node>");
}

static
void
dae_write_root_instance_node(DAEExpState    * __restrict st,
                             AkInstanceNode * __restrict nodeInst) {
  DAEExpWriter *w;
  AkNode       *target;
  uint32_t      targetIdx;

  if (!nodeInst)
    return;

  target = ak_instanceNodeTarget(nodeInst);
  if (target && target->gpuInstancing) {
    dae_write_node(st, target, UINT32_MAX);
    return;
  }

  targetIdx = target ? dae_map_index(st->nodes, target) : UINT32_MAX;
  if (targetIdx == UINT32_MAX) {
    dae_write_node(st, target, UINT32_MAX);
    return;
  }

  w = &st->w;
  dae_w_lit(w, "<node id=\"");
  dae_w_vnode_id(w, st->visualNodeCount++);
  dae_w_lit(w, "\">");
  dae_write_instance_node_ref(st, nodeInst);
  dae_w_lit(w, "</node>");
}

AK_HIDE
void
dae_write_visual_scene(DAEExpState * __restrict st,
                       AkScene     * __restrict scene,
                       uint32_t                 sceneIdx) {
  DAEExpWriter   *w;
  AkNode         *root;
  AkNode         *node;
  AkInstanceNode *nodeInst;

  w = &st->w;
  dae_w_lit(w, "<visual_scene id=\"");
  dae_w_id(w, DAE_EXP_NAME(scene), sceneIdx);
  if (scene && scene->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, scene->name, true);
  }
  dae_w_lit(w, "\">");

  root = scene ? scene->node : NULL;
  if (root) {
    for (node = root->chld; node; node = node->next)
      dae_write_node(st, node, dae_map_index(st->nodes, node));
    for (nodeInst = root->node; nodeInst; nodeInst = nodeInst->next)
      dae_write_root_instance_node(st, nodeInst);
  }

  dae_write_extra(w, scene ? scene->extra : NULL);
  dae_w_lit(w, "</visual_scene>");
}

AK_HIDE
void
dae_write_library_nodes(DAEExpState * __restrict st) {
  AkNode  *node;
  uint32_t idx;
  bool     any;

  if (st->nodeCount == 0)
    return;

  any = false;
  for (node = st->doc->lib.nodes.first; node; node = node->docNext) {
    if (!rb_find(st->sceneNodes, node)) {
      any = true;
      break;
    }
  }

  if (!any)
    return;

  dae_w_lit(&st->w, "<library_nodes>");
  idx = 0;
  for (node = st->doc->lib.nodes.first; node; node = node->docNext) {
    if (rb_find(st->sceneNodes, node)) {
      idx++;
      continue;
    }
    dae_write_node(st, node, idx++);
  }
  dae_w_lit(&st->w, "</library_nodes>\n");
}

AK_HIDE
uint32_t
dae_active_scene_index(AkDoc * __restrict doc) {
  AkScene *scene;
  uint32_t idx;

  if (!doc || !doc->scene)
    return 0;

  idx = 0;
  for (scene = doc->lib.scenes.first; scene; scene = scene->next, idx++) {
    if (scene == doc->scene)
      return idx;
  }

  return 0;
}
