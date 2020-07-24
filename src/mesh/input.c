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
ak_inputNameBySet(AkInput * __restrict input,
                  char    * __restrict buf) {
  if (!input->semanticRaw)
    return;

  if (input->set > 0)
    sprintf(buf, "%s%u", input->semanticRaw, input->set);
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
