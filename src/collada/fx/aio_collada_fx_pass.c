/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_pass.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_asset.h"
#include "../aio_collada_common.h"
#include "../aio_collada_annotate.h"

#include "aio_collada_fx_states.h"
#include "aio_collada_fx_program.h"
#include "aio_collada_fx_evaluate.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_dae_fxPass(xmlTextReaderPtr __restrict reader,
               aio_pass ** __restrict dest) {
  return 0;
}

int _assetio_hide
aio_dae_fxPass0(xmlNode * __restrict xml_node,
               aio_pass ** __restrict dest) {
  xmlNode  * curr_node;
  xmlAttr  * curr_attr;
  aio_pass * pass;

  pass = aio_malloc(sizeof(*pass));
  memset(pass, '\0', sizeof(*pass));

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    const char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "sid")) {
      pass->sid = aio_strdup(attr_val);
      break;
    }
  }

  for (curr_node = xml_node;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {
    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "asset")) {

      _AIO_ASSET_LOAD_TO(curr_node,
                         pass->inf);

    } else if (AIO_IS_EQ_CASE(node_name, "annotate")) {
      aio_annotate * annotate;
      aio_annotate * last_annotate;
      int            ret;

//      ret = aio_load_collada_annotate(curr_node, &annotate);

      if (ret == 0) {
        last_annotate = pass->annotate;
        if (last_annotate) {
//          annotate->prev = last_annotate;
          last_annotate->next = annotate;
        } else {
          pass->annotate = annotate;
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "states")) {
      aio_states * states;
      int          ret;

      states = NULL;
      ret = aio_dae_fxState(curr_node, &states);

      if (ret == 0)
        pass->states = states;

    } else if (AIO_IS_EQ_CASE(node_name, "program")) {
      aio_program * prog;
      int           ret;

      prog = NULL;
      ret = aio_dae_fxProg(curr_node, &prog);

      if (ret == 0)
        pass->program = prog;

    } else if (AIO_IS_EQ_CASE(node_name, "evaluate")) {
      aio_evaluate * evaluate;
      int ret;

      evaluate = NULL;
      ret = aio_dae_fxEvaluate(curr_node, &evaluate);

      if (ret == 0)
        pass->evaluate = evaluate;

    } else if (AIO_IS_EQ_CASE(node_name, "extra")) {

      _AIO_TREE_LOAD_TO(curr_node->children,
                        pass->extra,
                        NULL);
      
    }
  }
  
  *dest = pass;
  
  return 0;
}
