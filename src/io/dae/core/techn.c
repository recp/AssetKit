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

#include "techn.h"
#include "../common.h"

static
void
dae_techn_copy_attrs(AkHeap     * __restrict heap,
                     AkTreeNode * __restrict node,
                     xml_t      * __restrict xml) {
  AkTreeNodeAttr *att;
  xml_attr_t     *xatt;

  xatt = xml->attr;
  while (xatt) {
    att       = ak_heap_calloc(heap, node, sizeof(*att));
    att->name = ak_heap_strndup(heap, att, xatt->name, xatt->namesize);
    att->val  = ak_heap_strndup(heap, att, xatt->val,  xatt->valsize);

    att->next = node->attribs;
    if (node->attribs)
      node->attribs->prev = att;
    node->attribs = att;
    node->attrc++;

    xatt = xatt->next;
  }
}

AK_HIDE
void
dae_techn_append_extra(xml_t  * __restrict xml,
                       AkHeap * __restrict heap,
                       void   * __restrict owner) {
  AkTreeNode *root, *node, *child, *tail;
  AkTreeNode *subtree;
  size_t      namelen;

  if (!xml || !heap || !owner)
    return;

  root = ak_extra(owner);
  if (!root) {
    root = ak_heap_calloc(heap, owner, sizeof(*root));
    ak_extra_set(owner, root);
  }

  node = ak_heap_calloc(heap, root, sizeof(*node));
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

  dae_techn_copy_attrs(heap, node, xml);

  subtree = tree_fromxml(heap, node, xml);
  child   = subtree ? subtree->chld : NULL;
  if (child) {
    node->chld = child;
    tail = child;
    while (tail) {
      tail->parent = node;
      node->chldc++;
      if (!tail->next)
        break;
      tail = tail->next;
    }
  }

  node->parent = root;
  node->next   = root->chld;
  if (root->chld)
    root->chld->prev = node;
  root->chld = node;
  root->chldc++;
}

AkTechnique*
dae_techn(xml_t  * __restrict xml,
          AkHeap * __restrict heap,
          void   * __restrict memp) {
  AkTechnique *techn;

  techn          = ak_heap_calloc(heap, memp, sizeof(*techn));
  techn->profile = DAE_XMLA_STRDUP8(xml, heap, profile, techn);
  techn->xmlns   = DAE_XMLA_STRDUP8(xml, heap, xmlns, techn);
  techn->chld    = tree_fromxml(heap, techn, xml);

  dae_techn_append_extra(xml, heap, memp);

  return techn;
}
