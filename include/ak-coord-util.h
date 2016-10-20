/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_coord_util_h
#define ak_coord_util_h

AK_EXPORT
void
ak_changeCoordSys(AkDoc * __restrict mesh,
                  AkCoordSys * newCoordSys);

AK_EXPORT
void
ak_changeCoordSysGeom(AkDoc * __restrict mesh,
                      AkCoordSys * newCoordSys);

AK_EXPORT
void
ak_changeCoordSysMesh(AkMesh * __restrict mesh,
                      AkCoordSys * newCoordSys);

#endif /* ak_coord_util_h */
