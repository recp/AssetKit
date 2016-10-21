/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"

#define AK__Z_RH AK_AXIS_POSITIVE_X, AK_AXIS_POSITIVE_Z, AK_AXIS_POSITIVE_Y
#define AK__Y_RH AK_AXIS_POSITIVE_X, AK_AXIS_POSITIVE_Y, AK_AXIS_NEGATIVE_Z
#define AK__X_RH AK_AXIS_NEGATIVE_Y, AK_AXIS_POSITIVE_X, AK_AXIS_NEGATIVE_Z

#define AK__Z_LH AK_AXIS_POSITIVE_X, AK_AXIS_POSITIVE_Z, AK_AXIS_NEGATIVE_Y
#define AK__Y_LH AK_AXIS_POSITIVE_X, AK_AXIS_POSITIVE_Y, AK_AXIS_POSITIVE_Z
#define AK__X_LH AK_AXIS_NEGATIVE_Y, AK_AXIS_POSITIVE_X, AK_AXIS_POSITIVE_Z

AkCoordSys AK__Y_RH_VAL = {AK__Y_RH};

/* Right Hand (Default) */
AkCoordSys * AK_YUP    = &AK__Y_RH_VAL;
AkCoordSys * AK_ZUP    = &(AkCoordSys){AK__Z_RH};
AkCoordSys * AK_XUP    = &(AkCoordSys){AK__X_RH};

/* Left Hand */
AkCoordSys * AK_ZUP_LH = &(AkCoordSys){AK__Z_LH};
AkCoordSys * AK_YUP_LH = &(AkCoordSys){AK__Y_LH};
AkCoordSys * AK_XUP_LH = &(AkCoordSys){AK__X_LH};

AkCoordSys * AK_DEFAULT_COORD = &AK__Y_RH_VAL;

AK_EXPORT
void
ak_defaultSetCoordSys(AkCoordSys * coordsys) {
  AK_DEFAULT_COORD = coordsys;
}

AK_EXPORT
AkCoordSys *
ak_defaultCoordSys() {
  return AK_DEFAULT_COORD;
}
