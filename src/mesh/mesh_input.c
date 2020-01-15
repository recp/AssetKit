/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../mem_common.h"
#include "mesh_index.h"
#include "mesh_edit_common.h"
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
          inp->index = (int32_t)found->val;
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
