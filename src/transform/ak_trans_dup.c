/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include <cglm.h>

AK_EXPORT
void
ak_transformDup(AkNode * __restrict srcNode,
                AkNode * __restrict destNode) {
  AkHeap   *heap;
  AkObject *transform, *newTransform, *lastTransform;

  if (!srcNode->transform || !destNode || !srcNode)
    return;

  heap = ak_heap_getheap(srcNode);
  newTransform = ak_heap_calloc(heap,
                                destNode,
                                sizeof(*newTransform));

  transform     = srcNode->transform;
  lastTransform = NULL;

  while (transform) {
    switch (transform->type) {
      case AK_NODE_TRANSFORM_TYPE_MATRIX: {
        AkMatrix *matrix, *newMatrix;

        matrix = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*matrix),
                                   AK_NODE_TRANSFORM_TYPE_MATRIX,
                                   true,
                                   false);
        newMatrix = ak_objGet(newTransform);

        newMatrix->sid = ak_heap_strdup(heap, newTransform, matrix->sid);
        glm_mat4_dup(matrix->val, newMatrix->val);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_LOOK_AT: {
        AkLookAt *lookAt, *newLookAt;

        lookAt = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*lookAt),
                                   AK_NODE_TRANSFORM_TYPE_LOOK_AT,
                                   true,
                                   false);
        newLookAt = ak_objGet(newTransform);

        newLookAt->sid = ak_heap_strdup(heap, newTransform, lookAt->sid);
        glm_vec_dup(lookAt->val[0], newLookAt->val[0]);
        glm_vec_dup(lookAt->val[1], newLookAt->val[1]);
        glm_vec_dup(lookAt->val[2], newLookAt->val[2]);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_ROTATE: {
        AkRotate *rotate, *newRotate;

        rotate = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*rotate),
                                   AK_NODE_TRANSFORM_TYPE_ROTATE,
                                   true,
                                   false);
        newRotate = ak_objGet(newTransform);

        newRotate->sid = ak_heap_strdup(heap, newTransform, rotate->sid);
        glm_vec4_dup(rotate->val, newRotate->val);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_SCALE: {
        AkScale *scale, *newScale;

        scale = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*scale),
                                   AK_NODE_TRANSFORM_TYPE_SCALE,
                                   true,
                                   false);
        newScale = ak_objGet(newTransform);

        newScale->sid = ak_heap_strdup(heap, newTransform, scale->sid);
        glm_vec_dup(scale->val, newScale->val);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_TRANSLATE: {
        AkTranslate *translate, *newTranslate;

        translate = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*translate),
                                   AK_NODE_TRANSFORM_TYPE_TRANSLATE,
                                   true,
                                   false);
        newTranslate = ak_objGet(newTransform);

        newTranslate->sid = ak_heap_strdup(heap, newTransform, translate->sid);
        glm_vec_dup(translate->val, newTranslate->val);
        break;
      }
      case AK_NODE_TRANSFORM_TYPE_SKEW: {
        AkSkew *skew, *newSkew;

        skew = ak_objGet(transform);
        newTransform = ak_objAlloc(heap,
                                   destNode,
                                   sizeof(*skew),
                                   AK_NODE_TRANSFORM_TYPE_SKEW,
                                   true,
                                   false);
        newSkew = ak_objGet(newTransform);

        newSkew->sid = ak_heap_strdup(heap, newTransform, skew->sid);
        newSkew->angle = skew->angle;
        glm_vec_dup(skew->aroundAxis, newSkew->aroundAxis);
        glm_vec_dup(skew->rotateAxis, newSkew->rotateAxis);
        break;
      }
    }

    if (lastTransform)
      lastTransform->next = newTransform;
    else
      destNode->transform = newTransform;

    lastTransform = newTransform;

    transform = transform->next;
  }
}
