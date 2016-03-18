/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__tree__h_
#define __libassetkit__tree__h_

#include "../include/assetkit.h"

typedef struct _xmlNode xmlNode;

/**
 * @brief load a tree from xml
 */
int _assetkit_hide
ak_tree_fromXmlNode(void * __restrict memParent,
                     xmlNode * __restrict xml_node,
                     ak_tree_node ** __restrict dest,
                     ak_tree_node * __restrict parent);

#endif /* __libassetkit__tree__h_ */
