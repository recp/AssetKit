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

/*!
 * @brief Axis-aligned bounds represented by minimum and maximum corners.
 *
 * The coordinate space depends on the owner: primitive, mesh, and geometry
 * boxes are geometry-local; a scene box includes node world transforms.
 */
typedef struct AkBoundingBox {
  float min[3];
  float max[3];
  bool  isvalid;
} AkBoundingBox;

/*!
 * @brief Recompute a scene's world-space bounding box.
 *
 * Geometry instances and instance-node references are traversed with their
 * composed node transforms. The result is stored in scene->bbox.
 * Skin/morph deformation and GPU-instancing rows are not evaluated.
 *
 * @param[in,out] scene A nullable scene to update.
 */
void
ak_bbox_scene(struct AkScene * __restrict scene);

/*!
 * @brief Recompute the local bounding box of a mesh geometry.
 *
 * The mesh, its primitives, and geom->bbox are refreshed together. Non-mesh
 * geometry payloads are left unchanged.
 *
 * @param[in,out] geom A nullable geometry to update.
 */
void
ak_bbox_geom(struct AkGeometry * __restrict geom);

/*!
 * @brief Recompute a mesh and its owning geometry's local bounds.
 *
 * Every primitive is recalculated and merged into mesh->bbox and the owning
 * geometry's box. Mesh and primitive center fields are updated as well.
 *
 * @param[in,out] mesh A nullable mesh to update.
 */
void
ak_bbox_mesh(struct AkMesh * __restrict mesh);

/*!
 * @brief Compute one primitive's local bounds from referenced positions.
 *
 * POSITION data must contain float components. Convert preserved integer
 * positions with ak_accessorMakeFloat() before calling this helper.
 *
 * The result is stored in prim->bbox and merged into its mesh and geometry
 * boxes. Use ak_bbox_mesh() when previously computed parent bounds must be
 * rebuilt after an edit.
 *
 * @param[in,out] prim A nullable primitive to update.
 */
void
ak_bbox_mesh_prim(struct AkMeshPrimitive * __restrict prim);

/*!
 * @brief Return the midpoint of a bounding box.
 *
 * @param[in] bbox The bounding box to read.
 * @param[out] center Receives the midpoint of min and max.
 */
void
ak_bbox_center(AkBoundingBox * __restrict bbox,
               float center[3]);

/*!
 * @brief Return the radius of a sphere enclosing a bounding box.
 *
 * @param[in] bbox The bounding box to read.
 * @return Half the distance from min to max.
 */
float
ak_bbox_radius(AkBoundingBox * __restrict bbox);

#endif /* assetkit_bbox_h */
