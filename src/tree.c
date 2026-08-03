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

#define AK_TREE_XML_MAX_DEPTH 512u

static
void
tree_children_fromxml(AkHeap    * __restrict heap,
                      AkTreeNode * __restrict parent,
                      xml_t      * __restrict xml,
                      uint32_t                depth) {
  AkTreeNode     *node;
  AkTreeNodeAttr *att;
  xml_attr_t     *xatt;
  size_t          namelen;

  if (!xml || depth > AK_TREE_XML_MAX_DEPTH)
    return;

  for (xml = xml->val; xml; xml = xml->next) {
    if (xml->type == XML_STRING) {
      parent->val = xml_strdup(xml, heap, parent);
      continue;
    }
    if (xml->type != XML_ELEMENT)
      continue;

    node = ak_heap_calloc(heap, parent, sizeof(*node));
    namelen = xml->tagsize;
    if (xml->prefix)
      namelen += xml->prefixsize;

    node->name = ak_heap_alloc(heap, node, namelen + 1);
    if (xml->prefix) {
      memcpy((void *)node->name, xml->prefix, xml->prefixsize);
      memcpy((void *)(node->name + xml->prefixsize), xml->tag, xml->tagsize);
    } else {
      memcpy((void *)node->name, xml->tag, xml->tagsize);
    }
    memset((void *)(node->name + namelen), '\0', 1);

    for (xatt = xml->attr; xatt; xatt = xatt->next) {
      att       = ak_heap_calloc(heap, node, sizeof(*att));
      att->name = ak_heap_strndup(heap, att, xatt->name, xatt->namesize);
      att->val  = ak_heap_strndup(heap, att, xatt->val, xatt->valsize);
      att->next = node->attribs;
      node->attribs = att;
      node->attrc++;
    }

    if (parent->chld)
      parent->chld->prev = node;
    node->next   = parent->chld;
    node->parent = parent;
    parent->chld = node;
    parent->chldc++;

    tree_children_fromxml(heap, node, xml, depth + 1u);
  }
}

AK_HIDE
AkTreeNode*
tree_fromxml(AkHeap * __restrict heap,
             void   * __restrict memParent,
             xml_t  * __restrict xml) {
  AkTreeNode *tree;

  if (!ak_opt_get(AK_OPT_PRESERVE_EXTRAS))
    return NULL;

  tree = ak_heap_calloc(heap, memParent, sizeof(*tree));
  tree_children_fromxml(heap, tree, xml, 0u);
  return tree;
}
