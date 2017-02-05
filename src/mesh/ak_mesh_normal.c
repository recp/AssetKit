/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_mesh_util.h"
#include "../ak_common.h"
#include "../ak_id.h"
#include "../ak_memory_common.h"
#include <cglm.h>

AK_EXPORT
bool
ak_meshPrimNeedsNormals(AkMeshPrimitive * __restrict prim) {
  AkAccessor *acc;
  AkSource   *src;
  AkObject   *data;
  AkInput    *input;
  bool        ret;

  ret   = true;
  input = prim->input;
  while (input) {
    if (input->base.semantic == AK_INPUT_SEMANTIC_NORMAL) {
      src = ak_getObjectByUrl(&input->base.source);
      if (!src
          || !(acc = src->tcommon)
          || !(data = ak_getObjectByUrl(&acc->source)))
        return ret;
      ret = false;
      break;
    }

    input = (AkInput *)input->base.next;
  }

  return ret;
}

AK_EXPORT
bool
ak_meshNeedsNormals(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;
  bool ret;

  ret  = false;
  prim = mesh->primitive;
  while (prim) {
    ret |= ak_meshPrimNeedsNormals(prim);
    if (ret)
      break;
    prim = prim->next;
  }

  return ret;
}
