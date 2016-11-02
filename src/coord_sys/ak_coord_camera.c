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
ak_coordRotForFixCamOri(AkCoordSys *oldCoordSys,
                        AkCoordSys *newCoordSys,
                        vec4        fwdAxis,
                        vec4        upAxis) {
  ivec3          camOri, camOriNew;
  vec3           v1, v2, v3;
  AkAxisAccessor a0, a1;

  ak_coordAxisAccessors(oldCoordSys, newCoordSys, &a0,  &a1);
  ak_coordAxisToiVec3(oldCoordSys->cameraOrientation, camOri);

  AK_CVT_VEC_TO(camOri, camOriNew)

  glm_vec_broadcast(0.0f, v1);
  glm_vec_broadcast(0.0f, v2);

  v1[abs(camOri[2]) - 1]    = glm_sign(camOri[2]);
  v2[abs(camOriNew[2]) - 1] = glm_sign(camOriNew[2]);
  glm_vec_cross(v1, v2, v3);

  /* angle for forward axis */
  fwdAxis[3] = glm_vec_angle(v1, v2);
  if (fwdAxis[3] != 0.0f) {
    /* forward axis */
    glm_vec_dup(v3, fwdAxis);

    /* now forward1 == forward0 == v1, fix up vector */
    /* v2 is new up */
    glm_vec_cross(v1, v3, v2);
  } else {
    /* forward1 == forward0 [already], Up = Right x Fwd */
    glm_vec_broadcast(0.0f, v3);
    v3[abs(camOri[0]) - 1] = glm_sign(camOri[0]);
    glm_vec_cross(v3, v1, v2);
  }

  glm_vec_broadcast(0.0f, v1);
  v1[abs(camOri[1]) - 1] = glm_sign(camOri[1]);

  /* angle for up axis */
  upAxis[3] = glm_vec_angle(v1, v2);

  /* up direction */
  if (upAxis[3] != 0.0f)
    glm_vec_dup(v2, upAxis);
}

AK_EXPORT
void
ak_coordFixCamOri(AkCoordSys *oldCoordSys,
                  AkCoordSys *newCoordSys,
                  AkFloat4x4  transform) {
  vec4 fwdAxis, upAxis;

  if (ak_coordOrientationIsEq(oldCoordSys, newCoordSys))
    return;

  ak_coordRotForFixCamOri(oldCoordSys,
                          newCoordSys,
                          fwdAxis,
                          upAxis);

  /* apply rotation for forward direction */
  if (fwdAxis[3] != 0.0f)
    glm_rotate(transform, fwdAxis[3], fwdAxis);

  /* apply rotation for up direction */
  if (upAxis[3] != 0.0f)
    glm_rotate(transform, upAxis[3], upAxis);
}

AK_EXPORT
void
ak_coordRotNodeForFixCamOri(AkDoc     *doc,
                            void      *memparent,
                            AkObject **destTransform) {
  AkHeap     *heap;
  AkCoordSys *oldCoordSys, *newCoordSys;
  AkObject   *transformFwd, *transformUp;
  AkRotate   *rotate;
  vec4        fwdAxis, upAxis;

  oldCoordSys = doc->coordSys;
  newCoordSys = ak_defaultCoordSys();

  *destTransform = NULL;
  if (ak_coordOrientationIsEq(oldCoordSys, newCoordSys))
    return;

  ak_coordRotForFixCamOri(oldCoordSys,
                          newCoordSys,
                          fwdAxis,
                          upAxis);

  heap = ak_heap_getheap(doc);

  /* rotation for forward direction */
  if (fwdAxis[3] != 0.0f) {
    transformFwd = ak_objAlloc(heap,
                            memparent,
                            sizeof(*rotate),
                            AK_NODE_TRANSFORM_TYPE_ROTATE,
                            true,
                            false);

    rotate = ak_objGet(transformFwd);
    rotate->sid = ak_heap_strdup(heap, transformFwd, "ak-cam-fix-rot1");
    glm_vec4_dup(fwdAxis, rotate->val);

    *destTransform = transformFwd;
  }

  /* rotation for up direction */
  if (upAxis[3] != 0.0f) {
    transformUp = ak_objAlloc(heap,
                               memparent,
                               sizeof(*rotate),
                               AK_NODE_TRANSFORM_TYPE_ROTATE,
                               true,
                               false);

    rotate = ak_objGet(transformUp);
    rotate->sid = ak_heap_strdup(heap, transformUp, "ak-cam-fix-rot2");
    glm_vec4_dup(fwdAxis, rotate->val);

    if (*destTransform)
      (*destTransform)->next = transformUp;
    else
      *destTransform = transformUp;
  }
}
