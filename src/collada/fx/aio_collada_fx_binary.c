/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_binary.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_common.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_dae_fxBinary(xmlNode * __restrict xml_node,
                 aio_binary ** __restrict dest) {
  xmlNode * curr_node;
  aio_binary * binary;

  binary = aio_malloc(sizeof(*binary));
  memset(binary, '\0', sizeof(*binary));

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {
    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "ref")) {
      binary->ref = aio_strdup(aio_xml_content(curr_node));
    } else if (AIO_IS_EQ_CASE(node_name, "hex")) {
      xmlAttr      * curr_attr;
      aio_hex_data * hex;

      hex = aio_malloc(sizeof(*hex));
      memset(hex, '\0', sizeof(*hex));

      for (curr_attr = xml_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "format")) {
          hex->format = aio_strdup(attr_val);
          
          break;
        }
      }

      curr_attr = NULL;

      /* hex data should specify format */
      if (!hex->format) {
        aio_free(hex);
        goto err;
      }

      hex->val = aio_strdup(aio_xml_content(curr_node));
    }
  }

  *dest = binary;
  return 0;
err:
  return -1;
}


