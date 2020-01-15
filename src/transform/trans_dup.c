/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../mem_common.h"
#include <cglm/cglm.h>

AK_EXPORT
void
ak_transformDup(AkNode * __restrict srcNode,
                AkNode * __restrict destNode) {
  AkHeap      *heap;
  AkTransform *trans;
  AkObject    *transItem, *newTransItem, *lastTransItem;

  if (!srcNode || !srcNode->transform || !destNode)
    return;

  heap  = ak_heap_getheap(srcNode);
  trans = ak_heap_calloc(heap, destNode, sizeof(*trans));

  transItem     = srcNode->transform->item;
  lastTransItem = NULL;

  while (transItem) {
    switch (transItem->type) {
      case AKT_MATRIX: {
        AkMatrix *matrix, *newMatrix;

        matrix = ak_objGet(transItem);
        newTransItem = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*matrix),
                                   AKT_MATRIX,
                                   true);
        newMatrix = ak_objGet(newTransItem);
        ak_sid_dup(newTransItem, transItem);
        glm_mat4_copy(matrix->val, newMatrix->val);
        break;
      }
      case AKT_LOOKAT: {
        AkLookAt *lookAt, *newLookAt;

        lookAt = ak_objGet(transItem);
        newTransItem = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*lookAt),
                                   AKT_LOOKAT,
                                   true);
        newLookAt = ak_objGet(newTransItem);
        ak_sid_dup(newTransItem, transItem);

        glm_vec3_copy(lookAt->val[0], newLookAt->val[0]);
        glm_vec3_copy(lookAt->val[1], newLookAt->val[1]);
        glm_vec3_copy(lookAt->val[2], newLookAt->val[2]);
        break;
      }
      case AKT_ROTATE: {
        AkRotate *rotate, *newRotate;

        rotate = ak_objGet(transItem);
        newTransItem = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*rotate),
                                   AKT_ROTATE,
                                   true);
        newRotate = ak_objGet(newTransItem);
        ak_sid_dup(newTransItem, transItem);
        glm_vec4_copy(rotate->val, newRotate->val);
        break;
      }
      case AKT_QUATERNION: {
        AkQuaternion *quat, *newQuat;

        quat = ak_objGet(transItem);
        newTransItem = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*quat),
                                   AKT_QUATERNION,
                                   true);
        newQuat = ak_objGet(newTransItem);
        ak_sid_dup(newTransItem, transItem);
        glm_vec4_copy(quat->val, newQuat->val);
        break;
      }
      case AKT_SCALE: {
        AkScale *scale, *newScale;

        scale = ak_objGet(transItem);
        newTransItem = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*scale),
                                   AKT_SCALE,
                                   true);
        newScale = ak_objGet(newTransItem);
        ak_sid_dup(newTransItem, transItem);
        glm_vec3_copy(scale->val, newScale->val);
        break;
      }
      case AKT_TRANSLATE: {
        AkTranslate *translate, *newTranslate;

        translate = ak_objGet(transItem);
        newTransItem = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*translate),
                                   AKT_TRANSLATE,
                                   true);
        newTranslate = ak_objGet(newTransItem);
        ak_sid_dup(newTransItem, transItem);
        glm_vec3_copy(translate->val, newTranslate->val);
        break;
      }
      case AKT_SKEW: {
        AkSkew *skew, *newSkew;

        skew = ak_objGet(transItem);
        newTransItem = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*skew),
                                   AKT_SKEW,
                                   true);
        newSkew = ak_objGet(newTransItem);
        ak_sid_dup(newTransItem, transItem);

        newSkew->angle = skew->angle;
        glm_vec3_copy(skew->aroundAxis, newSkew->aroundAxis);
        glm_vec3_copy(skew->rotateAxis, newSkew->rotateAxis);
        break;
      }
      default:
        transItem = transItem->next;
        continue;
    }

    if (lastTransItem)
      lastTransItem->next = newTransItem;
    else
      trans->item = newTransItem;
    lastTransItem = newTransItem;

    transItem = transItem->next;
  }

  destNode->transform = trans;
}
