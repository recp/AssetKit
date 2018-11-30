/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "bbox.h"
#include <cglm/cglm.h>

void
ak_bbox_pick(vec3 min, vec3 max, vec3 vec) {
  glm_vec3_minv(min, vec, min);
  glm_vec3_maxv(max, vec, max);
}

void
ak_bbox_pick_pbox(AkBoundingBox *parent,
                  AkBoundingBox *chld) {
  glm_vec3_minv(parent->min, chld->min, parent->min);
  glm_vec3_maxv(parent->max, chld->max, parent->max);
}

void
ak_bbox_pick_pbox2(AkBoundingBox *parent,
                   float vec1[3],
                   float vec2[3]) {
  glm_vec3_minv(parent->min, vec1, parent->min);
  glm_vec3_minv(parent->min, vec2, parent->min);

  glm_vec3_maxv(parent->max, vec1, parent->max);
  glm_vec3_maxv(parent->max, vec2, parent->max);
}

void
ak_bbox_center(AkBoundingBox * __restrict bbox,
               float center[3]) {
  glm_vec3_center(bbox->min, bbox->max, center);
}

float
ak_bbox_radius(AkBoundingBox * __restrict bbox) {
  return glm_vec3_distance(bbox->max, bbox->min) * 0.5f;
}
