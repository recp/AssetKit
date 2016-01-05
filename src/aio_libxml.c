/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_libxml.h"
#include <stdlib.h>

inline
char *
aio_xml_node_content(xmlNode * xml_node) {
  if (!xml_node)
    return NULL;

  if (!xml_node->children)
    return (char *)xml_node->content;

  return (char *)xml_node->children->content;
}