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

AK_HIDE
bool
dae_prepare_extra_camera(DAEExpState * __restrict st,
                         AkCamera    * __restrict camera) {
  if (!camera)
    return true;

  return dae_prepare_extra_object(st->cameras,
                                  &st->cameraCount,
                                  &st->extraCameras,
                                  &st->lastExtraCamera,
                                  camera);
}

AK_HIDE
void
dae_write_camera(DAEExpState * __restrict st,
                 AkCamera    * __restrict camera,
                 uint32_t                 cameraIdx) {
  DAEExpWriter *w;
  AkProjection *proj;

  w    = &st->w;
  proj = camera && camera->optics ? camera->optics->proj : NULL;

  dae_w_lit(w, "<camera id=\"");
  dae_w_id(w, DAE_EXP_NAME(camera), cameraIdx);
  if (camera && camera->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, camera->name, true);
  }
  dae_w_lit(w, "\"><optics><technique_common>");

  if (proj && proj->type == AK_PROJECTION_ORTHOGRAPHIC) {
    AkOrthographic *ortho;

    ortho = (AkOrthographic *)proj;
    dae_w_lit(w, "<orthographic>");
    if (ortho->xmag > 0.0f)
      dae_write_float_elem(w, DAE_EXP_NAME(xmag), ortho->xmag);
    if (ortho->ymag > 0.0f)
      dae_write_float_elem(w, DAE_EXP_NAME(ymag), ortho->ymag);
    if (ortho->aspectRatio > 0.0f)
      dae_write_float_elem(w, DAE_EXP_NAME(aspect_ratio), ortho->aspectRatio);
    dae_write_float_elem(w, DAE_EXP_NAME(znear), ortho->znear);
    dae_write_float_elem(w, DAE_EXP_NAME(zfar), ortho->zfar);
    dae_w_lit(w, "</orthographic>");
  } else {
    AkPerspective *persp;

    persp = proj && proj->type == AK_PROJECTION_PERSPECTIVE
              ? (AkPerspective *)proj
              : NULL;
    dae_w_lit(w, "<perspective>");
    if (persp) {
      if (persp->xfov > 0.0f)
        dae_write_float_elem(w, DAE_EXP_NAME(xfov),
                             persp->xfov * DAE_EXP_RAD_TO_DEG);
      if (persp->yfov > 0.0f)
        dae_write_float_elem(w, DAE_EXP_NAME(yfov),
                             persp->yfov * DAE_EXP_RAD_TO_DEG);
      if (persp->aspectRatio > 0.0f)
        dae_write_float_elem(w, DAE_EXP_NAME(aspect_ratio),
                             persp->aspectRatio);
      dae_write_float_elem(w, DAE_EXP_NAME(znear), persp->znear);
      dae_write_float_elem(w, DAE_EXP_NAME(zfar), persp->zfar);
    }
    dae_w_lit(w, "</perspective>");
  }

  dae_w_lit(w, "</technique_common></optics>");
  dae_write_extra(w, camera ? camera->extra : NULL);
  dae_w_lit(w, "</camera>");
}
