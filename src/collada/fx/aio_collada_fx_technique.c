/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_technique.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_asset.h"
#include "../aio_collada_common.h"
#include "../aio_collada_annotate.h"
#include "aio_collada_fx_blinn_phong.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_technique_fx(xmlNode * __restrict xml_node,
                              aio_technique_fx ** __restrict dest) {
  xmlNode          * curr_node;
  xmlAttr          * curr_attr;
  aio_technique_fx * technique_fx;

  curr_node = xml_node;
  technique_fx = aio_malloc(sizeof(*technique_fx));
  memset(technique_fx, '\0', sizeof(*technique_fx));

  curr_attr = curr_node->properties;

  /* parse camera attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "id"))
        technique_fx->id = aio_strdup(attr_val);
      else if (AIO_IS_EQ_CASE(attr_name, "sid"))
        technique_fx->sid = aio_strdup(attr_val);
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
                           technique_fx->inf);

      } else if (AIO_IS_EQ_CASE(node_name, "annotate")) {
        aio_annotate * annotate;
        aio_annotate * last_annotate;
        int            ret;

        ret = aio_load_collada_annotate(curr_node, &annotate);

        if (ret == 0) {
          last_annotate = technique_fx->annotate;
          if (last_annotate) {
            annotate->prev = last_annotate;
            last_annotate->next = annotate;
          } else {
            technique_fx->annotate = annotate;
          }

        }
      } else if (AIO_IS_EQ_CASE(node_name, "pass")) {
      } else if (AIO_IS_EQ_CASE(node_name, "blinn")) {
        aio_blinn * blinn;
        int         ret;

        blinn = NULL;

        ret = aio_load_blinn_phong(curr_node, &blinn);
        if (ret == 0)
          technique_fx->blinn = blinn;
        
      } else if (AIO_IS_EQ_CASE(node_name, "constant")) {
      } else if (AIO_IS_EQ_CASE(node_name, "lambert")) {
      } else if (AIO_IS_EQ_CASE(node_name, "phong")) {
        aio_blinn * blinn;
        int         ret;

        blinn = NULL;

        ret = aio_load_blinn_phong(curr_node, &blinn);
        if (ret == 0)
          technique_fx->phong = (aio_phong *)blinn;

      } else if (AIO_IS_EQ_CASE(node_name, "extra")) {

        _AIO_TREE_LOAD_TO(curr_node->children,
                          technique_fx->extra,
                          NULL);

      }
    }
    
    curr_node = curr_node->next;
  }

  *dest = technique_fx;

  return 0;
}
