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

#include "camera.h"
#include "../extra.h"

#define k_name         0
#define k_type         1
#define k_perspective  2
#define k_orthographic 3

AK_HIDE
void
gltf_cameras(json_t * __restrict jcam,
             void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jcams;
  json_t             *it;
  size_t              cameraIndex;

  if (!(jcams = json_array(jcam)))
    return;

  gst       = userdata;
  heap      = gst->heap;
  doc       = gst->doc;
  jcam      = jcams->base.value;
  gst->camerasCount   = jcams->count;
  gst->camerasByIndex = ak_heap_calloc(heap,
                                       gst->tmpParent,
                                       sizeof(*gst->camerasByIndex)
                                       * gst->camerasCount);
  cameraIndex = gst->camerasCount;

  while (jcam) {
    AkCamera   *cam;
    AkOptics   *optics;
    json_t     *jtechn;

    cam         = ak_heap_calloc(heap, doc, sizeof(*cam));
    ak_setypeid(cam, AKT_CAMERA);
    optics      = ak_heap_calloc(heap, cam, sizeof(*optics));
    cam->optics = optics;

    gltf_extra(gst,
               cam,
               GLTF_JSON_GET8(jcam, extras),
               GLTF_JSON_GET(jcam, extensions));

    json_objmap_t camMap[] = {
      GLTF_JSON_OBJMAP_OBJ8(name,           I2P k_name),
      GLTF_JSON_OBJMAP_OBJ8(type,           I2P k_type),
      JSON_OBJMAP_OBJ(_s_gltf_perspective,  I2P k_perspective),
      JSON_OBJMAP_OBJ(_s_gltf_orthographic, I2P k_orthographic)
    };

    json_objmap(jcam, camMap, JSON_ARR_LEN(camMap));

    if ((it = camMap[k_name].object)) {
      cam->name = json_strdup(it, heap, cam);
    }
  
    if (!(it = camMap[k_type].object)) {
      ak_free(cam);
      continue;
    }
    
    if (json_val_eqsz(it, _s_gltf_perspective, it->valsize)) {
      AkPerspective *persp;
      
      persp            = ak_heap_calloc(heap, optics, sizeof(*persp));
      persp->base.type = AK_PROJECTION_PERSPECTIVE;

      if ((it = camMap[k_perspective].object) && (jtechn = json_json(it))) {
        while (jtechn) {
          if (GLTF_JSON_KEY_EQ8(jtechn, xfov)) {
            persp->xfov = json_float(jtechn, 0.0f);
          } else if (GLTF_JSON_KEY_EQ8(jtechn, yfov)) {
            persp->yfov = json_float(jtechn, 0.0f);
          } else if (GLTF_JSON_KEY_EQ8(jtechn, znear)) {
            persp->znear = json_float(jtechn, 0.0f);
          } else if (GLTF_JSON_KEY_EQ8(jtechn, zfar)) {
            persp->zfar = json_float(jtechn, 0.0f);
          } else if (GLTF_JSON_KEY_EQ(jtechn, aspectRatio)) {
            persp->aspectRatio = json_float(jtechn, 0.0f);
          }
          jtechn = jtechn->next;
        }
      }

      if (!persp->aspectRatio && persp->yfov && persp->xfov) {
        persp->aspectRatio = persp->xfov / persp->yfov;
      } else if (!persp->yfov && persp->aspectRatio && persp->xfov) {
        persp->yfov = persp->xfov / persp->aspectRatio;
      } else if (!persp->xfov && persp->aspectRatio && persp->yfov) {
        persp->xfov = persp->yfov * persp->aspectRatio;
      }

      optics->proj = &persp->base;
    } else if (json_val_eqsz(it, _s_gltf_orthographic, it->valsize)) {
      AkOrthographic *ortho;

      ortho            = ak_heap_calloc(heap, optics, sizeof(*ortho));
      ortho->base.type = AK_PROJECTION_ORTHOGRAPHIC;

      if ((it = camMap[k_orthographic].object) && (jtechn = json_json(it))) {
        while (jtechn) {
          if (GLTF_JSON_KEY_EQ8(jtechn, xmag)) {
            ortho->xmag = json_float(jtechn, 0.0f);
          } else if (GLTF_JSON_KEY_EQ8(jtechn, ymag)) {
            ortho->ymag = json_float(jtechn, 0.0f);
          } else if (GLTF_JSON_KEY_EQ8(jtechn, znear)) {
            ortho->znear = json_float(jtechn, 0.0f);
          } else if (GLTF_JSON_KEY_EQ8(jtechn, zfar)) {
            ortho->zfar = json_float(jtechn, 0.0f);
          }

          jtechn = jtechn->next;
        }
      }

      if (ortho->ymag && ortho->xmag)
        ortho->aspectRatio = ortho->xmag / ortho->ymag;

      optics->proj = &ortho->base;
    }
    
    AK_LIB_PREPEND(doc->lib.cameras, cam, next);
    if (cameraIndex > 0)
      gst->camerasByIndex[--cameraIndex] = cam;

    jcam = jcam->next;
  }

}
