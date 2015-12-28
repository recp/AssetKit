/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_annotate.h"

#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_memory.h"
#include "../aio_utils.h"
#include "../aio_tree.h"

#include "aio_collada_value.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_annotate(xmlNode * __restrict xml_node,
                          aio_annotate ** __restrict dest) {
  xmlNode      * curr_node;
  xmlAttr      * curr_attr;
  aio_annotate * annotate;
  int            ret;

  curr_node = xml_node;
  annotate = aio_malloc(sizeof(*annotate));
  memset(annotate, '\0', sizeof(*annotate));

  curr_attr = curr_node->properties;

  /* parse attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "name"))
        annotate->name = aio_strdup(attr_val);

      break;
    }

    curr_attr = curr_attr->next;
  }

  ret = aio_load_collada_value(curr_node->children,
                               &annotate->val,
                               &annotate->val_type);

  if (ret == 0)
    *dest = annotate;

  return 0;
}
