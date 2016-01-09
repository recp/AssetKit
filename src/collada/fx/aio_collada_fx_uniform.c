/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_uniform.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_common.h"
#include "../aio_collada_param.h"
#include "../aio_collada_value.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_dae_fxBindUniform(xmlNode * __restrict xml_node,
                      aio_bind_uniform ** __restrict dest) {
  xmlNode * curr_node;
  xmlAttr * curr_attr;
  aio_bind_uniform * bind_uniform;
  aio_param * last_param;

  bind_uniform = aio_malloc(sizeof(*bind_uniform));
  memset(bind_uniform, '\0', sizeof(*bind_uniform));

  last_param = NULL;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "symbol")) {
      bind_uniform->symbol = aio_strdup(attr_val);
      break;
    }
  }

  curr_attr = NULL;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {
    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "param")) {
      aio_param * param;
      int         ret;

      param = NULL;
      ret = aio_load_collada_param(curr_node,
                                   AIO_PARAM_TYPE_BASIC,
                                   &param);

      if (ret == 0) {
        if (last_param) {
          last_param->next = param;
          param->prev = last_param;
        } else {
          bind_uniform->param = (aio_param_basic *)param;
        }

        last_param = param;
      }
    } else {
      void * val;
      aio_value_type val_type;
      int ret;

      ret = aio_load_collada_value(curr_node,
                                   &val,
                                   &val_type);

      if (ret == 0) {
        bind_uniform->val = val;
        bind_uniform->val_type = val_type;
      }
    }
  }

  *dest = bind_uniform;

  return 0;
}
