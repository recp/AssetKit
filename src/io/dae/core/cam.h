/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_camera_h
#define dae_camera_h

#include "../common.h"

_assetkit_hide
void*
dae_cam(DAEState * __restrict dst,
        xml_t    * __restrict xml,
        void     * __restrict memp);

#endif /* dae_camera_h */
