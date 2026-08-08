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
#include "extra.h"
#include "../strpool.h"

static
void
gltf_write_perspective(GLTFExpWriter * __restrict w,
                       AkPerspective * __restrict persp,
                       float                       unitScale) {
  bool comma;

  gltf_w_key_str(w,
                 _s_gltf_type,
                 _s_gltf_type_len,
                 _s_gltf_perspective);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_perspective, _s_gltf_perspective_len);
  gltf_w_ch(w, '{');

  comma = false;
  if (persp->aspectRatio > 0.0f) {
    gltf_w_key(w, _s_gltf_aspectRatio, _s_gltf_aspectRatio_len);
    gltf_w_float(w, persp->aspectRatio);
    comma = true;
  }

  if (comma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_yfov, _s_gltf_yfov_len);
  gltf_w_float(w, persp->yfov);

  if (persp->zfar > 0.0f) {
    gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_zfar, _s_gltf_zfar_len);
    gltf_w_float(w, persp->zfar * unitScale);
  }

  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_znear, _s_gltf_znear_len);
  gltf_w_float(w, persp->znear * unitScale);

  gltf_w_ch(w, '}');
}

static
void
gltf_write_orthographic(GLTFExpWriter  * __restrict w,
                        AkOrthographic * __restrict ortho,
                        float                        unitScale) {
  gltf_w_key_str(w,
                 _s_gltf_type,
                 _s_gltf_type_len,
                 _s_gltf_orthographic);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_orthographic, _s_gltf_orthographic_len);
  gltf_w_ch(w, '{');

  gltf_w_key(w, _s_gltf_xmag, _s_gltf_xmag_len);
  gltf_w_float(w, ortho->xmag * unitScale);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_ymag, _s_gltf_ymag_len);
  gltf_w_float(w, ortho->ymag * unitScale);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_zfar, _s_gltf_zfar_len);
  gltf_w_float(w, ortho->zfar * unitScale);
  gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_znear, _s_gltf_znear_len);
  gltf_w_float(w, ortho->znear * unitScale);

  gltf_w_ch(w, '}');
}

void
gltf_write_cameras(GLTFExpWriter * __restrict w,
                   GLTFExpState  * __restrict st) {
  size_t i;

  if (st->cameras.count == 0)
    return;

  gltf_w_key(w, _s_gltf_cameras, _s_gltf_cameras_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->cameras.count; i++) {
    AkCamera     *camera;
    AkProjection *proj;
    bool          comma;

    camera = (AkCamera *)st->cameras.items[i];
    proj   = camera && camera->optics ? camera->optics->proj : NULL;
    if (!proj) {
      w->result = AK_ERR;
      return;
    }

    if (i > 0)
      gltf_w_ch(w, ',');

    comma = false;
    gltf_w_ch(w, '{');

    if (camera->name) {
      gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, camera->name);
      comma = true;
    }

    if (comma)
      gltf_w_ch(w, ',');

    switch (proj->type) {
      case AK_PROJECTION_PERSPECTIVE:
        gltf_write_perspective(w, (AkPerspective *)proj, st->unitScale);
        break;
      case AK_PROJECTION_ORTHOGRAPHIC:
        gltf_write_orthographic(w, (AkOrthographic *)proj, st->unitScale);
        break;
      default:
        w->result = AK_ERR;
        break;
    }
    comma = true;

    gltf_write_extra_extensions_member(w, &comma, ak_extra(camera), NULL, NULL);

    if (gltf_extra_has_json_extras(ak_extra(camera))) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key(w, _s_gltf_extras, _s_gltf_extras_len);
      gltf_write_extra_json_extras(w, ak_extra(camera));
    }

    gltf_w_ch(w, '}');
  }

  gltf_w_ch(w, ']');
}
