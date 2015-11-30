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
aio_tree_load_from_xml(xmlNode * __restrict xml_node,
                       aio_tree_node ** __restrict dest,
                       aio_tree_node * __restrict parent);


#define _AIO_TREE_LOAD_TO(NODE, TREE, PARENT)                                 \
  do {                                                                        \
    aio_tree_node * tree_root_node;                                           \
    int             ret;                                                      \
                                                                              \
    tree_root_node = NULL;                                                    \
                                                                              \
    ret = aio_tree_load_from_xml(NODE,                                        \
                                 &tree_root_node,                             \
                                 PARENT);                                     \
                                                                              \
    if (ret == 0)                                                             \
      TREE = tree_root_node;                                                  \
  } while(0)

#endif /* __libassetio__tree__h_ */
