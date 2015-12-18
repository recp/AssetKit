/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_color_or_tex.h"

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
aio_load_color_or_tex(xmlNode * __restrict xml_node,
                      aio_fx_color_or_tex ** __restrict dest) {
  xmlNode             * curr_node;
  xmlAttr             * curr_attr;
  aio_fx_color_or_tex * color_or_tex;

  curr_node = xml_node;
  color_or_tex = aio_malloc(sizeof(*color_or_tex));
  memset(color_or_tex, '\0', sizeof(*color_or_tex));

  curr_attr = curr_node->properties;

  /* parse camera attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "opaque")) {
        long opaque;

        if (AIO_IS_EQ_CASE(attr_val, "A_ONE"))
          opaque = AIO_OPAQUE_A_ONE;
        else if (AIO_IS_EQ_CASE(attr_val, "RGB_ZERO"))
          opaque = AIO_OPAQUE_RGB_ZERO;
        else if (AIO_IS_EQ_CASE(attr_val, "A_ZERO"))
          opaque = AIO_OPAQUE_A_ZERO;
        else if (AIO_IS_EQ_CASE(attr_val, "RGB_ONE"))
          opaque = AIO_OPAQUE_RGB_ONE;

        color_or_tex->opaque = opaque;
      }
    }

    curr_attr = curr_attr->next;
  }

  curr_attr = NULL;

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "color")) {
        const char * node_content;

        aio_color * color;
        char      * sid;
        char      * node_content_dup;
        char      * color_comp;
        float     * color_vec;
        int         color_comp_idx;

        color = aio_malloc(sizeof(*color));
        memset(color, '\0', sizeof(*color));

        sid = NULL;
        aio_xml_collada_read_attr(curr_node, "sid", &sid);

        if (sid)
          color->sid = sid;

        color_vec = color->vec;
        color_comp_idx = 0;

        node_content = aio_xml_node_content(curr_node);
        node_content_dup = strdup(node_content);

        color_comp = strtok(node_content_dup, " ");
        color_vec[color_comp_idx] = strtod(color_comp, NULL);

        while (color_comp && ++color_comp_idx < 4) {
          color_comp = strtok(NULL, " ");

          if (!color_comp) {
            --color_comp_idx;
            continue;
          }

          color_vec[color_comp_idx] = strtod(color_comp, NULL);
        }

        /* make alpha channel to 1.0 as default */
        if (color_comp_idx < 3)
          color_vec[3] = 1.0;

        free(node_content_dup);

        color_or_tex->color = color;

      } else if (AIO_IS_EQ_CASE(node_name, "texture")) {
        aio_fx_texture * tex;

        tex = aio_malloc(sizeof(*tex));
        memset(tex, '\0', sizeof(*tex));

        curr_attr = curr_node->properties;

        /* parse camera attributes */
        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "texture"))
              tex->texture = aio_strdup(attr_val);
            else if (AIO_IS_EQ_CASE(attr_name, "texcoord"))
              tex->texcoord = aio_strdup(attr_val);
          }
          
          curr_attr = curr_attr->next;
        }
        
        curr_attr = NULL;

        /* parse childrens */
        curr_node = xml_node->children;
        while (curr_node) {
          if (curr_node->type == XML_ELEMENT_NODE) {
            const char * node_name;
            node_name = (const char *)curr_node->name;

            if (AIO_IS_EQ_CASE(node_name, "extra")) {
              aio_tree_node * extra;
              aio_tree_node * last_extra;
              int             ret;

              extra = NULL;
              last_extra = tex->extra;

              ret = aio_tree_load_from_xml(curr_node,
                                           &extra,
                                           NULL);

              if (ret == 0)
                tex->extra = extra;

              if (ret == 0) {
                if (last_extra) {
                  last_extra->next = extra;
                  extra->prev = last_extra;
                } else {
                  tex->extra = extra;
                }
              }

            }
          }

          curr_node = curr_node->next;
        } /* while */

        color_or_tex->texture = tex;
        curr_node = NULL;

      } else if (AIO_IS_EQ_CASE(node_name, "param")) {
        aio_param * param;
        aio_param * last_param;
        int         ret;

        aio_param_extended * h;
        h = NULL;

        param = NULL;
        last_param = (aio_param *)color_or_tex->param;
        
        ret = aio_load_collada_param(curr_node,
                                     AIO_PARAM_TYPE_BASIC,
                                     &param);

        if (ret == 0) {
          if (last_param) {
            last_param->next = param;
            param->prev = last_param;
          } else {
            color_or_tex->param = (aio_param_basic *)param;
          }
        }
      } /* if */
    }
    
    curr_node = curr_node->next;
  }

  *dest = color_or_tex;

  return 0;
}
