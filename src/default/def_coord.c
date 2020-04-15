/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../coord_sys/coord_common.h"

/* Right Hand (Default) */
AkCoordSys AK__Y_RH_VAL =         {AK__Y_RH, AK_AXIS_ROT_DIR_RH, AK__Y_RH};
AkCoordSys *AK_YUP      = &AK__Y_RH_VAL;
AkCoordSys *AK_ZUP      = AK_COORD(AK__Z_RH, AK_AXIS_ROT_DIR_RH, AK__Y_RH);
AkCoordSys *AK_XUP      = AK_COORD(AK__X_RH, AK_AXIS_ROT_DIR_RH, AK__Y_RH);

/* Left Hand */
AkCoordSys *AK_ZUP_LH   = AK_COORD(AK__Z_LH, AK_AXIS_ROT_DIR_LH, AK__Y_LH);
AkCoordSys *AK_YUP_LH   = AK_COORD(AK__Y_LH, AK_AXIS_ROT_DIR_LH, AK__Y_LH);
AkCoordSys *AK_XUP_LH   = AK_COORD(AK__X_LH, AK_AXIS_ROT_DIR_LH, AK__Y_LH);

AkCoordSys *AK_DEFAULT_COORD = &AK__Y_RH_VAL;
