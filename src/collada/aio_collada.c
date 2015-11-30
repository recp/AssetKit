/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada.h"
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "../aio_memory.h"
#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_tree.h"
#include "aio_collada_common.h"

#include "aio_collada_asset.h"
#include "aio_collada_camera.h"
#include "aio_collada_light.h"
#include "fx/aio_collada_fx_effect.h"

int
_assetio_hide
aio_load_collada(aio_doc ** __restrict dest,
                 const char * __restrict data) {

  aio_doc   * asst_doc;
  xmlDocPtr   dae_doc;
  xmlNode   * root_node;
  xmlNode   * curr_node;

  if (!(dae_doc = xmlReadMemory(data,
                                (int)(sizeof(*data) * strlen(data)),
                                NULL,
                                NULL,
                                XML_PARSE_NOBLANKS)))
    goto err;

  root_node = xmlDocGetRootElement(dae_doc);
  curr_node = root_node;
  if (strcasecmp((const char*)curr_node->name, "COLLADA") != 0)
    goto err;

  asst_doc = aio_malloc(sizeof(*asst_doc));

  do {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;

      node_name = (const char *)curr_node->name;

      /* COLLADA Core */
      if (AIO_IS_EQ_CASE(node_name, "asset")) {
        aio_docinf * doc_inf;
        int          ret;

        ret = aio_load_collada_asset(curr_node,
                                     _AIO_ASSET_LOAD_AS_DOCINF,
                                     &doc_inf);

        if (ret == 0) {
          doc_inf->ftype = AIO_FILE_TYPE_COLLADA;
          asst_doc->docinf = *doc_inf;
        }

      } else if (AIO_IS_EQ_CASE(node_name, "library_cameras")) {
        xmlNode        * prev_node;
        aio_lib_camera * camera_lib;

        camera_lib = &asst_doc->lib.cameras;

        aio_xml_collada_read_id_name(curr_node,
                                     &camera_lib->id,
                                     &camera_lib->name);

        prev_node = curr_node;
        curr_node = curr_node->children;

        while (curr_node) {;
          if (curr_node->type == XML_ELEMENT_NODE) {
            node_name = (const char *)curr_node->name;

            if (AIO_IS_EQ_CASE(node_name, "asset")) {
              _AIO_ASSET_LOAD_TO(curr_node,
                                 asst_doc->lib.cameras.inf);

            } else if (AIO_IS_EQ_CASE(node_name, "camera")) {
              aio_camera * acamera;
              int          ret;

              ret = aio_load_collada_camera(curr_node, &acamera);
              if (ret == 0) {
                if (camera_lib->next) {
                  acamera->prev = camera_lib->next;
                  acamera->next = NULL;

                  camera_lib->next = acamera;
                } else {
                  acamera->prev = NULL;
                  acamera->next = NULL;

                  camera_lib->next = acamera;
                }
              }
            } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
              _AIO_TREE_LOAD_TO(curr_node->children,
                                asst_doc->lib.cameras.extra,
                                NULL);
            }
          }

          curr_node = curr_node->next;
        } /* while library_cameras */

        node_name = NULL;
        curr_node = prev_node;
      } else if (AIO_IS_EQ_CASE(node_name, "library_lights")) {
        xmlNode       * prev_node;
        aio_lib_light * light_lib;
        aio_light     * prev_light;

        light_lib = &asst_doc->lib.lights;

        aio_xml_collada_read_id_name(curr_node,
                                     &light_lib->id,
                                     &light_lib->name);

        prev_node = curr_node;
        curr_node = curr_node->children;
        prev_light = light_lib->next;
        
        while (curr_node) {
          if (curr_node->type == XML_ELEMENT_NODE) {
            node_name = (const char *)curr_node->name;

            if (AIO_IS_EQ_CASE(node_name, "asset")) {
              _AIO_ASSET_LOAD_TO(curr_node,
                                 asst_doc->lib.cameras.inf);

            } else if (AIO_IS_EQ_CASE(node_name, "light")) {
              aio_light * alight;
              int         ret;

              ret = aio_load_collada_light(curr_node, &alight);
              if (ret == 0) {
                if (prev_light) {
                  alight->prev = prev_light;
                  prev_light->next = alight;
                  prev_light = alight;
                } else {
                  prev_light = alight;
                  light_lib->next = alight;
                }
              }
            } else if (AIO_IS_EQ_CASE(node_name, "technique")) {
              /* TODO: */
            } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
              _AIO_TREE_LOAD_TO(curr_node->children,
                                asst_doc->lib.cameras.extra,
                                NULL);
            }
          }

          curr_node = curr_node->next;
        } /* while library_cameras */
        
        node_name = NULL;
        curr_node = prev_node;
      }

      /* COLLADA FX */
      else if (AIO_IS_EQ_CASE(node_name, "library_effects")) {
        xmlNode        * prev_node;
        aio_lib_effect * effect_lib;

        effect_lib = &asst_doc->lib.effects;

        aio_xml_collada_read_id_name(curr_node,
                                     &effect_lib->id,
                                     &effect_lib->name);

        prev_node = curr_node;
        curr_node = curr_node->children;

        while (curr_node) {
          if (curr_node->type == XML_ELEMENT_NODE) {
            node_name = (const char *)curr_node->name;

            if (AIO_IS_EQ_CASE(node_name, "asset")) {
              _AIO_ASSET_LOAD_TO(curr_node,
                                 asst_doc->lib.cameras.inf);

            } else if (AIO_IS_EQ_CASE(node_name, "effect")) {
              aio_effect * an_effect;
              int          ret;

              ret = aio_load_collada_effect(curr_node, &an_effect);
              if (ret == 0) {
                if (effect_lib->next) {
                  an_effect->prev = effect_lib->next;
                  effect_lib->next = an_effect;
                } else {
                  effect_lib->next = an_effect;
                }
              }
            } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
              _AIO_TREE_LOAD_TO(curr_node->children,
                                asst_doc->lib.cameras.extra,
                                NULL);
            }
          }

          curr_node = curr_node->next;
        } /* while library_cameras */
        
        node_name = NULL;
        curr_node = prev_node;
      }
    }

    if (!curr_node->next) {
      curr_node = curr_node->children;
      continue;
    }

    curr_node = curr_node->next;
  } while (curr_node);

  xmlFreeDoc(dae_doc);
  xmlCleanupParser();

  *dest = asst_doc;
  return 0;
err:
  return -1;
}
