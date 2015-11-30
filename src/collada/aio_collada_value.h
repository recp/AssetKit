/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_value__h_
#define __libassetio__aio_collada_value__h_

#include "../../include/assetio.h"

typedef struct _xmlNode xmlNode;

int _assetio_hide
aio_load_collada_value(xmlNode * __restrict xml_node,
                       void ** __restrict dest,
                       aio_value_type * __restrict val_type);

#endif /* __libassetio__aio_collada_value__h_ */
