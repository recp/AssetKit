/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_profile.h"

#include "../aio_collada_asset.h"
#include "../aio_collada_technique.h"
#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"
#include "../aio_collada_param.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_profile(xmlNode * __restrict xml_node,
                         aio_profile_type profile_type,
                         aio_profile ** __restrict dest) {
  xmlNode     * curr_node;
  xmlAttr     * curr_attr;
  aio_profile * profile;

  curr_node = xml_node;

  switch (profile_type) {
    case AIO_PROFILE_TYPE_COMMON:
      profile = aio_malloc(sizeof(aio_profile_common));
      memset(profile, '\0', sizeof(aio_profile_common));
      break;
    case AIO_PROFILE_TYPE_GLSL:
      profile = aio_malloc(sizeof(aio_profile_GLSL));
      memset(profile, '\0', sizeof(aio_profile_GLSL));
      break;
    case AIO_PROFILE_TYPE_GLES2:
      profile = aio_malloc(sizeof(aio_profile_GLES2));
      memset(profile, '\0', sizeof(aio_profile_GLES2));
      break;
    case AIO_PROFILE_TYPE_GLES:
      profile = aio_malloc(sizeof(aio_profile_GLES));
      memset(profile, '\0', sizeof(aio_profile_GLES));
      break;
    case AIO_PROFILE_TYPE_CG:
      profile = aio_malloc(sizeof(aio_profile_CG));
      memset(profile, '\0', sizeof(aio_profile_CG));
      break;
    case AIO_PROFILE_TYPE_BRIDGE:
      profile = aio_malloc(sizeof(aio_profile_BRIDGE));
      memset(profile, '\0', sizeof(aio_profile_BRIDGE));
      break;
    default:
      profile = NULL;
      break;
  }

  if (!profile)
    return -1;

  curr_attr = curr_node->properties;

  /* parse camera attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "id")) {
        switch (profile_type) {
          case AIO_PROFILE_TYPE_GLSL:
            ((aio_profile_GLSL *)profile)->id = aio_strdup(attr_val);
            break;
          case AIO_PROFILE_TYPE_GLES:
            ((aio_profile_GLES *)profile)->id = aio_strdup(attr_val);
            break;
          case AIO_PROFILE_TYPE_GLES2:
            ((aio_profile_GLES2 *)profile)->id = aio_strdup(attr_val);
            break;
          case AIO_PROFILE_TYPE_CG:
            ((aio_profile_CG *)profile)->id = aio_strdup(attr_val);
            break;
          case AIO_PROFILE_TYPE_BRIDGE:
            ((aio_profile_BRIDGE *)profile)->id = aio_strdup(attr_val);
            break;
          case AIO_PROFILE_TYPE_COMMON:
            ((aio_profile_common *)profile)->id = aio_strdup(attr_val);
            break;
          default:
            break;
        }

      } else if (AIO_IS_EQ_CASE(attr_name, "platform")) {
        switch (profile_type) {
          case AIO_PROFILE_TYPE_GLSL:
            ((aio_profile_GLSL *)profile)->platform = aio_strdup(attr_val);
            break;
          case AIO_PROFILE_TYPE_GLES:
            ((aio_profile_GLES *)profile)->platform = aio_strdup(attr_val);
            break;
          case AIO_PROFILE_TYPE_CG:
            ((aio_profile_CG *)profile)->platform = aio_strdup(attr_val);
            break;
          case AIO_PROFILE_TYPE_BRIDGE:
            ((aio_profile_BRIDGE *)profile)->platform = aio_strdup(attr_val);
            break;
          default:
            break;
        }
      } else if (AIO_IS_EQ_CASE(attr_name, "language")) {
        /* only GLES2 */
        if (profile_type == AIO_PROFILE_TYPE_GLES2)
          ((aio_profile_GLES2 *)profile)->language = aio_strdup(attr_val);
      } else if (AIO_IS_EQ_CASE(attr_name, "platforms")) {
        /* only GLES2 */
        if (profile_type == AIO_PROFILE_TYPE_GLES2)
          ((aio_profile_GLES2 *)profile)->platforms = aio_strdup(attr_val);
     } else if (AIO_IS_EQ_CASE(attr_name, "platforms")) {
        /* only BRIDGE */
       if (profile_type == AIO_PROFILE_TYPE_BRIDGE)
         ((aio_profile_BRIDGE *)profile)->url = aio_strdup(attr_val);
     }

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
                           profile->inf);

      } else if (AIO_IS_EQ_CASE(node_name, "newparam")) {
        aio_newparam * newparam;
        int            ret;

        ret = aio_load_collada_newparam(curr_node, &newparam);

        if (ret == 0)
          profile->newparam = newparam;

      } else if (AIO_IS_EQ_CASE(node_name, "technique")) {
        /* TODO: */
      } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
        _AIO_TREE_LOAD_TO(curr_node->children,
                          profile->extra,
                          NULL);
      } else if (AIO_IS_EQ_CASE(node_name, "code")) {
        aio_code   * code;
        const char * attr_name;
        const char * node_val;

        code = aio_malloc(sizeof(*code));
        memset(code, '\0', sizeof(*code));

        curr_attr = curr_node->properties;
        while (curr_attr) {
          attr_name = (const char *)curr_attr->name;
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            if (AIO_IS_EQ_CASE(attr_name, "sid")) {
              const char * attr_val;

              attr_val = aio_xml_node_content((xmlNode *)curr_attr);
              code->sid = aio_strdup(attr_val);

              break;
            }
          }
          
          curr_attr = curr_attr->next;
        } /* while */


        node_val = aio_xml_node_content(curr_node);
        code->val = aio_strdup(node_val);

        switch (profile_type) {
          case AIO_PROFILE_TYPE_CG: {
            aio_profile_CG * profile_cg;
            aio_code       * last_code;

            profile_cg = (aio_profile_CG *)profile;
            last_code = profile_cg->code;

            if (last_code) {
              last_code->next = code;
              code->prev = last_code;
            } else {
              profile_cg->code = code;
            }

            break;
          } case AIO_PROFILE_TYPE_GLSL: {
            aio_profile_GLSL * profile_glsl;
            aio_code         * last_code;

            profile_glsl = (aio_profile_GLSL *)profile;
            last_code = profile_glsl->code;

            if (last_code) {
              last_code->next = code;
              code->prev = last_code;
            } else {
              profile_glsl->code = code;
            }

            break;
          } case AIO_PROFILE_TYPE_GLES2: {
            aio_profile_GLES2 * profile_gles2;
            aio_code          * last_code;

            profile_gles2 = (aio_profile_GLES2 *)profile;
            last_code = profile_gles2->code;

            if (last_code) {
              last_code->next = code;
              code->prev = last_code;
            } else {
              profile_gles2->code = code;
            }
            
            break;
          }
          default:
            break;
        }
      } else if (AIO_IS_EQ_CASE(node_name, "include")) {
        aio_include * include;
        const char  * attr_name;
        const char  * node_val;

        include = aio_malloc(sizeof(*include));
        memset(include, '\0', sizeof(*include));

        curr_attr = curr_node->properties;
        while (curr_attr) {
          attr_name = (const char *)curr_attr->name;
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            if (AIO_IS_EQ_CASE(attr_name, "sid")) {
              const char * attr_val;

              attr_val = aio_xml_node_content((xmlNode *)curr_attr);
              include->sid = aio_strdup(attr_val);
            } else if (AIO_IS_EQ_CASE(attr_name, "url")) {
              const char * attr_val;

              attr_val = aio_xml_node_content((xmlNode *)curr_attr);
              include->url = aio_strdup(attr_val);
            }
          }

          curr_attr = curr_attr->next;
        } /* while */


        node_val = aio_xml_node_content(curr_node);
        if (!include->url && node_val)
          include->url = aio_strdup(node_val);

        switch (profile_type) {
          case AIO_PROFILE_TYPE_CG: {
            aio_profile_CG * profile_cg;
            aio_include    * last_inc;

            profile_cg = (aio_profile_CG *)profile;
            last_inc = profile_cg->include;

            if (last_inc) {
              last_inc->next = include;
              include->prev = last_inc;
            } else {
              profile_cg->include = include;
            }

            break;
          } case AIO_PROFILE_TYPE_GLSL: {
            aio_profile_GLSL * profile_glsl;
            aio_include      * last_inc;

            profile_glsl = (aio_profile_GLSL *)profile;
            last_inc = profile_glsl->include;

            if (last_inc) {
              last_inc->next = include;
              include->prev = last_inc;
            } else {
              profile_glsl->include = include;
            }

            break;
          } case AIO_PROFILE_TYPE_GLES2: {
            aio_profile_GLES2 * profile_gles2;
            aio_include       * last_inc;

            profile_gles2 = (aio_profile_GLES2 *)profile;
            last_inc = profile_gles2->include;

            if (last_inc) {
              last_inc->next = include;
              include->prev = last_inc;
            } else {
              profile_gles2->include = include;
            }

            break;
          }
          default:
            break;
        }
      }
    }
    
    curr_node = curr_node->next;
  }

  *dest = profile;

  return 0;
}
