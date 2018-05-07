/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../../include/ak/assetkit.h"
#include "../../include/ak/node.h"
#include "../../include/ak/coord-util.h"
#include <assert.h>

AK_EXPORT
void
ak_addSubNode(AkNode * __restrict parent,
              AkNode * __restrict subnode,
              bool                fixCoordSys) {
  assert(parent != subnode);

  if (subnode->parent) {
    if (subnode->parent->chld == subnode) {
      if (subnode->next)
        subnode->parent->chld = subnode->next;
      else
        subnode->parent->chld = subnode->prev;
    }
  }

  if (subnode->next)
    subnode->next->prev = subnode->prev;

  if (subnode->prev)
    subnode->prev->next = subnode->next;

  if (parent->chld)
    parent->chld->prev = subnode;

  subnode->next   = parent->chld;
  subnode->prev   = NULL;
  subnode->parent = parent;
  parent->chld    = subnode;

  /* fix node transforms after attached to new node */
  if (fixCoordSys)
    ak_fixNodeCoordSys(subnode);
}
