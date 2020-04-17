/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "mesh_index.h"
#include "mesh_edit_common.h"
#include <stdio.h>
#include <string.h>

void
ak_inputNameBySet(AkInput * __restrict input,
                  char    * __restrict buf) {
  if (!input->semanticRaw)
    return;

  if (input->set > 0)
    sprintf(buf, "%s%d", input->semanticRaw, input->set);
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
