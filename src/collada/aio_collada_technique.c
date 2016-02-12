/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_technique.h"
#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_memory.h"
#include "../aio_utils.h"
#include "../aio_tree.h"

#include "aio_collada_common.h"

int _assetio_hide
aio_dae_technique(xmlTextReaderPtr __restrict reader,
                  aio_technique ** __restrict dest) {

  aio_technique *technique;
  aio_tree      *tree;
  xmlNodePtr     nodePtr;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  technique = aio_calloc(sizeof(*technique), 1);

  _xml_readAttr(technique->profile, _s_dae_profile);
  _xml_readAttr(technique->xmlns, _s_dae_xmlns);

  nodePtr = xmlTextReaderExpand(reader);
  tree = NULL;

  aio_tree_fromXmlNode(nodePtr, &tree, NULL);
  technique->chld = tree;

  nodeName = xmlTextReaderConstName(reader);
  nodeType = xmlTextReaderNodeType(reader);
  
  _xml_skipElement;

  *dest = technique;

  return 0;
}

int _assetio_hide
aio_load_collada_technique(xmlNode * __restrict xml_node,
                           aio_technique ** __restrict dest) {
  xmlNode       * curr_node;
  xmlAttr       * curr_attr;
  aio_technique * technique;

  curr_node = xml_node;
  technique = aio_malloc(sizeof(*technique));
  memset(technique, '\0', sizeof(*technique));

  curr_attr = curr_node->properties;

  /* parse camera attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "profile"))
        technique->profile = aio_strdup(attr_val);
      else if (AIO_IS_EQ_CASE(attr_name, "xmlns"))
        technique->xmlns = aio_strdup(attr_val);
    }

    curr_attr = curr_attr->next;
  }

  
  _AIO_TREE_LOAD_TO(curr_node->children,
                    technique->chld,
                    NULL);

  *dest = technique;

  return 0;
}
