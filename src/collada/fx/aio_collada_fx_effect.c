/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_effect.h"

#include "../aio_collada_asset.h"
#include "../aio_collada_technique.h"
#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "aio_collada_fx_profile.h"
#include "../aio_collada_annotate.h"
#include "../aio_collada_param.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_effect(xmlNode * __restrict xml_node,
                        aio_effect ** __restrict dest) {

  xmlNode    * curr_node;
  xmlAttr    * curr_attr;
  aio_effect * effect;

  curr_node = xml_node;
  effect = aio_malloc(sizeof(*effect));
  memset(effect, '\0', sizeof(*effect));

  curr_attr = curr_node->properties;

  /* parse camera attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "id"))
        effect->id = aio_strdup(attr_val);
      else if (AIO_IS_EQ_CASE(attr_name, "name"))
        effect->name = aio_strdup(attr_val);

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
                           effect->inf);

      } else if (AIO_IS_EQ_CASE(node_name, "annotate")) {
        aio_annotate * annotate;
        aio_annotate * last_annotate;
        int            ret;

        ret = aio_load_collada_annotate(curr_node, &annotate);

        if (ret == 0) {
          last_annotate = effect->annotate;
          if (last_annotate) {
            annotate->prev = last_annotate;
            last_annotate->next = annotate;
          } else {
            effect->annotate = annotate;
          }

        }
      } else if (AIO_IS_EQ_CASE(node_name, "newparam")) {
        aio_newparam * newparam;
        aio_newparam * last_newparam;
        int            ret;

        ret = aio_load_collada_newparam(curr_node, &newparam);

        if (ret == 0) {
          last_newparam = effect->newparam;
          if (last_newparam) {
            newparam->prev = last_newparam;
            last_newparam->next = newparam;
          } else {
            effect->newparam = newparam;
          }
          
        }

      } else if (AIO_IS_EQ_CASE(node_name, "profile_COMMON")) {
        aio_profile * profile;
        aio_profile * last_profile;
        int           ret;

        last_profile = effect->profile;

        ret = aio_load_collada_profile(curr_node,
                                       AIO_PROFILE_TYPE_COMMON,
                                       &profile);

        if (ret == 0) {
          if (last_profile) {
            last_profile->next = profile;
            profile->prev = last_profile;
          } else {
            effect->profile = profile;
          }
        }
      } else if (AIO_IS_EQ_CASE(node_name, "profile_GLSL")) {
        aio_profile * profile;
        aio_profile * last_profile;
        int           ret;

        last_profile = effect->profile;

        ret = aio_load_collada_profile(curr_node,
                                       AIO_PROFILE_TYPE_GLSL,
                                       &profile);

        if (ret == 0) {
          if (last_profile) {
            last_profile->next = profile;
            profile->prev = last_profile;
          } else {
            effect->profile = profile;
          }
        }
      } else if (AIO_IS_EQ_CASE(node_name, "profile_GLES2")) {
        aio_profile * profile;
        aio_profile * last_profile;
        int           ret;

        last_profile = effect->profile;

        ret = aio_load_collada_profile(curr_node,
                                       AIO_PROFILE_TYPE_GLES2,
                                       &profile);

        if (ret == 0) {
          if (last_profile) {
            last_profile->next = profile;
            profile->prev = last_profile;
          } else {
            effect->profile = profile;
          }
        }
      } else if (AIO_IS_EQ_CASE(node_name, "profile_GLES")) {
        aio_profile * profile;
        aio_profile * last_profile;
        int           ret;

        last_profile = effect->profile;

        ret = aio_load_collada_profile(curr_node,
                                       AIO_PROFILE_TYPE_GLES,
                                       &profile);

        if (ret == 0) {
          if (last_profile) {
            last_profile->next = profile;
            profile->prev = last_profile;
          } else {
            effect->profile = profile;
          }
        }
      } else if (AIO_IS_EQ_CASE(node_name, "profile_CG")) {
        aio_profile * profile;
        aio_profile * last_profile;
        int           ret;

        last_profile = effect->profile;

        ret = aio_load_collada_profile(curr_node,
                                       AIO_PROFILE_TYPE_CG,
                                       &profile);

        if (ret == 0) {
          if (last_profile) {
            last_profile->next = profile;
            profile->prev = last_profile;
          } else {
            effect->profile = profile;
          }
        }
      } else if (AIO_IS_EQ_CASE(node_name, "profile_BRIDGE")) {
        aio_profile * profile;
        aio_profile * last_profile;
        int           ret;

        last_profile = effect->profile;

        ret = aio_load_collada_profile(curr_node,
                                       AIO_PROFILE_TYPE_BRIDGE,
                                       &profile);

        if (ret == 0) {
          if (last_profile) {
            last_profile->next = profile;
            profile->prev = last_profile;
          } else {
            effect->profile = profile;
          }
        }

      } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
        _AIO_TREE_LOAD_TO(curr_node->children,
                          effect->extra,
                          NULL);
      } else {

      }
    }

    curr_node = curr_node->next;
  }

  *dest = effect;

  return 0;
}
