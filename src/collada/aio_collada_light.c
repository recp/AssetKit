/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_light.h"
#include "aio_collada_asset.h"
#include "aio_collada_technique.h"
#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_memory.h"
#include "../aio_utils.h"
#include "../aio_tree.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_light(xmlNode * __restrict xml_node,
                       aio_light ** __restrict  dest) {
  xmlNode   * curr_node;
  xmlAttr   * curr_attr;
  aio_light * light;

  curr_node = xml_node;
  light = aio_malloc(sizeof(*light));
  curr_attr = curr_node->properties;

  /* parse camera attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "id"))
        light->id = aio_strdup(attr_val);
      else if (AIO_IS_EQ_CASE(attr_name, "name"))
        light->name = aio_strdup(attr_val);

    }

    curr_attr = curr_attr->next;
  }

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "asset")) {

        _AIO_ASSET_LOAD_TO(curr_node,
                           light->inf);

      } else if (AIO_IS_EQ_CASE(node_name, "technique_common")) {

        aio_technique_common * technique_c;
        int                    ret;

        ret = aio_load_collada_techniquec(curr_node,
                                          &technique_c);

        if (ret == 0)
          light->technique_common = technique_c;

      } else if (AIO_IS_EQ_CASE(node_name, "technique")) {
        /* TODO: */
      } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
        _AIO_TREE_LOAD_TO(curr_node->children,
                          light->extra,
                          NULL);
      }
    }
    
    curr_node = curr_node->next;
  }
  
  return -1;
}
