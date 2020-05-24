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
#include "common.h"
#include <cglm/cglm.h>

AK_EXPORT
void
ak_coordCvtNodeTransforms(AkDoc  * __restrict doc,
                          AkNode * __restrict node) {
  AkCoordSys    *oldCoordSys, *newCoordsys;
  AkObject      *transform, *lastTransform;
  vec3           tmp;
  AkAxisAccessor a0, a1;

  if (!node->transform)
    return;

  oldCoordSys = doc->coordSys;
  newCoordsys = (void *)ak_opt_get(AK_OPT_COORD);

  ak_coordAxisAccessors(oldCoordSys, newCoordsys, &a0, &a1);

  transform = lastTransform = node->transform->item;

  while (transform) {
    switch (transform->type) {
      case AKT_MATRIX: {
        AkMatrix *matrix;
        matrix = ak_objGet(transform);

        ak_coordCvtTransform(oldCoordSys,
                             matrix->val,
                             newCoordsys);
        break;
      }
      case AKT_LOOKAT: {
        AkLookAt *lookAt;
        lookAt = ak_objGet(transform);

        /* convert eye vector */
        AK_CVT_VEC(lookAt->val[0]);

        /* convert center vector */
        AK_CVT_VEC(lookAt->val[1]);

        /* convert up vector */
        AK_CVT_VEC(lookAt->val[2]);
        break;
      }
      case AKT_ROTATE: {
        AkRotate *rotate;
        rotate = ak_objGet(transform);

        AK_CVT_VEC(rotate->val);
        break;
      }
      case AKT_QUATERNION: {
        AkQuaternion *quat;
        float        *val;

        quat = ak_objGet(transform);
        val  = &quat->val[1];

        AK_CVT_VEC(val);
        break;
      }
      case AKT_SCALE: {
        AkScale *scale;
        scale = ak_objGet(transform);

        AK_CVT_VEC_NOSIGN(scale->val);
        break;
      }
      case AKT_TRANSLATE: {
        AkTranslate *translate;

        translate = ak_objGet(transform);
        AK_CVT_VEC(translate->val);
        break;
      }
      case AKT_SKEW: {
        AkSkew *skew;
        skew = ak_objGet(transform);

        /* TODO: */
        break;
      }
    }

    lastTransform = transform;
    transform = transform->next;
  }

  /* extra rotation for camera orientation */
  if (node->flags & AK_NODEF_FIXED_COORD) {
    AkObject *extraTransformItem;
    ak_coordRotNodeForFixedCoord(doc, node, &extraTransformItem);

    if (extraTransformItem) {
      if (lastTransform)
        lastTransform->next = extraTransformItem;
      else {
        node->transform = ak_heap_calloc(ak_heap_getheap(extraTransformItem),
                                         node,
                                         sizeof(*node->transform));
        node->transform->item = extraTransformItem;
      }
    }
  }
}
