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

#ifndef assetkit_bbox_h
#define assetkit_bbox_h

#include "common.h"
#include <stdbool.h>

struct AkMesh;
struct AkMeshPrimitive;
struct AkGeometry;
struct AkScene;
struct AkMeshPrimitive;

typedef struct AkBoundingBox {
  float min[3];
  float max[3];
  bool  isvalid;
} AkBoundingBox;

/*!
 * @brief calc bbox for whole scene, this calc scene bbox with transformations
 *        of geom nodes
 *
 * @param scene visual scene
 */
void
ak_bbox_scene(struct AkVisualScene * __restrict scene);

/*!
 * @brief calc bbox for whole geometry
 *        this will affect scene bbox
 *
 * @param geom  geometry
 */
void
ak_bbox_geom(struct AkGeometry * __restrict geom);

/*!
 * @brief calc bbox for whole mesh
 *        this will affect geom bbox
 *
 * @param mesh  mesh
 */
void
ak_bbox_mesh(struct AkMesh * __restrict mesh);

/*!
 * @brief calc bbox for mesh primitive
 *        this will affect geom bbox and mesh bbox
 *
 * @param prim  primitive
 */
void
ak_bbox_mesh_prim(struct AkMeshPrimitive * __restrict prim);

/*!
 * @brief get center of bbox
 *
 * @param[in]  bbox   bbox
 * @param[out] center center of bbox
 */
void
ak_bbox_center(AkBoundingBox * __restrict bbox,
               float center[3]);

/*!
 * @brief returns radius of sphere which is surround bbox square
 *
 * @param bbox bbox
 *
 * @return radius (r) of outer sphere
 */
float
ak_bbox_radius(AkBoundingBox * __restrict bbox);

#endif /* assetkit_bbox_h */
