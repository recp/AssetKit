/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "tree.h"
#include "common.h"
#include "utils.h"

#include "xml.h"

AkTreeNode* _assetkit_hide
tree_fromxml(AkHeap * __restrict heap,
             void   * __restrict memParent,
             xml_t  * __restrict xml) {
  AkTreeNode     *tree, *root, *node, *pa;
  AkTreeNodeAttr *att;
  xml_t          *xpa;
  xml_attr_t     *xatt;
  
  tree = ak_heap_calloc(heap, memParent, sizeof(*tree));
  root = tree;

  while (xml) {
    if (xml->type == XML_ELEMENT) {
      node = ak_heap_calloc(heap, memParent, sizeof(*node));
      
      if ((xatt = xml->attr)) {
        do {
          att       = ak_heap_calloc(heap, node, sizeof(*att));
          att->name = ak_heap_strndup(heap, att, xatt->name, xatt->namesize);
          att->val  = ak_heap_strndup(heap, att, xatt->val,  xatt->valsize);
          
          att->next     = node->attribs;
          node->attribs = att;

          node->attrc++;
        } while ((xatt = xatt->next));
      }
      
      if (root)
        root->prev = node;

      node->next   = root->chld;
      root->chld   = node;
      node->parent = root;
      root->chldc++;
    } else if (xml->type == XML_STRING) {
      if (xml->parent)
        xml->parent->val = xml_strdup(xml, heap, xml->parent);
    }
    
    if (xml->next) {
      xml = xml->next;
    } else if ((xpa = xml->parent)) {
      do {
        pa = pa->parent;

        if (xpa->type == XML_ELEMENT) {
          
        }
        
        xml = xpa->next;
        xpa = xpa->parent;
      } while (!xml && xpa);
    } else {
      break;
    }
    
    xml = xml->next;
  }

  return tree;
}
