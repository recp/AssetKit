/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "cam.h"
#include <cglm/cglm.h>

AkPerspective ak_def_cam_tcommon = {
  .base = {
	.type = AK_PROJECTION_PERSPECTIVE,
	.tag = 0
  },
  .yfov = (float)CGLM_PI_4,
  .xfov = (float)CGLM_PI_2,
  .aspectRatio = 0.5f,
  .znear = 0.01f,
  .zfar = 100.0f
};

AkOptics ak_def_cam_optics = {
  .tcommon   = &ak_def_cam_tcommon.base,
  .technique = NULL
};

const AkCamera ak_def_cam = {
  .name   = "default",
  .optics = &ak_def_cam_optics
};

const AkCamera*
ak_def_camera() {
  return &ak_def_cam;
}
