/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_color_h_
#define __libassetkit__ak_collada_color_h_

#include "../../include/assetkit.h"
#include "ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_color(xmlTextReaderPtr reader,
              bool read_sid,
              AkColor * __restrict dest);

#endif /* __libassetkit__ak_collada_color_h_ */
