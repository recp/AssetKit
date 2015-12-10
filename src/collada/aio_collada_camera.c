/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_camera.h"
#include "aio_collada_asset.h"
#include "aio_collada_technique.h"
#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_memory.h"
#include "../aio_utils.h"
#include "../aio_tree.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_camera(xmlNode * __restrict xml_node,
                        aio_camera ** __restrict  dest) {
  xmlNode         * curr_node;
  xmlNode         * prev_node;
  xmlAttr         * curr_attr;
  aio_camera      * camera;

  curr_node = xml_node;
  camera = aio_malloc(sizeof(*camera));
  curr_attr = curr_node->properties;

  /* parse camera attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "id"))
        camera->id = aio_strdup(attr_val);
      else if (AIO_IS_EQ_CASE(attr_name, "name"))
        camera->name = aio_strdup(attr_val);

    }

    curr_attr = curr_attr->next;
  }

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "asset")) {

        _AIO_ASSET_LOAD_TO(curr_node,
                           camera->inf);

      } else if (AIO_IS_EQ_CASE(node_name, "optics")) {
        aio_optics    * optics;
        aio_technique * last_technique;

        optics = aio_malloc(sizeof(*optics));
        memset(optics, '\0', sizeof(*optics));

        last_technique = optics->technique;

        prev_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          if (curr_node->type == XML_ELEMENT_NODE) {
            node_name = (const char *)curr_node->name;
            if (AIO_IS_EQ_CASE(node_name, "technique_common")) {

              aio_technique_common * technique_c;
              int                    ret;

              ret = aio_load_collada_techniquec(curr_node,
                                                &technique_c);

              if (ret == 0)
                optics->technique_common = technique_c;

            } else if (AIO_IS_EQ_CASE(node_name, "technique")) {
              aio_technique * technique;
              int             ret;

              ret = aio_load_collada_technique(curr_node, &technique);
              if (ret == 0) {
                if (last_technique) {
                  technique->prev = last_technique;
                  last_technique->next = technique;
                } else {
                  optics->technique = technique;
                }

                last_technique = technique;
              }
            }

          } // if elm
          
          curr_node = curr_node->next;
        }

        node_name = NULL;
        curr_node = prev_node;

        camera->optics = optics;

      } else if (AIO_IS_EQ_CASE(node_name, "imager")) {
        aio_imager    * imager;
        aio_technique * last_technique;

        imager = aio_malloc(sizeof(*imager));
        memset(imager, '\0', sizeof(*imager));

        last_technique = imager->technique;

        prev_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          if (curr_node->type == XML_ELEMENT_NODE) {
            node_name = (const char *)curr_node->name;
            if (AIO_IS_EQ_CASE(node_name, "technique")) {
              aio_technique * technique;
              int             ret;

              ret = aio_load_collada_technique(curr_node, &technique);
              if (ret == 0) {
                if (last_technique) {
                  technique->prev = last_technique;
                  last_technique->next = technique;
                } else {
                  imager->technique = technique;
                }

                last_technique = technique;
              }
            } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
              _AIO_TREE_LOAD_TO(curr_node->children,
                                imager->extra,
                                NULL);
            }

          } // if elm

          curr_node = curr_node->next;
        }
        
        node_name = NULL;
        curr_node = prev_node;

        camera->imager = imager;

      } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
        _AIO_TREE_LOAD_TO(curr_node->children,
                          camera->extra,
                          NULL);
      } else {

      }
    }

    curr_node = curr_node->next;
  }

  *dest = camera;
  
  return 0;
}
