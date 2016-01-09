/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_sampler.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_common.h"
#include "../aio_collada_color.h"

#include "aio_collada_fx_color_or_tex.h"
#include "aio_collada_fx_float_or_param.h"
#include "aio_collada_fx_image.h"
#include "aio_collada_fx_enums.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_dae_fxSampler(xmlNode * __restrict xml_node,
                  aio_fx_sampler_common ** __restrict dest) {
  xmlNode * curr_node;
  aio_fx_sampler_common * sampler;

  sampler = aio_malloc(sizeof(*sampler));
  memset(sampler, '\0', sizeof(*sampler));

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "instance_image")) {
      aio_image_instance * image_inst;
      int ret;

      image_inst = NULL;
      ret = aio_load_collada_image_instance(curr_node, &image_inst);

      if (ret == 0)
        sampler->image_inst = image_inst;

    } else if (AIO_IS_EQ_CASE(node_name, "texcoord")) {
      xmlAttr * curr_attr;

      for (curr_attr = xml_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "texture")) {
          sampler->texcoord.semantic = aio_strdup(attr_val);
          break;
        }
      }
    } else if (AIO_IS_EQ_CASE(node_name, "wrap_s")) {
      char * node_content;
      node_content = aio_xml_content(curr_node);
      sampler->wrap_s = aio_dae_fxEnumWrap(node_content);
    } else if (AIO_IS_EQ_CASE(node_name, "wrap_t")) {
      char * node_content;
      node_content = aio_xml_content(curr_node);
      sampler->wrap_t = aio_dae_fxEnumWrap(node_content);
    } else if (AIO_IS_EQ_CASE(node_name, "wrap_p")) {
      char * node_content;
      node_content = aio_xml_content(curr_node);
      sampler->wrap_p = aio_dae_fxEnumWrap(node_content);
    } else if (AIO_IS_EQ_CASE(node_name, "minfilter")) {
      char * node_content;
      node_content = aio_xml_content(curr_node);
      sampler->minfilter = aio_dae_fxEnumMinfilter(node_content);
    } else if (AIO_IS_EQ_CASE(node_name, "magfilter")) {
      char * node_content;
      node_content = aio_xml_content(curr_node);
      sampler->magfilter = aio_dae_fxEnumMagfilter(node_content);
    } else if (AIO_IS_EQ_CASE(node_name, "mipfilter")) {
      char * node_content;
      node_content = aio_xml_content(curr_node);
      sampler->mipfilter = aio_dae_fxEnumMipfilter(node_content);
    } else if (AIO_IS_EQ_CASE(node_name, "border_color")) {
      aio_color * color;
      color = aio_malloc(sizeof(*color));
      memset(color, '\0', sizeof(*color));

      aio_dae_color(curr_node, 1, color);
    } else if (AIO_IS_EQ_CASE(node_name, "mip_max_level")) {
      char * node_content;
      node_content = aio_xml_content(curr_node);

      sampler->mip_max_level = strtoul(node_content, NULL, 10);
    } else if (AIO_IS_EQ_CASE(node_name, "mip_min_level")) {
      char * node_content;
      node_content = aio_xml_content(curr_node);

      sampler->mip_max_level = strtoul(node_content, NULL, 10);
    } else if (AIO_IS_EQ_CASE(node_name, "mip_bias")) {
      char * node_content;
      node_content = aio_xml_content(curr_node);

      sampler->mip_max_level = strtoul(node_content, NULL, 10);
    } else if (AIO_IS_EQ_CASE(node_name, "max_anisotropy")) {
      char * node_content;
      char * tmp;

      node_content = aio_xml_content(curr_node);

      sampler->mip_max_level = strtoul(node_content, &tmp, 10);

      if (*tmp == '\0')
        sampler->mip_max_level = 1;
    } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
      aio_tree_node * extra;
      aio_tree_node * last_extra;
      int             ret;

      extra = NULL;
      last_extra = sampler->extra;

      ret = aio_tree_load_from_xml(curr_node, &extra, NULL);
      if (ret == 0) {
        if (last_extra) {
          last_extra->next = extra;
          extra->prev = last_extra;
        } else {
          sampler->extra = extra;
        }
      }
    }
  } /* for */

  *dest = sampler;

  return 0;
}
