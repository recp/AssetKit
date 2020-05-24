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
#include "../default/cam.h"
#include <cglm/cglm.h>

AK_EXPORT
AkResult
ak_firstCamera(AkDoc     * __restrict doc,
               AkCamera ** camera,
               float     * matrix,
               float     * projMatrix) {
  AkHeap        *heap;
  AkVisualScene *scene;
  AkNode        *camNode;
  AkCamera      *cam;

  if (!doc->scene.visualScene)
    goto efound;

  scene = ak_instanceObject(doc->scene.visualScene);
  if (!scene->firstCamNode)
    goto efound;

  heap    = ak_heap_getheap(doc);
  camNode = scene->firstCamNode;

  /* view matrix */
  if (matrix)
    ak_transformCombineWorld(camNode, matrix);

  if (camera || projMatrix) {
    cam = ak_instanceObject(camNode->camera);

    if (!cam) {
      if (ak_opt_get(AK_OPT_ADD_DEFAULT_CAMERA)) {
        AkInstanceBase *cameraInst;
        cam = (AkCamera *)ak_defaultCamera(camNode);

        cameraInst = ak_instanceMake(heap, camNode, cam);
        ak_instanceListEmpty(scene->cameras);
        ak_instanceListAdd(scene->cameras, cameraInst);

        cameraInst->next = camNode->camera;
        camNode->camera  = cameraInst;

        ak_libAddCamera(doc, cam);
      } else {
        goto efound;
      }
    }

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
ak_defaultCamera(void * __restrict memparent) {
  AkHeap   *heap;
  AkCamera *cam;
  const AkCamera *defcam;

  defcam = ak_def_camera();

  if (memparent)
    heap = ak_heap_getheap(memparent);
  else
    heap = ak_heap_default();

  cam = ak_heap_calloc(heap,
                         memparent,
                         sizeof(*cam));
  memcpy(cam, defcam, sizeof(*defcam));
  cam->optics = ak_heap_calloc(heap,
                               cam,
                               sizeof(*cam->optics));
  cam->optics->tcommon = ak_heap_calloc(heap,
                                        cam,
                                        sizeof(AkPerspective));

  memcpy(cam->optics->tcommon,
         defcam->optics->tcommon,
         sizeof(AkPerspective));

  return cam;
}
