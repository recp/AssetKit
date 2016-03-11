/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__tree__h_
#define __libassetio__tree__h_

#include "../include/assetio.h"

typedef struct _xmlNode xmlNode;

/**
 * @brief load a tree from xml
 */
int _assetio_hide
aio_tree_fromXmlNode(xmlNode * __restrict xml_node,
                       aio_tree_node ** __restrict dest,
                       aio_tree_node * __restrict parent);

#endif /* __libassetio__tree__h_ */
