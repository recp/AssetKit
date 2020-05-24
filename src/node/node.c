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
