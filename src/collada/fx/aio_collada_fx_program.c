/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_program.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_common.h"
#include "aio_collada_fx_shader.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_dae_fxProg(xmlNode * __restrict xml_node,
               aio_program ** __restrict dest) {
  xmlNode * curr_node;
  aio_program * prog;

  prog = aio_malloc(sizeof(*prog));
  memset(prog, '\0', sizeof(*prog));

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {
    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "shader")) {
      aio_shader * shader;
      int          ret;

      shader = NULL;
      ret = aio_dae_fxShader(curr_node, &shader);

      if (ret == 0)
        prog->shader = shader;

    } else if (AIO_IS_EQ_CASE(node_name, "linker")) {
    } else if (AIO_IS_EQ_CASE(node_name, "bind_attribute")) {
    } else if (AIO_IS_EQ_CASE(node_name, "bind_uniform")) {
    }
  }

  *dest = prog;
  
  return 0;
}
