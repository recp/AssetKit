/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "ak_mesh_index.h"
#include "ak_mesh_util.h"
#include "ak_mesh_edit_common.h"
#include <stdio.h>
#include <string.h>

void
ak_meshReIndexInputs(AkMesh * __restrict mesh) {
  AkHeap          *heap;
  AkObject        *meshobj;
  RBTree          *tree;
  AkInputBasic    *inp;
  AkMeshPrimitive *prim;
  RBNode          *found;

  meshobj = ak_objFrom(mesh);
  heap    = ak_heap_getheap(meshobj);
  tree    = rb_newtree_str();
  prim    = mesh->primitive;

  ak_dsSetAllocator(heap->allocator, tree->alc);

  if (prim) {
    do {
      if (mesh->vertices) {
        inp = mesh->vertices->input;
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
      }

      inp = &prim->input->base;
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
        if (mesh->vertices) {
          inp = mesh->vertices->input;
          while (inp) {
            found = rb_find_node(tree, (void *)inp->semanticRaw);
            if (found && !inp->index && found->val)
              inp->isIndexed = true;
            inp = inp->next;
          }
        }

        inp = &prim->input->base;
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
  } else {
    inp = mesh->vertices->input;
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

    if (ak_opt_get(AK_OPT_ZERO_INDEXED_INPUT)) {
      inp = mesh->vertices->input;
      while (inp) {
        found = rb_find_node(tree, (void *)inp->semanticRaw);
        if (found && !inp->index && found->val)
          inp->index = 0;
        inp = inp->next;
      }
    }
  }
  rb_destroy(tree);
}

void
ak_inputNameIndexed(AkInputBasic * __restrict input,
                    char         * __restrict buf) {
  if (!input->semanticRaw)
    return;

  if (input->isIndexed)
    sprintf(buf, "%s%d", input->semanticRaw, input->index);
  else
    strcpy(buf, input->semanticRaw);
}

AkInputBasic*
ak_meshInputGet(AkMeshPrimitive *prim,
                const char      *inputSemantic,
                uint32_t         set) {
  AkInputBasic *inputb;
  AkInput      *input;

  /* first search in primitive */
  input = prim->input;
  while (input) {
    if (!input->base.semanticRaw)
      goto cont1;

    if (strcmp(input->base.semanticRaw, inputSemantic) == 0
        && input->set == set)
      return &input->base;

  cont1:
    input = (AkInput *)input->base.next;
  }

  if (!prim->mesh || !prim->mesh->vertices)
    return NULL;

  inputb = prim->mesh->vertices->input;
  while (inputb) {
    if (!inputb->semanticRaw)
      goto cont2;

    if (strcmp(inputb->semanticRaw, inputSemantic) == 0
        && set == 0)
      return inputb;

  cont2:
    inputb = inputb->next;
  }

  return NULL;
}
