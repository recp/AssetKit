/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_common.h"

#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_memory.h"
#include "../aio_utils.h"
#include "../aio_tree.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

inline
int
aio_xml_collada_read_attr(xmlNode * __restrict xml_node,
                          const char * __restrict attr_name,
                          char ** __restrict val) {

  xmlAttr    * curr_attr;
  const char * curr_attr_name;

  curr_attr = xml_node->properties;
  while (curr_attr) {
    curr_attr_name = (const char *)curr_attr->name;
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      if (AIO_IS_EQ_CASE(curr_attr_name, attr_name)) {
        const char * attr_val;

        attr_val = aio_xml_content((xmlNode *)curr_attr);
        *val = aio_strdup(attr_val);

        /* single attr */
        break;
      }
    } /* if elm */
  } /* while */
  
  return 0;
}

inline
int
aio_xml_collada_read_id_name(xmlNode * __restrict xml_node,
                             const char ** __restrict id,
                             const char ** __restrict name) {

  xmlAttr    * curr_attr;
  const char * curr_attr_name;

  curr_attr = xml_node->properties;
  while (curr_attr) {
    curr_attr_name = (const char *)curr_attr->name;
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      if (AIO_IS_EQ_CASE(curr_attr_name, "id")) {
        const char * attr_val;

        attr_val = aio_xml_content((xmlNode *)curr_attr);
        *id = aio_strdup(attr_val);
      } else if (AIO_IS_EQ_CASE(curr_attr_name, "name")) {
        const char * attr_val;

        attr_val = aio_xml_content((xmlNode *)curr_attr);
        *name = aio_strdup(attr_val);
      }
    } /* if elm */

    curr_attr = curr_attr->next;
  } /* while */
  
  return 0;
}
