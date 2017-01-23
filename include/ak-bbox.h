/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_bbox_h
#define ak_bbox_h

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
 * @brief calc bbox for whole scene
 *
 * @param scene visual scene
 */
void
ak_bbox_scene(struct AkVisualScene * __restrict scene);

/*!
 * @brief calc bbox for whole geometry
 *        this will affect scene bbox
 *
 * @param scene visual scene
 * @param geom  geometry
 */
void
ak_bbox_geom(struct AkVisualScene * __restrict scene,
             struct AkGeometry    * __restrict geom);

/*!
 * @brief calc bbox for whole mesh
 *        this will affect scene bbox and geom bbox
 *
 * @param scene visual scene
 * @param geom  geometry
 * @param mesh  mesh
 */
void
ak_bbox_mesh(struct AkVisualScene * __restrict scene,
             struct AkGeometry    * __restrict geom,
             struct AkMesh        * __restrict mesh);

/*!
 * @brief calc bbox for mesh primitive
 *        this will affect scene bbox, geom bbox and mesh bbox
 *
 * @param scene visual scene
 * @param geom  geometry
 * @param mesh  mesh
 * @param prim  primitive
 */
void
ak_bbox_mesh_prim(struct AkVisualScene   * __restrict scene,
                  struct AkGeometry      * __restrict geom,
                  struct AkMesh          * __restrict mesh,
                  struct AkMeshPrimitive * __restrict prim);

#endif /* ak_bbox_h */
