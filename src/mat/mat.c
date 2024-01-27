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
#include "../../include/ak/material.h"

AK_EXPORT
AkEffect*
ak_effectForBindMaterial(AkBindMaterial      * __restrict bindMat,
                         AkMeshPrimitive     * __restrict meshPrim,
                         AkInstanceMaterial ** __restrict foundInstMat) {
  AkMaterial         *material;
  AkInstanceMaterial *materialInst;
  AkGeometry         *geom;
  AkMap              *materialMap;
  AkMapItem          *mi;

  if (!meshPrim || !meshPrim->mesh || !meshPrim->mesh->geom)
    return NULL;

  geom         = meshPrim->mesh->geom;
  materialInst = bindMat->tcommon;
  materialMap  = geom->materialMap;

  while (materialInst) {
    /* there is symbol, bind only to specified primitive */
    if (materialInst->symbol) {
      mi = ak_map_find(materialMap, (void *)materialInst->symbol);
      while (mi) {
        if ((AkMeshPrimitive *)mi->data == meshPrim) {
          material = ak_instanceObject(&materialInst->base);
          if (material && material->effect) {
            *foundInstMat = materialInst;
            return ak_instanceObject(&material->effect->base);
          } else {
            return NULL;
          }
        }
        mi = mi->next;
      }
    }

    /* bind to whole geometry, TODO: is this OK ? */
    else {
      material = ak_instanceObject(&materialInst->base);
      if (material && material->effect) {
        *foundInstMat = materialInst;
        return ak_instanceObject(&material->effect->base);
      } else {
        return NULL;
      }
    }

    materialInst = (AkInstanceMaterial *)materialInst->base.next;
  }

  return NULL;
}
