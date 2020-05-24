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
