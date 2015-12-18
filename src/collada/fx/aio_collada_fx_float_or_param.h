/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_float_or_param___h_
#define __libassetio__aio_collada_float_or_param___h_

#include "../../../include/assetio.h"

typedef struct _xmlNode xmlNode;

int _assetio_hide
aio_load_float_or_param(xmlNode * __restrict xml_node,
                        aio_fx_float_or_param ** __restrict dest);

#endif /* __libassetio__aio_collada_float_or_param___h_ */
