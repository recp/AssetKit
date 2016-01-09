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
#include "aio_collada_fx_uniform.h"
#include "aio_collada_fx_binary.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_dae_fxProg(xmlNode * __restrict xml_node,
               aio_program ** __restrict dest) {
  xmlNode          * curr_node;
  aio_program      * prog;
  aio_bind_uniform * last_bind_uniform;
  aio_bind_attrib  * last_bind_attrib;
  aio_linker       * last_linker;

  prog = aio_malloc(sizeof(*prog));
  memset(prog, '\0', sizeof(*prog));

  last_bind_uniform = NULL;
  last_bind_attrib = NULL;
  last_linker = NULL;

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
      aio_linker * linker;
      xmlAttr    * curr_attr;
      xmlNode    * prev_node;
      aio_binary * last_binary;

      linker = aio_malloc(sizeof(*linker));
      memset(linker, '\0', sizeof(*linker));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "platform")) {
          linker->platform = aio_strdup(attr_val);
        } else if (AIO_IS_EQ_CASE(node_name, "target")) {
          linker->target = aio_strdup(attr_val);
        } else if (AIO_IS_EQ_CASE(node_name, "options")) {
          linker->options = aio_strdup(attr_val);
        }
      }

      last_binary = NULL;
      curr_attr = NULL;
      prev_node = curr_node;

      for (curr_node = xml_node->children;
           curr_node && curr_node->type == XML_ELEMENT_NODE;
           curr_node = curr_node->next) {
        const char * node_name;
        node_name = (const char *)curr_node->name;
        if (AIO_IS_EQ_CASE(node_name, "binary")) {
          aio_binary * binary;
          int          ret;

          binary = NULL;
          ret = aio_dae_fxBinary(curr_node, &binary);

          if (ret == 0) {
            if (last_binary)
              last_binary->next = binary;
            else
              linker->binary = binary;

            last_binary = binary;
          }
        }
      }

      node_name = NULL;
      curr_node = prev_node;

    } else if (AIO_IS_EQ_CASE(node_name, "bind_attribute")) {
      aio_bind_attrib * bind_attrib;
      xmlAttr * curr_attr;
      xmlNode * prev_node;

      bind_attrib = aio_malloc(sizeof(*bind_attrib));
      memset(bind_attrib, '\0', sizeof(*bind_attrib));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "symbol")) {
          bind_attrib->symbol = aio_strdup(attr_val);
          break;
        }
      }

      curr_attr = NULL;
      prev_node = curr_node;

      for (curr_node = xml_node->children;
           curr_node && curr_node->type == XML_ELEMENT_NODE;
           curr_node = curr_node->next) {
        const char * node_name;
        node_name = (const char *)curr_node->name;
        if (AIO_IS_EQ_CASE(node_name, "semantic")) {
          bind_attrib->semantic = aio_strdup(aio_xml_content(curr_node));
          break;
        }
      }

      if (last_bind_attrib)
        last_bind_attrib->next = bind_attrib;
      else
        prog->bind_attrib = bind_attrib;

      last_bind_attrib = bind_attrib;

      node_name = NULL;
      curr_node = prev_node;

    } else if (AIO_IS_EQ_CASE(node_name, "bind_uniform")) {
      aio_bind_uniform * bind_uniform;
      int ret;

      bind_uniform = NULL;
      ret = aio_dae_fxBindUniform(curr_node, &bind_uniform);

      if (ret == 0) {
        if (last_bind_uniform)
          last_bind_uniform->next = bind_uniform;
        else
          prog->bind_uniform = bind_uniform;

        last_bind_uniform = bind_uniform;
      }
    }
  }

  *dest = prog;
  
  return 0;
}
