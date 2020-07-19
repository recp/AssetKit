/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tree.h"
#include "common.h"
#include "utils.h"

#include "xml.h"

AkTreeNode* AK_HIDE
tree_fromxml(AkHeap * __restrict heap,
             void   * __restrict memParent,
             xml_t  * __restrict xml) {
  AkTreeNode     *tree, *node, *inode, *pa;
  AkTreeNodeAttr *att;
  xml_t          *root, *xpa;
  xml_attr_t     *xatt;
  size_t          namelen;

  tree = ak_heap_calloc(heap, memParent, sizeof(*tree));

  root = xml;
  xml  = xml->val;
  node = tree;
  pa   = NULL;

  while (xml) {
    switch (xml->type) {
      case XML_ELEMENT: {
        inode = ak_heap_calloc(heap, node, sizeof(*inode));
        namelen = xml->tagsize;

        if (xml->prefix)
          namelen += xml->prefixsize;
        
        inode->name = ak_heap_alloc(heap, inode, namelen + 1);

        if (xml->prefix) {
          memcpy((void *)inode->name, xml->prefix, xml->prefixsize);
          memcpy((void *)(inode->name + xml->prefixsize), xml->tag, xml->tagsize);
        } else {
          memcpy((void *)inode->name, xml->tag, xml->tagsize);
        }

        memset((void *)(inode->name + namelen), '\0', 1);
        
        if ((xatt = xml->attr)) {
          do {
            att       = ak_heap_calloc(heap, inode, sizeof(*att));
            att->name = ak_heap_strndup(heap, att, xatt->name, xatt->namesize);
            att->val  = ak_heap_strndup(heap, att, xatt->val,  xatt->valsize);

            att->next      = inode->attribs;
            inode->attribs = att;

            inode->attrc++;
          } while ((xatt = xatt->next));
        }

        if (node->chld)
          node->chld->prev = inode;

        inode->next   = node->chld;
        node->chld    = inode;
        inode->parent = node;
        node->chldc++;
        
        node = inode;
        
        if (xml->val) {
          xml = xml->val;
          continue;
        }
        break;
      }
      case XML_STRING:
        node->val = xml_strdup(xml, heap, node);
        break;
      default:
        break;
    }

    if (xml->next) {
      xml = xml->next;
    } else if ((xpa = xml->parent) != root) {
      do {
        node = node->parent;
        xml  = xpa->next;
        xpa  = xpa->parent;
        
        if (xpa == root || xml == root)
          goto end;
      } while (!xml && xpa);
    } else {
      break;
    }
  } /* while (xml)  */

end:
  return tree;
}
