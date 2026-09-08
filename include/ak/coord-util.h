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

#ifndef assetkit_coord_util_h
#define assetkit_coord_util_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

struct AkNode;
struct AkScene;

/*!
 * @brief Bake a coordinate-system change into the document's geometry and scene data.
 *
 * Gaussian splats convert position, quaternion rotation, principal-axis scales,
 * and complete RGB spherical-harmonic bands through degree 4 together. Opacity
 * and the DC coefficient remain unchanged. Shared accessors are converted once;
 * SH inputs that alias other SH inputs receive independent coefficient storage.
 * Read-only mapped buffers are copied before modification.
 * Updates the document's coordinate system and invalidates cached bounds.
 */
AK_EXPORT
void
ak_changeCoordSys(AkDoc * __restrict doc,
                  AkCoordSys * newCoordSys);

/*!
 * @brief Bake geometry, including Gaussian splats, into a new coordinate system.
 *
 * Uses the owning document's coordinate system as the source. Does not change
 * the document's coordinate metadata or the transforms of geometry instances.
 */
AK_EXPORT
void
ak_changeCoordSysGeom(AkGeometry * __restrict geom,
                      AkCoordSys * newCoordSys);

/*!
 * @brief Bake mesh attributes, including Gaussian rotation, scale and SH bands.
 *
 * Uses the owning document's coordinate system as the source, without changing
 * document metadata or instance transforms. Invalidates mesh and primitive bounds.
 */
AK_EXPORT
void
ak_changeCoordSysMesh(AkMesh * __restrict mesh,
                      AkCoordSys * newCoordSys);

AK_EXPORT
void
ak_fixNodeCoordSys(struct AkNode * __restrict node);

AK_EXPORT
void
ak_fixSceneCoordSys(struct AkScene * __restrict scene);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_coord_util_h */
