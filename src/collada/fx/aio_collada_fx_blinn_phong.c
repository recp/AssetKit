/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_blinn_phong.h"

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
aio_load_blinn_phong(xmlNode * __restrict xml_node,
                     aio_blinn ** __restrict dest) {
  xmlNode   * curr_node;
  aio_blinn * blinn;

  curr_node = xml_node;
  blinn = aio_malloc(sizeof(*blinn));
  memset(blinn, '\0', sizeof(*blinn));

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
          blinn->emission = color_or_tex;

      } else if (AIO_IS_EQ_CASE(node_name, "ambient")) {
        aio_fx_color_or_tex * color_or_tex;
        int ret;

        color_or_tex = NULL;
        ret = aio_load_color_or_tex(curr_node, &color_or_tex);

        if (ret == 0)
          blinn->ambient = color_or_tex;

      } else if (AIO_IS_EQ_CASE(node_name, "diffuse")) {
        aio_fx_color_or_tex * color_or_tex;
        int ret;

        color_or_tex = NULL;
        ret = aio_load_color_or_tex(curr_node, &color_or_tex);

        if (ret == 0)
          blinn->diffuse = color_or_tex;

      } else if (AIO_IS_EQ_CASE(node_name, "specular")) {
        aio_fx_color_or_tex * color_or_tex;
        int ret;

        color_or_tex = NULL;
        ret = aio_load_color_or_tex(curr_node, &color_or_tex);

        if (ret == 0)
          blinn->specular = color_or_tex;

      } else if (AIO_IS_EQ_CASE(node_name, "shininess")) {
        aio_fx_float_or_param * float_or_param;
        int ret;

        float_or_param = NULL;
        ret = aio_load_float_or_param(curr_node, &float_or_param);

        if (ret == 0)
          blinn->shininess = float_or_param;

      } else if (AIO_IS_EQ_CASE(node_name, "reflective")) {
        aio_fx_color_or_tex * color_or_tex;
        int ret;

        color_or_tex = NULL;
        ret = aio_load_color_or_tex(curr_node, &color_or_tex);

        if (ret == 0)
          blinn->reflective = color_or_tex;

      } else if (AIO_IS_EQ_CASE(node_name, "reflectivity")) {
        aio_fx_float_or_param * float_or_param;
        int ret;

        float_or_param = NULL;
        ret = aio_load_float_or_param(curr_node, &float_or_param);

        if (ret == 0)
          blinn->reflectivity = float_or_param;

      } else if (AIO_IS_EQ_CASE(node_name, "transparent")) {
        aio_fx_color_or_tex * color_or_tex;
        int ret;

        color_or_tex = NULL;
        ret = aio_load_color_or_tex(curr_node, &color_or_tex);

        if (ret == 0)
          blinn->transparent = color_or_tex;

      } else if (AIO_IS_EQ_CASE(node_name, "transparency")) {
        aio_fx_float_or_param * float_or_param;
        int ret;

        float_or_param = NULL;
        ret = aio_load_float_or_param(curr_node, &float_or_param);

        if (ret == 0)
          blinn->transparency = float_or_param;

      } else if (AIO_IS_EQ_CASE(node_name, "index_of_refraction")) {
        aio_fx_float_or_param * float_or_param;
        int ret;

        float_or_param = NULL;
        ret = aio_load_float_or_param(curr_node, &float_or_param);

        if (ret == 0)
          blinn->index_of_refraction = float_or_param;
      }
    }

    curr_node = curr_node->next;
  }
  
  *dest = blinn;
  
  return 0;
}
