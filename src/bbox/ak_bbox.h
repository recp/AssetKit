/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_src_bbox_h
#define ak_src_bbox_h

#include "../ak_common.h"
#include "../../include/ak/bbox.h"

void
ak_bbox_pick(float min[3],
             float max[3],
             float vec[3]);

void
ak_bbox_pick_pbox(AkBoundingBox *parent,
                  AkBoundingBox *chld);

void
ak_bbox_pick_pbox2(AkBoundingBox *parent,
                   float vec1[3],
                   float vec2[3]);

#endif /* ak_src_bbox_h */
