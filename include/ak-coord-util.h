/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_coord_util_h
#define ak_coord_util_h
#ifdef __cplusplus
extern "C" {
#endif

struct AkNode;
struct AkVisualScene;

AK_EXPORT
void
ak_changeCoordSys(AkDoc * __restrict doc,
                  AkCoordSys * newCoordSys);

AK_EXPORT
void
ak_changeCoordSysGeom(AkGeometry * __restrict geom,
                      AkCoordSys * newCoordSys);

AK_EXPORT
void
ak_changeCoordSysMesh(AkMesh * __restrict mesh,
                      AkCoordSys * newCoordSys);

AK_EXPORT
void
ak_fixNodeCoordSys(struct AkNode * __restrict node);

AK_EXPORT
void
ak_fixSceneCoordSys(struct AkVisualScene * __restrict scene);

#ifdef __cplusplus
}
#endif
#endif /* ak_coord_util_h */
