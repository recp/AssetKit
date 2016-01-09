/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_shader.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_common.h"

#include "aio_collada_fx_enums.h"
#include "aio_collada_fx_binary.h"
#include "aio_collada_fx_uniform.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_dae_fxShader(xmlNode * __restrict xml_node,
                 aio_shader ** __restrict dest) {
  xmlNode      * curr_node;
  xmlAttr      * curr_attr;
  aio_shader   * shader;
  aio_compiler * last_compiler;
  aio_bind_uniform * last_bind_uniform;

  shader = aio_malloc(sizeof(*shader));
  memset(shader, '\0', sizeof(*shader));

  last_compiler = NULL;
  last_bind_uniform = NULL;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "stage")) {
      long val;
      val = aio_dae_fxEnumShaderStage(attr_val);
      if (val == 0)
        shader->stage = val;

      break;
    }
  }

  curr_attr = NULL;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {
    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "sources")) {
      xmlNode * prev_node;
      aio_sources * sources;
      aio_inline * last_inline;
      aio_import * last_import;

      sources = aio_malloc(sizeof(*sources));
      memset(sources, '\0', sizeof(*sources));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "entry")) {
          sources->entry = aio_strdup(attr_val);
          break;
        }
      }

      curr_attr = NULL;

      last_inline = NULL;
      last_import = NULL;
      prev_node = curr_node;

      for (curr_node = curr_node->children;
           curr_node && curr_node->type == XML_ELEMENT_NODE;
           curr_node = curr_node->next) {
        node_name = (const char *)curr_node->name;
        if (AIO_IS_EQ_CASE(node_name, "inline")) {
          aio_inline * _inline;

          _inline = aio_malloc(sizeof(*_inline));
          memset(_inline, '\0', sizeof(*_inline));

          _inline->val = aio_strdup(aio_xml_content(curr_node));
           if (last_inline)
             last_inline->next = _inline;
           else
             sources->inlines = _inline;

          last_inline = _inline;
        } else if (AIO_IS_EQ_CASE(node_name, "import")) {
          aio_import * _import;

          _import = aio_malloc(sizeof(*_import));
          memset(_import, '\0', sizeof(*_import));

          _import->ref = aio_strdup(aio_xml_content(curr_node));
          if (last_import)
            last_import->next = _import;
          else
            sources->imports = _import;

          last_import = _import;
        }
      }

      node_name = NULL;
      curr_node = prev_node;

    } else if (AIO_IS_EQ_CASE(node_name, "compiler")) {
      aio_compiler * compiler;
      xmlNode      * prev_node;

      compiler = aio_malloc(sizeof(*compiler));
      memset(compiler, '\0', sizeof(*compiler));

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "platform")) {
          compiler->platform = aio_strdup(attr_val);
        } else if (AIO_IS_EQ_CASE(node_name, "target")) {
          compiler->target = aio_strdup(attr_val);
        } else if (AIO_IS_EQ_CASE(node_name, "options")) {
          compiler->options = aio_strdup(attr_val);
        }
      }

      curr_attr = NULL;
      prev_node = curr_node;

      for (curr_node = curr_node->children;
           curr_node && curr_node->type == XML_ELEMENT_NODE;
           curr_node = curr_node->next) {
        node_name = (const char *)curr_node->name;
        if (AIO_IS_EQ_CASE(node_name, "binary")) {
          aio_binary * binary;
          int          ret;

          binary = NULL;
          ret = aio_dae_fxBinary(curr_node, &binary);

          if (ret == 0)
            compiler->binary = binary;

          break;
        }
      }

      if (last_compiler)
        last_compiler->next = compiler;
      else
        shader->compiler = compiler;

      last_compiler = compiler;

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
          shader->bind_uniform = bind_uniform;

        last_bind_uniform = bind_uniform;
      }

    } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
      _AIO_TREE_LOAD_TO(curr_node->children,
                        shader->extra,
                        NULL);
    }
  }

  *dest = shader;
  
  return 0;
}
