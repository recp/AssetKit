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

#ifndef ak_src_bbox_h
#define ak_src_bbox_h

#include "../common.h"
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
