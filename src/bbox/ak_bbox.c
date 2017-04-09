/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_bbox.h"
#include <cglm.h>

void
ak_bbox_pick(float min[3],
             float max[3],
             float vec[3]) {
  /* min */
  if (vec[0] < min[0])
    min[0] = vec[0];

  if (vec[1] < min[1])
    min[1] = vec[1];

  if (vec[2] < min[2])
    min[2] = vec[2];

  /* max */
  if (vec[0] > max[0])
    max[0] = vec[0];

  if (vec[1] > max[1])
    max[1] = vec[1];

  if (vec[2] > max[2])
    max[2] = vec[2];
}

void
ak_bbox_pick_pbox(AkBoundingBox *parent,
                  AkBoundingBox *chld) {
  /* min */
  if (chld->min[0] < parent->min[0])
    parent->min[0] = chld->min[0];

  if (chld->min[1] < parent->min[1])
    parent->min[1] = chld->min[1];

  if (chld->min[2] < parent->min[2])
    parent->min[2] = chld->min[2];

  /* max */
  if (chld->max[0] > parent->max[0])
    parent->max[0] = chld->max[0];

  if (chld->max[1] > parent->max[1])
    parent->max[1] = chld->max[1];

  if (chld->max[2] > parent->max[2])
    parent->max[2] = chld->max[2];
}

void
ak_bbox_pick_pbox2(AkBoundingBox *parent,
                   float vec1[3],
                   float vec2[3]) {
  /* min */
  if (vec1[0] < parent->min[0])
    parent->min[0] = vec1[0];

  if (vec1[1] < parent->min[1])
    parent->min[1] = vec1[1];

  if (vec1[2] < parent->min[2])
    parent->min[2] = vec1[2];

  if (vec2[0] < parent->min[0])
    parent->min[0] = vec2[0];

  if (vec2[1] < parent->min[1])
    parent->min[1] = vec2[1];

  if (vec2[2] < parent->min[2])
    parent->min[2] = vec2[2];

  /* max */
  if (vec1[0] > parent->max[0])
    parent->max[0] = vec1[0];

  if (vec1[1] > parent->max[1])
    parent->max[1] = vec1[1];

  if (vec1[2] > parent->max[2])
    parent->max[2] = vec1[2];

  if (vec2[0] > parent->max[0])
    parent->max[0] = vec2[0];

  if (vec2[1] > parent->max[1])
    parent->max[1] = vec2[1];

  if (vec2[2] > parent->max[2])
    parent->max[2] = vec2[2];
}

void
ak_bbox_center(AkBoundingBox * __restrict bbox,
               float center[3]) {
  glm_vec_sub(bbox->max, bbox->min, center);
  glm_vec_scale(center, 0.5f, center);
  glm_vec_add(bbox->min, center, center);
}

float
ak_bbox_radius(AkBoundingBox * __restrict bbox) {
  float v[3];
  glm_vec_sub(bbox->max, bbox->min, v);
  return glm_vec_norm(v) / 2.0f;
}
