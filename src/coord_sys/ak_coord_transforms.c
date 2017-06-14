/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "ak_coord_common.h"
#include <cglm.h>

AK_EXPORT
void
ak_coordCvtNodeTransforms(AkDoc  * __restrict doc,
                          AkNode * __restrict node) {
  AkCoordSys    *oldCoordSys, *newCoordsys;
  AkObject      *transform, *lastTransform;
  vec3           tmp;
  AkAxisAccessor a0, a1;

  oldCoordSys = doc->coordSys;
  newCoordsys = (void *)ak_opt_get(AK_OPT_COORD);

  ak_coordAxisAccessors(oldCoordSys, newCoordsys, &a0, &a1);

  transform = lastTransform = node->transform->item;

  while (transform) {
    switch (transform->type) {
      case AK_TRANSFORM_MATRIX: {
        AkMatrix *matrix;
        matrix = ak_objGet(transform);

        ak_coordCvtTransform(oldCoordSys,
                             matrix->val,
                             newCoordsys);
        break;
      }
      case AK_TRANSFORM_LOOKAT: {
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
      case AK_TRANSFORM_ROTATE: {
        AkRotate *rotate;
        rotate = ak_objGet(transform);

        AK_CVT_VEC(rotate->val);
        break;
      }
      case AK_TRANSFORM_SCALE: {
        AkScale *scale;
        scale = ak_objGet(transform);

        AK_CVT_VEC_NOSIGN(scale->val);
        break;
      }
      case AK_TRANSFORM_TRANSLATE: {
        AkTranslate *translate;

        translate = ak_objGet(transform);
        AK_CVT_VEC(translate->val);
        break;
      }
      case AK_TRANSFORM_SKEW: {
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
  if (node->nodeType == AK_NODE_TYPE_CAMERA_NODE) {
    AkObject *extraTransformItem;
    ak_coordRotNodeForFixCamOri(doc, node, &extraTransformItem);

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
