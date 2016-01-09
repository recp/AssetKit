/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_evaluate.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_common.h"
#include "../aio_collada_param.h"
#include "../aio_collada_color.h"

#include "aio_collada_fx_enums.h"
#include "aio_collada_fx_image.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_dae_fxEvaluate(xmlNode * __restrict xml_node,
                   aio_evaluate ** __restrict dest) {
  xmlNode          * curr_node;
  aio_evaluate     * evaluate;

  evaluate = aio_malloc(sizeof(*evaluate));
  memset(evaluate, '\0', sizeof(*evaluate));

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {
    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "color_target")) {
      xmlNode * prev_node;
      xmlAttr * curr_attr;
      aio_evaluate_target * evaluate_target;

      evaluate_target = aio_malloc(sizeof(*evaluate_target));
      memset(evaluate_target, '\0', sizeof(*evaluate_target));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "index")) {
          evaluate_target->index = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(node_name, "slice")) {
          evaluate_target->slice = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(node_name, "mip")) {
          evaluate_target->mip = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(node_name, "face")) {
          long val;

          val = aio_dae_fxEnumFace(attr_val);
          if (val != -1)
            evaluate_target->face = val;
        }
      }

      curr_attr = NULL;
      prev_node = curr_node;

      for (curr_node = curr_node->children;
           curr_node && curr_node->type == XML_ELEMENT_NODE;
           curr_node = curr_node->next) {
        node_name = (const char *)curr_node->name;
        if (AIO_IS_EQ_CASE(node_name, "param")) {
          aio_param * param;
          int         ret;

          param = NULL;
          ret = aio_load_collada_param(curr_node,
                                       AIO_PARAM_TYPE_BASIC,
                                       &param);

          if (ret == 0)
            evaluate_target->param = param;

        } else if (AIO_IS_EQ_CASE(node_name, "instance_image")) {
          aio_image_instance * image_inst;
          int ret;

          image_inst = NULL;
          ret = aio_load_collada_image_instance(curr_node, &image_inst);

          if (ret == 0)
            evaluate_target->image_inst = image_inst;
        }
      }
      
      node_name = NULL;
      curr_node = prev_node;

      evaluate->color_target = evaluate_target;

    } else if (AIO_IS_EQ_CASE(node_name, "depth_target")) {
      xmlNode * prev_node;
      xmlAttr * curr_attr;
      aio_evaluate_target * evaluate_target;

      evaluate_target = aio_malloc(sizeof(*evaluate_target));
      memset(evaluate_target, '\0', sizeof(*evaluate_target));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "index")) {
          evaluate_target->index = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(node_name, "slice")) {
          evaluate_target->slice = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(node_name, "mip")) {
          evaluate_target->mip = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(node_name, "face")) {
          long val;

          val = aio_dae_fxEnumFace(attr_val);
          if (val != -1)
            evaluate_target->face = val;
        }
      }

      curr_attr = NULL;
      prev_node = curr_node;

      for (curr_node = curr_node->children;
           curr_node && curr_node->type == XML_ELEMENT_NODE;
           curr_node = curr_node->next) {
        node_name = (const char *)curr_node->name;
        if (AIO_IS_EQ_CASE(node_name, "param")) {
          aio_param * param;
          int         ret;

          param = NULL;
          ret = aio_load_collada_param(curr_node,
                                       AIO_PARAM_TYPE_BASIC,
                                       &param);

          if (ret == 0)
            evaluate_target->param = param;

        } else if (AIO_IS_EQ_CASE(node_name, "instance_image")) {
          aio_image_instance * image_inst;
          int ret;

          image_inst = NULL;
          ret = aio_load_collada_image_instance(curr_node, &image_inst);

          if (ret == 0)
            evaluate_target->image_inst = image_inst;
        }
      }

      node_name = NULL;
      curr_node = prev_node;
      
      evaluate->depth_target = evaluate_target;

    } else if (AIO_IS_EQ_CASE(node_name, "stencil_target")) {
      xmlNode * prev_node;
      xmlAttr * curr_attr;
      aio_evaluate_target * evaluate_target;

      evaluate_target = aio_malloc(sizeof(*evaluate_target));
      memset(evaluate_target, '\0', sizeof(*evaluate_target));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "index")) {
          evaluate_target->index = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(node_name, "slice")) {
          evaluate_target->slice = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(node_name, "mip")) {
          evaluate_target->mip = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(node_name, "face")) {
          long val;

          val = aio_dae_fxEnumFace(attr_val);
          if (val != -1)
            evaluate_target->face = val;
        }
      }

      curr_attr = NULL;
      prev_node = curr_node;

      for (curr_node = curr_node->children;
           curr_node && curr_node->type == XML_ELEMENT_NODE;
           curr_node = curr_node->next) {
        node_name = (const char *)curr_node->name;
        if (AIO_IS_EQ_CASE(node_name, "param")) {
          aio_param * param;
          int         ret;

          param = NULL;
          ret = aio_load_collada_param(curr_node,
                                       AIO_PARAM_TYPE_BASIC,
                                       &param);

          if (ret == 0)
            evaluate_target->param = param;

        } else if (AIO_IS_EQ_CASE(node_name, "instance_image")) {
          aio_image_instance * image_inst;
          int ret;

          image_inst = NULL;
          ret = aio_load_collada_image_instance(curr_node, &image_inst);

          if (ret == 0)
            evaluate_target->image_inst = image_inst;
        }
      }

      node_name = NULL;
      curr_node = prev_node;
      
      evaluate->stencil_target = evaluate_target;

    } else if (AIO_IS_EQ_CASE(node_name, "color_clear")) {
      xmlAttr * curr_attr;
      aio_color_clear * color_clear;

      color_clear = aio_malloc(sizeof(*color_clear));
      memset(color_clear, '\0', sizeof(*color_clear));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "index")) {
          color_clear->index = strtoul(attr_val, NULL, 10);
          break;
        }
      }

      curr_attr = NULL;
      aio_dae_color(curr_node, 0, &color_clear->val);

      evaluate->color_clear = color_clear;

    } else if (AIO_IS_EQ_CASE(node_name, "depth_clear")) {
      xmlAttr * curr_attr;
      aio_depth_clear * depth_clear;

      depth_clear = aio_malloc(sizeof(*depth_clear));
      memset(depth_clear, '\0', sizeof(*depth_clear));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "index")) {
          depth_clear->index = strtoul(attr_val, NULL, 10);
          break;
        }
      }

      curr_attr = NULL;
      depth_clear->val = strtof(aio_xml_content(curr_node), NULL);

      evaluate->depth_clear = depth_clear;

    } else if (AIO_IS_EQ_CASE(node_name, "stencil_clear")) {
      xmlAttr * curr_attr;
      aio_stencil_clear * stencil_clear;

      stencil_clear = aio_malloc(sizeof(*stencil_clear));
      memset(stencil_clear, '\0', sizeof(*stencil_clear));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "index")) {
          stencil_clear->index = strtoul(attr_val, NULL, 10);
          break;
        }
      }

      curr_attr = NULL;
      stencil_clear->val = strtoul(aio_xml_content(curr_node), NULL, 10);

      evaluate->stencil_clear = stencil_clear;

    } else if (AIO_IS_EQ_CASE(node_name, "draw")) {
      const char * node_val;
      long val;

      node_val = aio_xml_content(curr_node);
      val = aio_dae_fxEnumDraw(node_val);

      evaluate->draw.enum_draw = val;
      evaluate->draw.str_val = node_val;
    }
  }

  *dest = evaluate;

  return 0;
}