/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "../default/ak_def_cam.h"
#include <cglm.h>

AK_EXPORT
AkResult
ak_firstCamera(AkDoc     * __restrict doc,
               AkCamera ** camera,
               float     * matrix,
               float     * projMatrix) {
  AkVisualScene *scene;
  AkNode        *camNode;
  AkCamera      *cam;

  if (!doc->scene.visualScene)
    goto efound;

  scene = ak_instanceObject(doc->scene.visualScene);
  if (!scene->firstCamNode)
    goto efound;

  camNode = scene->firstCamNode;

  /* view matrix */
  if (matrix)
    ak_transformCombineWorld(camNode, matrix);

  if (camera || projMatrix) {
    cam = ak_instanceObject(camNode->camera);

    if (!cam)
      cam = (AkCamera *)ak_defaultCamera();

    if (camera)
      *camera = cam;

    if (projMatrix) {
      switch ((int)cam->optics->tcommon->type) {
        case AK_PROJECTION_PERSPECTIVE: {
          AkPerspective *perspective;
          perspective = (AkPerspective *)cam->optics->tcommon;

          glm_perspective(perspective->yfov,
                          perspective->aspectRatio,
                          perspective->znear,
                          perspective->zfar,
                          (vec4 *)projMatrix);
          break;
        }

        case AK_PROJECTION_ORTHOGRAPHIC: {
          AkOrthographic *ortho;
          ortho = (AkOrthographic *)cam->optics->tcommon;

          glm_ortho(-ortho->xmag,
                     ortho->xmag,
                    -ortho->ymag,
                     ortho->ymag,
                     ortho->znear,
                     ortho->zfar,
                     (vec4 *)projMatrix);
          break;
        }
      }
    }
  }

  return AK_OK;

efound:
  if (camera)
    *camera = NULL;

  return AK_EFOUND;
}


AK_EXPORT
const AkCamera*
ak_defaultCamera() {
  return ak_def_camera();
}
