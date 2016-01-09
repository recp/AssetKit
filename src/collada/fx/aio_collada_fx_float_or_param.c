/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_float_or_param.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_common.h"
#include "../aio_collada_param.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_float_or_param(xmlNode * __restrict xml_node,
                        aio_fx_float_or_param ** __restrict dest) {
  xmlNode               * curr_node;
  aio_fx_float_or_param * float_or_param;

  float_or_param = aio_malloc(sizeof(*float_or_param));
  memset(float_or_param, '\0', sizeof(*float_or_param));

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "float")) {
        aio_basic_attrf * valuef;
        char            * sid;

        valuef = aio_malloc(sizeof(*valuef));
        memset(valuef, '\0', sizeof(*valuef));

        aio_xml_collada_read_attr(curr_node, "sid", &sid);

        if (sid)
          valuef->sid = sid;

        float_or_param->val = valuef;

      } else if (AIO_IS_EQ_CASE(node_name, "param")) {
        aio_param * param;
        aio_param * last_param;
        int         ret;

        param = NULL;
        last_param = (aio_param *)float_or_param->param;

        ret = aio_load_collada_param(curr_node,
                                     AIO_PARAM_TYPE_BASIC,
                                     &param);

        if (ret == 0) {
          if (last_param) {
            last_param->next = param;
            param->prev = last_param;
          } else {
            float_or_param->param = (aio_param_basic *)param;
          }
        }
      } /* if */
    }
    
    curr_node = curr_node->next;
  }
  
  *dest = float_or_param;
  
  return 0;
}
