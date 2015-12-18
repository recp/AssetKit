/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_constant.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_common.h"
#include "aio_collada_fx_color_or_tex.h"
#include "aio_collada_fx_float_or_param.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_constant_fx(xmlNode * __restrict xml_node,
                     aio_constant_fx ** __restrict dest) {
  xmlNode         * curr_node;
  aio_constant_fx * constant_fx;

  curr_node = xml_node;
  constant_fx = aio_malloc(sizeof(*constant_fx));
  memset(constant_fx, '\0', sizeof(*constant_fx));

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "emission")) {
        aio_fx_color_or_tex * color_or_tex;
        int ret;

        color_or_tex = NULL;
        ret = aio_load_color_or_tex(curr_node, &color_or_tex);

        if (ret == 0)
          constant_fx->emission = color_or_tex;

      } else if (AIO_IS_EQ_CASE(node_name, "reflective")) {
        aio_fx_color_or_tex * color_or_tex;
        int ret;

        color_or_tex = NULL;
        ret = aio_load_color_or_tex(curr_node, &color_or_tex);

        if (ret == 0)
          constant_fx->reflective = color_or_tex;

      } else if (AIO_IS_EQ_CASE(node_name, "reflectivity")) {
        aio_fx_float_or_param * float_or_param;
        int ret;

        float_or_param = NULL;
        ret = aio_load_float_or_param(curr_node, &float_or_param);

        if (ret == 0)
          constant_fx->reflectivity = float_or_param;

      } else if (AIO_IS_EQ_CASE(node_name, "transparent")) {
        aio_fx_color_or_tex * color_or_tex;
        int ret;

        color_or_tex = NULL;
        ret = aio_load_color_or_tex(curr_node, &color_or_tex);

        if (ret == 0)
          constant_fx->transparent = color_or_tex;

      } else if (AIO_IS_EQ_CASE(node_name, "transparency")) {
        aio_fx_float_or_param * float_or_param;
        int ret;

        float_or_param = NULL;
        ret = aio_load_float_or_param(curr_node, &float_or_param);

        if (ret == 0)
          constant_fx->transparency = float_or_param;

      } else if (AIO_IS_EQ_CASE(node_name, "index_of_refraction")) {
        aio_fx_float_or_param * float_or_param;
        int ret;

        float_or_param = NULL;
        ret = aio_load_float_or_param(curr_node, &float_or_param);
        
        if (ret == 0)
          constant_fx->index_of_refraction = float_or_param;
      }
    }
    
    curr_node = curr_node->next;
  }
  
  *dest = constant_fx;
  
  return 0;
}
