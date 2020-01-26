/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"

AK_EXPORT
uint32_t
ak_meshInputCount(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;
  uint32_t         count;

  count = 0;
  prim  = mesh->primitive;
  while (prim) {
    count += prim->inputCount;
    prim = prim->next;
  }

  return count;
}
