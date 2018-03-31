/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
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

  glm_vec_scale(skew->aroundAxis, skew->rotateAxis[0] * s, mat[0]);
  glm_vec_scale(skew->aroundAxis, skew->rotateAxis[1] * s, mat[1]);
  glm_vec_scale(skew->aroundAxis, skew->rotateAxis[2] * s, mat[2]);

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
ak_transformCombine(AkNode * __restrict node,
                    float  * matrix) {
  AkObject *transform, *transformGroup;
  mat4      mat = GLM_MAT4_IDENTITY_INIT;
  mat4      tmp;

  if (!node->transform)
    goto ret;

  transformGroup = node->transform->base;

again:
  transform = transformGroup;
  while (transform) {
    switch (transform->type) {
      case AK_TRANSFORM_MATRIX: {
        AkMatrix *matrix;
        matrix = ak_objGet(transform);

        glm_mat4_mul(mat, matrix->val, mat);
        break;
      }
      case AK_TRANSFORM_LOOKAT: {
        AkLookAt *lookAt;
        lookAt = ak_objGet(transform);

        glm_lookat(lookAt->val[0],
                   lookAt->val[1],
                   lookAt->val[2],
                   tmp);

        /* because this is view matrix */
        glm_inv_tr(tmp);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AK_TRANSFORM_ROTATE: {
        AkRotate *rotate;

        rotate = ak_objGet(transform);
        glm_rotate_make(tmp, rotate->val[3], rotate->val);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AK_TRANSFORM_QUAT: {
        AkQuaternion *quat;

        quat = ak_objGet(transform);
        glm_quat_mat4(quat->val, tmp);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AK_TRANSFORM_SCALE: {
        AkScale *scale;
        scale = ak_objGet(transform);

        glm_scale_make(tmp, scale->val);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AK_TRANSFORM_TRANSLATE: {
        AkTranslate *translate;
        translate = ak_objGet(transform);

        glm_translate_make(tmp, translate->val);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
      case AK_TRANSFORM_SKEW: {
        AkSkew *skew;
        skew = ak_objGet(transform);

        ak_transformSkewMatrix(skew, tmp[0]);
        glm_mat4_mul(mat, tmp, mat);
        break;
      }
    }

    transform = transform->next;
  }

  if (transformGroup != node->transform->item) {
    transformGroup = node->transform->item;
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
