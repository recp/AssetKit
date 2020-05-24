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

#include "../common.h"
#include "../coord/common.h"

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
