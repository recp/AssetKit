/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__tree__h_
#define __libassetkit__tree__h_

#include "../include/ak/assetkit.h"
#include <libxml/tree.h>

/**
 * @brief load a tree from xml
 */
AkTreeNode* _assetkit_hide
tree_fromxml(AkHeap * __restrict heap,
             void   * __restrict memParent,
             xml_t  * __restrict xml);

#endif /* __libassetkit__tree__h_ */
