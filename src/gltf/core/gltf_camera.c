/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_camera.h"

#define k_name         0
#define k_type         1
#define k_perspective  2
#define k_orthographic 3

void _assetkit_hide
gltf_cameras(json_t * __restrict jcam,
             void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jcams;
  AkLibrary          *lib;
  json_t             *it;

  if (!(jcams = json_array(jcam)))
    return;

  gst       = userdata;
  heap      = gst->heap;
  doc       = gst->doc;
  jcam      = jcams->base.value;
  lib       = ak_heap_calloc(heap, doc, sizeof(*lib));

  while (jcam) {
    AkCamera   *cam;
    AkOptics   *optics;
    json_t     *jtechn;

    cam         = ak_heap_calloc(heap, lib, sizeof(*cam));
    optics      = ak_heap_calloc(heap, cam, sizeof(*optics));
    cam->optics = optics;

    json_objmap_t camMap[] = {
      JSON_OBJMAP_OBJ(_s_gltf_name,         I2P k_name),
      JSON_OBJMAP_OBJ(_s_gltf_type,         I2P k_type),
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
          if (json_key_eq(jtechn, _s_gltf_xfov)) {
            persp->xfov = json_float(jtechn, 0.0f);
          } else if (json_key_eq(jtechn, _s_gltf_yfov)) {
            persp->yfov = json_float(jtechn, 0.0f);
          } else if (json_key_eq(jtechn, _s_gltf_znear)) {
            persp->znear = json_float(jtechn, 0.0f);
          } else if (json_key_eq(jtechn, _s_gltf_zfar)) {
            persp->zfar = json_float(jtechn, 0.0f);
          } else if (json_key_eq(jtechn, _s_gltf_aspectRatio)) {
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

      optics->tcommon = &persp->base;
    } else if (json_val_eqsz(it, _s_gltf_orthographic, it->valsize)) {
      AkOrthographic *ortho;

      ortho            = ak_heap_calloc(heap, optics, sizeof(*ortho));
      ortho->base.type = AK_PROJECTION_ORTHOGRAPHIC;

      if ((it = camMap[k_orthographic].object) && (jtechn = json_json(it))) {
        while (jtechn) {
          if (json_key_eq(jtechn, _s_gltf_xmag)) {
            ortho->xmag = json_float(jtechn, 0.0f);
          } else if (json_key_eq(jtechn, _s_gltf_ymag)) {
            ortho->ymag = json_float(jtechn, 0.0f);
          } else if (json_key_eq(jtechn, _s_gltf_znear)) {
            ortho->znear = json_float(jtechn, 0.0f);
          } else if (json_key_eq(jtechn, _s_gltf_zfar)) {
            ortho->zfar = json_float(jtechn, 0.0f);
          }

          jtechn = jtechn->next;
        }
      }

      if (ortho->ymag && ortho->xmag)
        ortho->aspectRatio = ortho->xmag / ortho->ymag;

      optics->tcommon = &ortho->base;
    }
    
    cam->base.next = lib->chld;
    lib->chld      = (void *)cam;
    
    lib->count++;

    jcam = jcam->next;
  }

  gst->doc->lib.cameras = lib;
}
