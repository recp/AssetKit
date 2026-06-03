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

static
bool
ak__cameraWorldFromNode(AkNode * __restrict node,
                        AkNode * __restrict target,
                        mat4                 parentWorld,
                        mat4                 world) {
  AkInstanceNode *inst;
  AkNode    *chld;
  mat4       local;
  mat4       nodeWorld;

  if (!node || !target)
    return false;

  ak_transformCombine(node->transform, local[0]);
  glm_mat4_mul(parentWorld, local, nodeWorld);

  if (node == target) {
    glm_mat4_copy(nodeWorld, world);
    return true;
  }

  if (node->node) {
    for (inst = node->node; inst; inst = inst->next) {
      AkNode *instNode;

      instNode = ak_instanceNodeTarget(inst);
      if (instNode && ak__cameraWorldFromNode(instNode,
                                             target,
                                             nodeWorld,
                                             world))
        return true;
    }
  }

  for (chld = node->chld; chld; chld = chld->next) {
    if (ak__cameraWorldFromNode(chld, target, nodeWorld, world))
      return true;
  }

  return false;
}

static
bool
ak__cameraWorldFromScene(AkScene * __restrict scene,
                         AkNode  * __restrict target,
                         float   * __restrict matrix) {
  mat4    identity = GLM_MAT4_IDENTITY_INIT;
  mat4    world;

  if (!scene || !target || !matrix)
    return false;

  if (ak__cameraWorldFromNode(scene->node, target, identity, world)) {
    glm_mat4_copy(world, (vec4 *)matrix);
    return true;
  }

  return false;
}

AK_EXPORT
AkResult
ak_firstCamera(AkDoc     * __restrict doc,
               AkCamera ** camera,
               float     * matrix,
               float     * projMatrix) {
  AkHeap        *heap;
  AkScene       *scene;
  AkNode        *camNode;
  AkCamera      *cam;

  if (!doc->scene)
    goto efound;

  scene = doc->scene;
  if (!scene->firstCamNode)
    goto efound;

  heap    = ak_heap_getheap(doc);
  camNode = scene->firstCamNode;

  /* view matrix */
  if (matrix
      && !ak__cameraWorldFromScene(scene, camNode, matrix))
    ak_transformCombineWorld(camNode, matrix);

  if (camera || projMatrix) {
    cam = ak_instanceObject(camNode->camera);

    if (!cam) {
      if (ak_opt_get(AK_OPT_ADD_DEFAULT_CAMERA)) {
        AkInstanceBase *cameraInst;
        cam = (AkCamera *)ak_defaultCamera(camNode);

        cameraInst = ak_nodeAttachCamera(camNode, cam);
        if (!scene->cameras)
          scene->cameras = ak_heap_calloc(heap, scene, sizeof(*scene->cameras));
        ak_instanceListEmpty(scene->cameras);
        ak_instanceListAdd(scene->cameras, cameraInst);

        ak_libAddCamera(doc, cam);
      } else {
        goto efound;
      }
    }

    if (camera)
      *camera = cam;

    if (projMatrix) {
      switch ((int)cam->optics->proj->type) {
        case AK_PROJECTION_PERSPECTIVE: {
          AkPerspective *perspective;
          perspective = (AkPerspective *)cam->optics->proj;

          glm_perspective(perspective->yfov,
                          perspective->aspectRatio,
                          perspective->znear,
                          perspective->zfar,
                          (vec4 *)projMatrix);
          break;
        }

        case AK_PROJECTION_ORTHOGRAPHIC: {
          AkOrthographic *ortho;
          ortho = (AkOrthographic *)cam->optics->proj;

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
  AkHeap         *heap;
  AkCamera       *cam;
  const AkCamera *defcam;

  defcam = ak_def_camera();

  if (memparent)
    heap = ak_heap_getheap(memparent);
  else
    heap = ak_heap_default();

  cam = ak_heap_calloc(heap, memparent, sizeof(*cam));
  memcpy(cam, defcam, sizeof(*defcam));

  cam->optics       = ak_heap_calloc(heap, cam, sizeof(*cam->optics));
  cam->optics->proj = ak_heap_calloc(heap, cam, sizeof(AkPerspective));

  memcpy(cam->optics->proj, defcam->optics->proj, sizeof(AkPerspective));

  return cam;
}

AK_EXPORT
AkCamera *
ak_camMakePerspective(AkDoc * __restrict doc,
                      void  * __restrict memparent,
                      float yfov,
                      float aspect,
                      float znear,
                      float zfar) {
  AkHeap        *heap;
  AkCamera      *cam;
  AkPerspective *persp;

  if (!doc) return NULL;

  heap        = ak_heap_getheap(doc);
  cam         = ak_heap_calloc(heap, memparent ? memparent : (void *)doc,
                                     sizeof(*cam));
  cam->optics = ak_heap_calloc(heap, cam, sizeof(*cam->optics));

  /* Concrete projection lives in the camera's heap region so it's
     freed with the camera. */
  persp              = ak_heap_calloc(heap, cam, sizeof(*persp));
  persp->base.type   = AK_PROJECTION_PERSPECTIVE;
  persp->yfov        = yfov;
  persp->aspectRatio = aspect;
  persp->znear       = znear;
  persp->zfar        = zfar;
  cam->optics->proj  = (AkProjection *)persp;

  ak_libAddCamera(doc, cam);
  return cam;
}

AK_EXPORT
AkCamera *
ak_camMakeOrthographic(AkDoc * __restrict doc,
                       void  * __restrict memparent,
                       float xmag,
                       float ymag,
                       float aspect,
                       float znear,
                       float zfar) {
  AkHeap         *heap;
  AkCamera       *cam;
  AkOrthographic *ortho;

  if (!doc) return NULL;

  heap        = ak_heap_getheap(doc);
  cam         = ak_heap_calloc(heap, memparent ? memparent : (void *)doc,
                                     sizeof(*cam));
  cam->optics = ak_heap_calloc(heap, cam, sizeof(*cam->optics));

  ortho              = ak_heap_calloc(heap, cam, sizeof(*ortho));
  ortho->base.type   = AK_PROJECTION_ORTHOGRAPHIC;
  ortho->xmag        = xmag;
  ortho->ymag        = ymag;
  ortho->aspectRatio = aspect;
  ortho->znear       = znear;
  ortho->zfar        = zfar;
  cam->optics->proj  = (AkProjection *)ortho;

  ak_libAddCamera(doc, cam);
  return cam;
}
