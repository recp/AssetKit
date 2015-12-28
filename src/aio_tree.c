/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_tree.h"

#include "aio_libxml.h"
#include "aio_types.h"
#include "aio_memory.h"
#include "aio_utils.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_tree_load_from_xml(xmlNode * __restrict xml_node,
                       aio_tree_node ** __restrict dest,
                       aio_tree_node * __restrict parent) {

  xmlNode       * curr_node;
  aio_tree_node * tree_curr_node;

  tree_curr_node = NULL;
  curr_node      = xml_node;

  while (curr_node) {

    if (curr_node->type == XML_ELEMENT_NODE) {
      const char         * node_name;
      aio_tree_node      * tree_node;
      const xmlAttr      * xml_curr_attr;
      const char         * xml_attr_name;
      aio_tree_node_attr * tree_curr_attr;

      node_name = (const char *)curr_node->name;
      tree_node = aio_malloc(sizeof(*tree_curr_node));
      memset(tree_node, '\0', sizeof(*tree_curr_node));

      tree_node->parent = parent;

      if (tree_curr_node) {
        tree_node->prev      = tree_curr_node;
        tree_curr_node->next = tree_node;
      }

      tree_curr_node = tree_node;

      /* 
        If the destination is NULL then set it as the first node
       */
      if (!(*dest))
        *dest = tree_node;

      tree_node->name = aio_malloc(sizeof(*tree_node->name));
      strcpy((char *)tree_node->name, node_name);

      /* Children count */
      if (parent)
        ++parent->chldc;

      /* Load attributes */
      tree_curr_attr = NULL;
      xml_curr_attr  = curr_node->properties;

      while (xml_curr_attr) {
        if (xml_curr_attr->type == XML_ATTRIBUTE_NODE) {
          aio_tree_node_attr * tree_node_attr;
          const char         * tree_attr_val;

          xml_attr_name = (const char *)xml_curr_attr->name;
          tree_node_attr = aio_malloc(sizeof(*tree_node_attr));
          memset(tree_node_attr, '\0', sizeof(*tree_node_attr));

          if (tree_curr_attr) {
            tree_node_attr->prev = tree_curr_attr;
            tree_curr_attr->next = tree_node_attr;
          } else {
            tree_node->attr = tree_node_attr;
          }

          tree_curr_attr = tree_node_attr;

          tree_node_attr->name = aio_malloc(sizeof(*tree_node_attr->name));
          strcpy((char *)tree_node_attr->name, xml_attr_name);

          tree_attr_val = aio_xml_node_content((xmlNode *)xml_curr_attr);

          tree_node_attr->val = aio_malloc(sizeof(*tree_node_attr->val));
          strcpy(tree_node_attr->val, tree_attr_val);

          ++tree_node->attrc;
        }

        xml_curr_attr = xml_curr_attr->next;
      } /* while */

      /* Load children and content if available */
      if (curr_node->children) {
        switch (curr_node->children->type) {
          /* 
            According to XML_PARSE_NOBLANKS option TEXT NODE is the content.
            Without the XML_PARSE_NOBLANKS option leading spaces, white spaces,
            new lines, tabs... would be cause libxml to yields extra TEXT NODE
           */
          case XML_TEXT_NODE:
            tree_node->val = aio_malloc(sizeof(*tree_node->val));
            strcpy(tree_node->val,
                   (const char *)curr_node->children->content);
            break;

          /* Load child nodes */
          case XML_ELEMENT_NODE:

            _AIO_TREE_LOAD_TO(curr_node->children,
                              tree_node->chld,
                              tree_node);

            break;

          default:
            break;
        } // switch
      } else {
        if (curr_node->content) {
          tree_node->val = aio_malloc(sizeof(*tree_node->val));
          strcpy(tree_node->val,
                 (const char *)curr_node->content);
        }
      }

    } // if

    curr_node = curr_node->next;
  }

  return 0;
}
