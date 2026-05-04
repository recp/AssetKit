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
#include <cglm/cglm.h>

/*
 Note from OpenCOLLADA repo:
 Important assumptions on skew and shears:

 COLLADA uses the RenderMan standard:
 [ 1+s*dx*ex   s*dx*ey   s*dx*ez  0 ]
 [   s*dy*ex 1+s*dy*ey   s*dy*ez  0 ]
 [   s*dz*ex   s*dz*ey 1+s*dz*ez  0 ]
 [         0         0         0  1 ]
 where s = tan(skewAngle), if the axises are normalized
 */
AK_EXPORT
void
ak_transformSkewMatrix(AkSkew * __restrict skew,
                       float  * matrix) {
  mat4 mat;
  float s;

  s = tanf(skew->angle);

  glm_vec3_scale(skew->aroundAxis, skew->rotateAxis[0] * s, mat[0]);
  glm_vec3_scale(skew->aroundAxis, skew->rotateAxis[1] * s, mat[1]);
  glm_vec3_scale(skew->aroundAxis, skew->rotateAxis[2] * s, mat[2]);

  mat[0][0] += 1.0f;
  mat[1][1] += 1.0f;
  mat[2][2] += 1.0f;

  mat[0][3] = mat[1][3] = mat[2][3] =
  mat[3][0] = mat[3][1] = mat[3][2] = 0.0f;

  mat[3][3] = 1.0f;

  glm_mat4_copy(mat, (vec4 *)matrix);
}

AK_EXPORT
void
ak_transformCombine(AkTransform * __restrict transform,
                    float       * __restrict matrix) {
  AkObject *transformItem, *transformGroup;
  mat4      mat = GLM_MAT4_IDENTITY_INIT;
  mat4      tmp;

  if (!transform || !matrix)
    goto ret;

  transformGroup = transform->base;

again:
  transformItem = transformGroup;
  while (transformItem) {
    switch (transformItem->type) {
      case AKT_MATRIX: {
        AkMatrix *matrix;
        matrix = ak_objGet(transformItem);

        glm_mat4_mul(mat, matrix->val, mat);
        break;
      }
      case AKT_LOOKAT: {
        AkLookAt *lookAt;
        lookAt = ak_objGet(transformItem);

        glm_lookat(lookAt->val[0],
                   lookAt->val[1],
                   lookAt->val[2],
                   tmp);

        /* because this is view matrix */
        glm_inv_tr(tmp);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AKT_ROTATE: {
        AkRotate *rotate;

        rotate = ak_objGet(transformItem);
        glm_rotate_make(tmp, rotate->val[3], rotate->val);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AKT_QUATERNION: {
        AkQuaternion *quat;

        quat = ak_objGet(transformItem);
        glm_quat_mat4(quat->val, tmp);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AKT_SCALE: {
        AkScale *scale;
        scale = ak_objGet(transformItem);

        glm_scale_make(tmp, scale->val);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AKT_TRANSLATE: {
        AkTranslate *translate;
        translate = ak_objGet(transformItem);

        glm_translate_make(tmp, translate->val);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AKT_SKEW: {
        AkSkew *skew;
        skew = ak_objGet(transformItem);

        ak_transformSkewMatrix(skew, tmp[0]);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
    }

    transformItem = transformItem->next;
  }

  if (transformGroup != transform->item) {
    transformGroup = transform->item;
    goto again;
  }

ret:
  glm_mat4_copy(mat, (vec4 *)matrix);
}

AK_EXPORT
void
ak_transformFreeBase(AkTransform * __restrict transform) {
  AkObject *transItem, *tofree;

  transItem = transform->base;
  while (transItem) {
    tofree = transItem;
    transItem = transItem->next;
    ak_free(tofree);
  }

  transform->base = NULL;
}

AK_EXPORT
AkObject*
ak_getTransformTRS(AkNode *node, AkTypeId transformType) {
  AkHeap       *heap;
  AkObject     *obj, *it, *prev;

  heap = ak_heap_getheap(node);

  if (node->transform) {
    obj = node->transform->item;

    if (obj) {
      do {
        if (obj->type == transformType)
          return obj;
      } while ((obj = obj->next));
    }
  } else {
    node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
  }

  switch (transformType) {
    case AKT_QUATERNION: {
      AkQuaternion *rot;
      obj = ak_objAlloc(heap, node, sizeof(*rot), AKT_QUATERNION, true);
      rot = ak_objGet(obj);
      glm_quat_identity(rot->val);
      break;
    }
    case AKT_TRANSLATE: {
      obj = ak_objAlloc(heap, node, sizeof(AkTranslate), AKT_TRANSLATE, true);
      break;
    }
    case AKT_SCALE: {
      AkScale *scale;
      obj   = ak_objAlloc(heap, node, sizeof(*scale), AKT_SCALE, true);
      scale = ak_objGet(obj);
      glm_vec3_one(scale->val);
      break;
    }
    default:
      return NULL;
  }

  it = node->transform->item;

  if (!it) {
    node->transform->item = obj;
    return obj;
  }

  prev = NULL;

  while (it) {
    if (transformType < it->type)
      break;
    prev = it;
    it   = it->next;
  }

  if (prev)
    prev->next = obj;
  else
    node->transform->item = obj;
  obj->next = it;

  return obj;
}
