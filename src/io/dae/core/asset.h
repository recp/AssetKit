/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_asset_h
#define dae_asset_h

#include "../common.h"

_assetkit_hide
AkAssetInf*
dae_asset(DAEState   * __restrict dst,
          xml_t      * __restrict xml,
          void       * __restrict memp,
          AkAssetInf * __restrict pinf);

#endif /* dae_asset_h */
