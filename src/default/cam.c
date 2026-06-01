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

#include "cam.h"
#include <cglm/cglm.h>

AkPerspective ak_def_cam_persp = {
  .base = {
	.type = AK_PROJECTION_PERSPECTIVE,
	.tag = 0
  },
  .yfov = GLM_PI_4f,
  .xfov = GLM_PI_2f,
  .aspectRatio = 0.5f,
  .znear = 0.01f,
  .zfar = 100.0f
};

AkOptics ak_def_cam_optics = {
  .proj     = &ak_def_cam_persp.base,
  .reserved = NULL
};

const AkCamera ak_def_cam = {
  .name   = "default",
  .optics = &ak_def_cam_optics
};

const AkCamera*
ak_def_camera(void) {
  return &ak_def_cam;
}
