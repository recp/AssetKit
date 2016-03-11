/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_tree.h"

#include "aio_libxml.h"
#include "aio_common.h"
#include "aio_memory.h"
#include "aio_utils.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_tree_fromXmlNode(xmlNode * __restrict xml_node,
                     aio_tree_node ** __restrict dest,
                     aio_tree_node * __restrict parent) {

  xmlNode       * currNode;
  aio_tree_node * tree_currNode;

  tree_currNode = NULL;
  currNode = xml_node->children;

  /* extra is text node */
  if (currNode && currNode->type == XML_TEXT_NODE) {
    if (currNode->content) {
      tree_currNode = aio_calloc(sizeof(*tree_currNode), 1);
      tree_currNode->val = aio_strdup((const char *)currNode->content);

      *dest = tree_currNode;
      return 0;
    }
  }

  for (;
       currNode && currNode->type == XML_ELEMENT_NODE;
       currNode = currNode->next) {
    aio_tree_node      * tree_nNode;
    aio_tree_node_attr * tree_currAttr;
    const xmlAttr      * xml_currAttr;

    tree_nNode = aio_calloc(sizeof(*tree_currNode), 1);
    tree_nNode->parent = parent;
    tree_nNode->name = aio_strdup((const char *)currNode->name);

    if (tree_currNode) {
      tree_nNode->prev    = tree_currNode;
      tree_currNode->next = tree_nNode;
    }

    tree_currNode = tree_nNode;

    /*
     If the destination is NULL then set it as the first node
     */
    if (!(*dest))
      *dest = tree_nNode;

    /* Children count */
    if (parent)
      ++parent->chldc;

    /* Load attributes */
    tree_currAttr = NULL;
    for (xml_currAttr = currNode->properties;
         xml_currAttr && xml_currAttr->type == XML_ATTRIBUTE_NODE;
         xml_currAttr = xml_currAttr->next) {
      aio_tree_node_attr * tree_nodeAttr;

      tree_nodeAttr = aio_calloc(sizeof(*tree_nodeAttr), 1);
      tree_nodeAttr->name = aio_strdup((const char *)xml_currAttr->name);
      tree_nodeAttr->val =
        aio_strdup(((const char *)xml_currAttr->children->content));

      if (tree_currAttr) {
        tree_nodeAttr->prev = tree_currAttr;
        tree_currAttr->next = tree_nodeAttr;
      } else {
        tree_nNode->attr = tree_nodeAttr;
      }

      tree_currAttr = tree_nodeAttr;
      ++tree_nNode->attrc;
    }

    /* Load children and content if available */
    if (currNode->children) {
      switch (currNode->children->type) {
          /*
           According to XML_PARSE_NOBLANKS option TEXT NODE is the content.
           Without the XML_PARSE_NOBLANKS option leading spaces, white spaces,
           new lines, tabs... would be cause libxml to yields extra TEXT NODE
           */
        case XML_TEXT_NODE:
          tree_nNode->val =
            aio_strdup((const char *)currNode->children->content);
          break;

          /* Load child nodes */
        case XML_ELEMENT_NODE:
          aio_tree_fromXmlNode(currNode,
                               &tree_nNode->chld,
                               tree_nNode);

          break;
        default:
          break;
      } // switch
    } else {
      if (currNode->content)
        tree_nNode->val = aio_strdup((const char *)currNode->content);
    }
  }

  return 0;
}
