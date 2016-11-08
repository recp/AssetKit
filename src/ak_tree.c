/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_tree.h"

#include "ak_libxml.h"
#include "ak_common.h"
#include "ak_utils.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

AkResult _assetkit_hide
ak_tree_fromXmlNode(AkHeap * __restrict heap,
                    void * __restrict memParent,
                    xmlNode * __restrict xml_node,
                    AkTreeNode ** __restrict dest,
                    AkTreeNode * __restrict parent) {
  
  xmlNode       * currNode;
  AkTreeNode * tree_currNode;

  tree_currNode = NULL;
  currNode = xml_node->children;

  /* extra is text node */
  if (currNode && currNode->type == XML_TEXT_NODE) {
    if (currNode->content) {
      tree_currNode = ak_heap_calloc(heap,
                                     memParent,
                                     sizeof(*tree_currNode),
                                     false);
      tree_currNode->val = ak_heap_strdup(heap,
                                          tree_currNode,
                                          (const char *)currNode->content);

      *dest = tree_currNode;
      return AK_OK;
    }
  }

  for (;
       currNode && currNode->type == XML_ELEMENT_NODE;
       currNode = currNode->next) {
    AkTreeNode      * tree_nNode;
    AkTreeNodeAttr * tree_currAttr;
    const xmlAttr      * xml_currAttr;

    tree_nNode = ak_heap_calloc(heap, parent, sizeof(*tree_currNode), false);
    tree_nNode->parent = parent;
    tree_nNode->name = ak_heap_strdup(heap,
                                      tree_nNode,
                                      (const char *)currNode->name);

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
      AkTreeNodeAttr * tree_nodeAttr;

      tree_nodeAttr = ak_heap_calloc(heap,
                                     tree_nNode,
                                     sizeof(*tree_nodeAttr),
                                     false);
      tree_nodeAttr->name = ak_heap_strdup(heap,
                                           tree_nodeAttr,
                                           (const char *)xml_currAttr->name);
      tree_nodeAttr->val =
        ak_heap_strdup(heap,
                       tree_nodeAttr,
                       ((const char *)xml_currAttr->children->content));

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
            ak_heap_strdup(heap,
                           tree_nNode,
                           (const char *)currNode->children->content);
          break;

          /* Load child nodes */
        case XML_ELEMENT_NODE:
          ak_tree_fromXmlNode(heap,
                              tree_nNode,
                              currNode,
                              &tree_nNode->chld,
                              tree_nNode);

          break;
        default:
          break;
      } /* switch */
    } else {
      if (currNode->content)
        tree_nNode->val = ak_heap_strdup(heap,
                                         tree_nNode,
                                         (const char *)currNode->content);
    }
  }

  return AK_OK;
}
