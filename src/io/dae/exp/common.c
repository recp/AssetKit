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

#include "common.h"

#include <cglm/cglm.h>

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

AK_HIDE
AkGeometry*
dae_instance_geometry_object(AkInstanceGeometry * __restrict inst) {
  void *obj;

  if (!inst)
    return NULL;
  if (inst->base.object)
    return inst->base.object;

  obj = ak_instanceObject(&inst->base);
  return obj && ak_typeid(obj) == AKT_GEOMETRY ? obj : NULL;
}

AK_HIDE
AkCamera*
dae_instance_camera_object(AkInstanceBase * __restrict inst) {
  void *obj;

  if (!inst)
    return NULL;
  if (inst->object)
    return inst->object;

  obj = ak_instanceObject(inst);
  return obj && ak_typeid(obj) == AKT_CAMERA ? obj : NULL;
}

AK_HIDE
AkLight*
dae_instance_light_object(AkInstanceBase * __restrict inst) {
  void *obj;

  if (!inst)
    return NULL;
  if (inst->object)
    return inst->object;

  obj = ak_instanceObject(inst);
  return obj && ak_typeid(obj) == AKT_LIGHT ? obj : NULL;
}

AK_HIDE
uint32_t
dae_map_index(RBTree * __restrict map, void * __restrict key) {
  uintptr_t idx;

  idx = (uintptr_t)rb_find(map, key);
  return idx ? (uint32_t)(idx - 1u) : UINT32_MAX;
}

AK_HIDE
bool
dae_w_node_id_ref(DAEExpState * __restrict st,
                  AkNode      * __restrict node) {
  const char *id;
  uint32_t idx;

  id = node ? ak_getId(node) : NULL;
  if (id && *id && rb_find(st->nodeIds, (void *)id) == node) {
    dae_w_xml(&st->w, id, true);
    return true;
  }

  idx = dae_map_index(st->nodes, node);
  if (idx != UINT32_MAX) {
    dae_w_node_id(&st->w, idx);
    return true;
  }

  idx = dae_map_index(st->visualNodes, node);
  if (idx != UINT32_MAX) {
    dae_w_vnode_id(&st->w, idx);
    return true;
  }

  return false;
}

AK_HIDE
AkNode*
dae_node_for_transform(DAEExpState * __restrict st,
                       AkObject    * __restrict transform) {
  return st && transform ? rb_find(st->nodeTransforms, transform) : NULL;
}

static
const char*
dae_transform_default_sid(AkObject * __restrict transform) {
  if (!transform)
    return NULL;

  switch ((AkTypeId)transform->type) {
    case AKT_MATRIX:     return "matrix";
    case AKT_LOOKAT:     return "lookat";
    case AKT_ROTATE:
    case AKT_QUATERNION: return "rotation";
    case AKT_SCALE:      return "scale";
    case AKT_SKEW:       return "skew";
    case AKT_TRANSLATE:  return "translation";
    default:             return NULL;
  }
}

AK_HIDE
const char*
dae_transform_sid(AkObject * __restrict transform) {
  const char *sid;

  sid = transform ? ak_sid_get(transform) : NULL;
  if (sid && *sid)
    return sid;

  return dae_transform_default_sid(transform);
}

AK_HIDE
void
dae_w_transform_sid(DAEExpWriter * __restrict w,
                    AkObject     * __restrict transform) {
  const char *sid;

  sid = dae_transform_sid(transform);
  if (sid)
    dae_w_xml(w, sid, true);
}

AK_HIDE
void
dae_w_node_id(DAEExpWriter * __restrict w, uint32_t nodeIdx) {
  dae_w_id(w, DAE_EXP_NAME(node), nodeIdx);
}

AK_HIDE
void
dae_w_vnode_id(DAEExpWriter * __restrict w, uint32_t vnodeIdx) {
  dae_w_id(w, DAE_EXP_NAME_LIT("vnode"), vnodeIdx);
}

AK_HIDE
bool
dae_prepare_extra_object(RBTree             * __restrict map,
                         uint32_t           * __restrict count,
                         DAEExpObjectRef   ** __restrict first,
                         DAEExpObjectRef   ** __restrict last,
                         void               * __restrict object) {
  DAEExpObjectRef *ref;

  if (!map || !count || !first || !last || !object)
    return false;

  if (dae_map_index(map, object) != UINT32_MAX)
    return true;

  if (*count == UINT32_MAX)
    return false;

  ref = calloc(1, sizeof(*ref));
  if (!ref)
    return false;

  ref->object = object;
  rb_insert(map, object, (void *)(uintptr_t)(++(*count)));

  if (*last)
    (*last)->next = ref;
  else
    *first = ref;
  *last = ref;

  return true;
}

AK_HIDE
void
dae_w_geom_id(DAEExpWriter * __restrict w, uint32_t geomIdx) {
  dae_w_id(w, DAE_EXP_NAME_LIT("geom"), geomIdx);
}

AK_HIDE
void
dae_quat_axis_angle_deg(AkQuaternion * __restrict quat,
                        float                      axis[3],
                        float        * __restrict angleDeg) {
  versor q;
  float  len2;
  float  angle;

  if (!quat) {
    axis[0]   = 0.0f;
    axis[1]   = 0.0f;
    axis[2]   = 1.0f;
    *angleDeg = 0.0f;
    return;
  }

  q[0] = quat->val[0];
  q[1] = quat->val[1];
  q[2] = quat->val[2];
  q[3] = quat->val[3];
  len2 = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];

  if (!isfinite(len2) || len2 <= FLT_EPSILON) {
    axis[0]   = 0.0f;
    axis[1]   = 0.0f;
    axis[2]   = 1.0f;
    *angleDeg = 0.0f;
    return;
  }

  glm_quat_normalize(q);
  if (q[3] < 0.0f)
    glm_vec4_negate(q);

  angle = glm_quat_angle(q);
  if (!isfinite(angle) || fabsf(angle) <= FLT_EPSILON) {
    axis[0]   = 0.0f;
    axis[1]   = 0.0f;
    axis[2]   = 1.0f;
    *angleDeg = 0.0f;
    return;
  }

  glm_quat_axis(q, axis);
  *angleDeg = glm_deg(angle);
}

AK_HIDE
void
dae_w_prim_material_symbol(DAEExpWriter * __restrict w, uint32_t primIdx) {
  dae_w_lit(w, "mat_");
  dae_w_uint_fast(w, primIdx);
}

AK_HIDE
void
dae_w_geom_prim_id(DAEExpWriter * __restrict w,
                   uint32_t                  geomIdx,
                   uint32_t                  primIdx,
                   DAEExpName                suffix) {
  dae_w_geom_id(w, geomIdx);
  dae_w_lit(w, "_prim_");
  dae_w_uint_fast(w, primIdx);
  if (suffix.ptr && suffix.len > 0) {
    dae_w_ch(w, '_');
    dae_w_name(w, suffix);
  }
}

AK_HIDE
void
dae_write_float_elem(DAEExpWriter * __restrict w,
                     DAEExpName                tag,
                     float                     val) {
  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
  dae_w_float_fast(w, val);
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
}

AK_HIDE
void
dae_w_matrix4x4_dae(DAEExpWriter * __restrict w,
                    const AkFloat             matrix[4][4]) {
  int r;
  int c;

  for (r = 0; r < 4; r++) {
    for (c = 0; c < 4; c++) {
      if (r > 0 || c > 0)
        dae_w_ch(w, ' ');
      dae_w_float_fast(w, matrix[c][r]);
    }
  }
}

AK_HIDE
void
dae_w_identity4x4(DAEExpWriter * __restrict w) {
  static const AkFloat ident[4][4] = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f}
  };

  dae_w_matrix4x4_dae(w, ident);
}
