/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_param.h"

#include "aio_collada_asset.h"
#include "aio_collada_technique.h"
#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_memory.h"
#include "../aio_utils.h"
#include "../aio_tree.h"

#include "aio_collada_value.h"
#include "aio_collada_annotate.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_newparam(xmlNode * __restrict xml_node,
                          aio_newparam ** __restrict dest) {
  xmlNode      * curr_node;
  aio_newparam * newparam;

  curr_node = xml_node;
  newparam = aio_malloc(sizeof(*newparam));
  memset(newparam, '\0', sizeof(*newparam));

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "annotate")) {
        aio_annotate * annotate;
        aio_annotate * last_annotate;
        int            ret;

        ret = aio_load_collada_annotate(curr_node, &annotate);

        if (ret == 0) {
          last_annotate = newparam->annotate;
          if (last_annotate) {
            annotate->prev = last_annotate;
            last_annotate->next = annotate;
          } else {
            newparam->annotate = annotate;
          }
          
        }
      } else if (AIO_IS_EQ_CASE(node_name, "semantic")) {
        const char * node_content;

        node_content = aio_xml_node_content(curr_node);
        newparam->semantic = aio_strdup(node_content);
      } else if (AIO_IS_EQ_CASE(node_name, "modifier")) {
        const char * node_content;

        node_content = aio_xml_node_content(curr_node);

        newparam->modifier = strtol(node_content, NULL, 10);
      }

      /* look for parameter type element */
      else {
        /* load once */
        if (!newparam->val) {
          void           * val;
          aio_value_type   val_type;
          int              ret;

          ret = aio_load_collada_value(curr_node,
                                       &val,
                                       &val_type);

          if (ret == 0) {
            newparam->val = val;
            newparam->val_type = val_type;
          }
        }
      }
    }

    curr_node = curr_node->next;
  } /* while */

  *dest = newparam;

  return 0;
}
