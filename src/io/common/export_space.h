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

#ifndef io_common_export_space_h
#define io_common_export_space_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"

#include <float.h>
#include <math.h>

AK_INLINE
void
io_export_root(AkDoc      * __restrict doc,
               AkCoordSys * __restrict targetCoord,
               float                   unitScale,
               mat4                    root) {
  AkCoordSys *sourceCoord;
  int         i;

  sourceCoord = doc && doc->coordSys ? doc->coordSys : AK_YUP;
  if (!targetCoord)
    targetCoord = AK_YUP;
  if (!(unitScale > 0.0f) || !isfinite(unitScale))
    unitScale = 1.0f;

  glm_mat4_identity(root);
  if (sourceCoord != targetCoord) {
    vec3 basis;
    vec3 converted;

    for (i = 0; i < 3; i++) {
      glm_vec3_zero(basis);
      basis[i] = 1.0f;
      ak_coordCvtVectorTo(sourceCoord, basis, targetCoord, converted);
      root[i][0] = converted[0];
      root[i][1] = converted[1];
      root[i][2] = converted[2];
    }
  }

  if (unitScale != 1.0f) {
    for (i = 0; i < 3; i++) {
      root[i][0] *= unitScale;
      root[i][1] *= unitScale;
      root[i][2] *= unitScale;
    }
  }
}

/*
 * OBJ, PLY and STL cannot describe their coordinate system or unit. Their
 * importers therefore use AssetKit's canonical unlabelled-file convention:
 * right-handed Y-up coordinates measured in metres. Build that conversion
 * once as the traversal root so the existing SIMD world transform remains
 * the only per-vertex operation.
 */
AK_INLINE
float
io_export_canonical_unit_scale(AkDoc * __restrict doc) {
  double unitDist;

  unitDist = doc && doc->unit ? doc->unit->dist : 1.0;
  return unitDist > 0.0 && isfinite(unitDist) && unitDist <= FLT_MAX
         ? (float)unitDist
         : 1.0f;
}

AK_INLINE
void
io_export_canonical_root(AkDoc * __restrict doc,
                         mat4                root) {
  io_export_root(doc, AK_YUP, io_export_canonical_unit_scale(doc), root);
}

#endif /* io_common_export_space_h */
