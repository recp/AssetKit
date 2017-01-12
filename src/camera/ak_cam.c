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
AkResult
ak_camFirstCamera(AkDoc     * __restrict doc,
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

    if (camera)
      *camera = cam;

    if (projMatrix) {
      switch ((int)cam->optics->techniqueCommon->techniqueType) {
        case AK_TECHNIQUE_COMMON_CAMERA_PERSPECTIVE: {
          AkPerspective *perspective;
          perspective = cam->optics->techniqueCommon->technique;

          glm_perspective(perspective->yfov,
                          perspective->aspectRatio,
                          perspective->znear,
                          perspective->zfar,
                          (vec4 *)projMatrix);
          break;
        }

        case AK_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC: {
          AkOrthographic *ortho;
          ortho = cam->optics->techniqueCommon->technique;

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
