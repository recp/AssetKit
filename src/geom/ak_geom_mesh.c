/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"

AK_EXPORT
uint32_t
ak_meshInputCount(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;

  prim = mesh->primitive;
  return prim->inputCount + prim->vertices->inputCount - 1;
}
