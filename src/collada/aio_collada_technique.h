/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_technique__h_
#define __libassetio__aio_collada_technique__h_

#include "../../include/assetio.h"

typedef struct _xmlNode xmlNode;

int _assetio_hide
aio_load_collada_technique(xmlNode * xml_node,
                           aio_technique ** technique);

int _assetio_hide
aio_load_collada_techniquec(xmlNode * xml_node,
                            aio_technique_common ** techniquec);

#endif /* __libassetio__aio_collada_technique__h_ */
