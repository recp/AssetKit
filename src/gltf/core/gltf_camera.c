/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_camera.h"

void _assetkit_hide
gltf_cameras(AkGLTFState * __restrict gst) {
  AkHeap    *heap;
  AkDoc     *doc;
  json_t    *jcams;
  AkLibItem *lib;
  AkCamera  *last_cam;
  size_t     jcamCount, i;

  heap      = gst->heap;
  doc       = gst->doc;
  lib       = ak_heap_calloc(heap, doc, sizeof(*lib));
  last_cam  = NULL;

  jcams     = json_object_get(gst->root, _s_gltf_cameras);
  jcamCount = json_array_size(jcams);
  for (i = 0; i < jcamCount; i++) {
    AkCamera   *cam;
    AkOptics   *optics;
    json_t     *jcam, *jtechnique;
    const char *sval;

    jcam   = json_array_get(jcams, i);
    cam    = ak_heap_calloc(heap, lib, sizeof(*cam));
    optics = ak_heap_calloc(heap, cam, sizeof(*optics));
    cam->optics = optics;

    if ((sval = json_cstr(jcam, _s_gltf_name)))
      cam->name = ak_heap_strdup(gst->heap, cam, sval);

    if ((sval = json_cstr(jcam, _s_gltf_type))) {
      ak_free(cam);
      continue;
    }

    jtechnique = json_object_get(jcams, sval);
    if (strcasecmp(sval, _s_gltf_perspective)) {
      AkPerspective *persp;
      persp = ak_heap_calloc(heap, optics, sizeof(*persp));
      persp->base.type = AK_PROJECTION_PERSPECTIVE;

      persp->xfov        = json_float(jtechnique, _s_gltf_xfov);
      persp->yfov        = json_float(jtechnique, _s_gltf_yfov);
      persp->znear       = json_float(jtechnique, _s_gltf_znear);
      persp->zfar        = json_float(jtechnique, _s_gltf_zfar);
      persp->aspectRatio = json_float(jtechnique, _s_gltf_zfar);

      if (!persp->aspectRatio
          && persp->yfov
          && persp->xfov) {
        persp->aspectRatio = persp->xfov / persp->yfov;
      } else if (!persp->yfov
                 && persp->aspectRatio
                 && persp->xfov) {
        persp->yfov = persp->xfov / persp->aspectRatio;
      } else if (!persp->xfov
                 && persp->aspectRatio
                 && persp->yfov) {
        persp->xfov = persp->yfov * persp->aspectRatio;
      }

      optics->tcommon = &persp->base;
    } else if (strcasecmp(sval, _s_gltf_orthographic)) {
      AkOrthographic *ortho;
      ortho = ak_heap_calloc(heap, optics, sizeof(*ortho));
      ortho->base.type = AK_PROJECTION_ORTHOGRAPHIC;

      ortho->xmag  = json_float(jtechnique, _s_gltf_xmag);
      ortho->ymag  = json_float(jtechnique, _s_gltf_ymag);
      ortho->znear = json_float(jtechnique, _s_gltf_znear);
      ortho->zfar  = json_float(jtechnique, _s_gltf_zfar);

      if (!ortho->aspectRatio
          && ortho->ymag
          && ortho->xmag) {
        ortho->aspectRatio = ortho->xmag / ortho->ymag;
      }

      optics->tcommon = &ortho->base;
    }

    if (last_cam)
      last_cam->next = cam;
    else
      lib->chld = cam;

    last_cam = cam;
    lib->count++;
  }
}
