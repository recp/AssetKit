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
#include "index.h"
#include "edit_common.h"
#include <stdio.h>
#include <string.h>

void
ak_meshReIndexInputs(AkMesh * __restrict mesh) {
  AkHeap          *heap;
  AkObject        *meshobj;
  RBTree          *tree;
  AkInput         *inp;
  AkMeshPrimitive *prim;
  RBNode          *found;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  tree    = rb_newtree_str();
  prim    = mesh->primitive;

  ak_dsSetAllocator(heap->allocator, tree->alc);

  if (prim) {
    do {
      inp = prim->input;
      while (inp) {
        found = rb_find_node(tree, (void *)inp->semanticRaw);
        if (found) {
          found->val = ((char *)found->val) + 1;
          inp->index = (int32_t)(uintptr_t)found->val;
          inp->isIndexed = true;
        } else {
          rb_insert(tree, (void *)inp->semanticRaw, NULL);
          inp->index = 0;
          inp->isIndexed = false;
        }
        inp = inp->next;
      }

      if (prim->next)
        rb_empty(tree);

      prim = prim->next;
    } while (prim);

    /* check for correct zero postfix e.g. TECOORD0 */
    if (ak_opt_get(AK_OPT_ZERO_INDEXED_INPUT)) {
      prim = mesh->primitive;
      do {
        inp = prim->input;
        while (inp) {
          found = rb_find_node(tree, (void *)inp->semanticRaw);
          if (found && !inp->index && found->val)
            inp->isIndexed = true;
          inp = inp->next;
        }

        rb_empty(tree);
        prim = prim->next;
      } while (prim);
    }
  }

  rb_destroy(tree);
}

void
ak_inputNameIndexed(AkInput * __restrict input,
                    char    * __restrict buf) {
  if (!input->semanticRaw)
    return;

  if (input->isIndexed)
    sprintf(buf, "%s%d", input->semanticRaw, input->index);
  else
    strcpy(buf, input->semanticRaw);
}

AK_EXPORT
void
ak_inputNameBySet(AkInput * __restrict input,
                  char    * __restrict buf) {
//  if (!input->semanticRaw)
//    return;
//
//  if (input->set > 0)
//    sprintf(buf, "%s%u", input->semanticRaw, input->set);
//  else
//    strcpy(buf, input->semanticRaw);

  ak_inputNameIndexed(input, buf);
}

AK_EXPORT
AkInput*
ak_meshInputGet(AkMeshPrimitive *prim,
                const char      *inputSemantic,
                uint32_t         set) {
  AkInput *input;

  /* first search in primitive */
  input = prim->input;
  while (input) {
    if (!input->semanticRaw)
      goto cont1;

    if (strcmp(input->semanticRaw, inputSemantic) == 0
        && input->set == set)
      return input;

  cont1:
    input = input->next;
  }

  return NULL;
}
