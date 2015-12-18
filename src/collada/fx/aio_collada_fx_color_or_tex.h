/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_color_or_tex__h_
#define __libassetio__aio_collada_color_or_tex__h_

#include "../../../include/assetio.h"

typedef struct _xmlNode xmlNode;

int _assetio_hide
aio_load_color_or_tex(xmlNode * __restrict xml_node,
                      aio_fx_color_or_tex ** __restrict dest);

#endif /* __libassetio__aio_collada_color_or_tex__h_ */
