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

void
ak_meshReIndexInputs(AkMesh * __restrict mesh) {
  RBTree          *tree;
  AkInputBasic    *inp;
  AkMeshPrimitive *prim;
  RBNode          *found;

  tree = rb_newtree_str();
  prim = mesh->primitive;
  if (prim) {
    do {
      inp = mesh->vertices->input;
      while (inp) {
        found = rb_find_node(tree, (void *)inp->semanticRaw);
        if (found) {
          found->val = ((char *)found->val) + 1;
          inp->index = (int32_t)found->val;
        } else {
          rb_insert(tree, (void *)inp->semanticRaw, NULL);
          inp->index = -1;
        }
        inp = inp->next;
      }

      inp = &prim->input->base;
      while (inp) {
        found = rb_find_node(tree, (void *)inp->semanticRaw);
        if (found) {
          found->val = ((char *)found->val) + 1;
          inp->index = (int32_t)found->val;
        } else {
          rb_insert(tree, (void *)inp->semanticRaw, NULL);
          inp->index = -1;
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
        inp = mesh->vertices->input;
        while (inp) {
          found = rb_find_node(tree, (void *)inp->semanticRaw);
          if (found && inp->index == -1 && found->val)
            inp->index = 0;
          inp = inp->next;
        }

        inp = &prim->input->base;
        while (inp) {
          found = rb_find_node(tree, (void *)inp->semanticRaw);
          if (found && inp->index == -1 && found->val)
            inp->index = 0;
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
      } else {
        rb_insert(tree, (void *)inp->semanticRaw, NULL);
        inp->index = -1;
      }
      inp = inp->next;
    }

    if (ak_opt_get(AK_OPT_ZERO_INDEXED_INPUT)) {
      inp = mesh->vertices->input;
      while (inp) {
        found = rb_find_node(tree, (void *)inp->semanticRaw);
        if (found && inp->index == -1 && found->val)
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

  if (input->index > -1)
    sprintf(buf, "%s%d", input->semanticRaw, input->index);
  else
    strcpy(buf, input->semanticRaw);
}
