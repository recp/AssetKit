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

#include "light.h"
#include "extra.h"
#include "../strpool.h"

#include <math.h>

static
bool
gltf_float_eq(float a, float b) {
  return fabsf(a - b) < 0.000001f;
}

static
float
gltf_cone_angle(float angle) {
  float maxAngle;

  if (!isfinite(angle) || angle < 0.0f)
    return 0.0f;

  maxAngle = nextafterf(1.57079632679489661923f, 0.0f);
  return angle > maxAngle ? maxAngle : angle;
}

static
bool
gltf_color_default(AkColor color) {
  return gltf_float_eq(color.rgba.R, 1.0f)
         && gltf_float_eq(color.rgba.G, 1.0f)
         && gltf_float_eq(color.rgba.B, 1.0f);
}

static
void
gltf_w_color3(GLTFExpWriter * __restrict w, AkColor color) {
  gltf_w_ch(w, '[');
  gltf_w_float(w, color.rgba.R);
  gltf_w_ch(w, ',');
  gltf_w_float(w, color.rgba.G);
  gltf_w_ch(w, ',');
  gltf_w_float(w, color.rgba.B);
  gltf_w_ch(w, ']');
}

static
void
gltf_write_light(GLTFExpWriter * __restrict w,
                 AkLight       * __restrict light,
                 float                       unitScale) {
  AkLightBase *base;
  bool         comma;

  base = light ? light->data : NULL;
  if (!base) {
    w->result = AK_ERR;
    return;
  }

  comma = false;
  gltf_w_ch(w, '{');

  if (light->name) {
    gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, light->name);
    comma = true;
  }

  if (comma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_type, _s_gltf_type_len);
  switch (base->type) {
    case AK_LIGHT_TYPE_DIRECTIONAL:
      gltf_w_qstr_len(w, _s_gltf_directional, _s_gltf_directional_len);
      break;
    case AK_LIGHT_TYPE_POINT:
      gltf_w_qstr_len(w, _s_gltf_point, _s_gltf_point_len);
      break;
    case AK_LIGHT_TYPE_SPOT:
      gltf_w_qstr_len(w, _s_gltf_spot, _s_gltf_spot_len);
      break;
    default:
      w->result = AK_ERR;
      break;
  }
  comma = true;

  if (!gltf_color_default(base->color)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_color, _s_gltf_color_len);
    gltf_w_color3(w, base->color);
    comma = true;
  }

  if (!gltf_float_eq(base->intensity, 1.0f)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_intensity, _s_gltf_intensity_len);
    gltf_w_float(w, base->intensity);
    comma = true;
  }

  if (base->range > 0.0f) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_range, _s_gltf_range_len);
    gltf_w_float(w, base->range * unitScale);
    comma = true;
  }

  if (base->type == AK_LIGHT_TYPE_SPOT) {
    AkSpotLight *spot;
    float        innerConeAngle;
    float        outerConeAngle;

    spot = (AkSpotLight *)base;
    innerConeAngle = gltf_cone_angle(spot->innerConeAngle);
    outerConeAngle = gltf_cone_angle(spot->outerConeAngle);
    if (innerConeAngle > outerConeAngle)
      innerConeAngle = outerConeAngle;

    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_spot, _s_gltf_spot_len);
    gltf_w_ch(w, '{');
    gltf_w_key(w, _s_gltf_innerConeAngle, _s_gltf_innerConeAngle_len);
    gltf_w_float(w, innerConeAngle);
    gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_outerConeAngle, _s_gltf_outerConeAngle_len);
    gltf_w_float(w, outerConeAngle);
    gltf_w_ch(w, '}');
    comma = true;
  }

  gltf_write_extra_extensions_member(w, &comma, ak_extra(light), NULL, NULL);

  if (gltf_extra_has_json_extras(ak_extra(light))) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_extras, _s_gltf_extras_len);
    gltf_write_extra_json_extras(w, ak_extra(light));
  }

  gltf_w_ch(w, '}');
}

void
gltf_write_lights_punctual_extension(GLTFExpWriter * __restrict w,
                                     GLTFExpState  * __restrict st) {
  size_t i;

  if (st->lights.count == 0)
    return;

  gltf_w_key(w,
             _s_gltf_KHR_lights_punctual,
             _s_gltf_KHR_lights_punctual_len);
  gltf_w_ch(w, '{');
  gltf_w_key(w, _s_gltf_lights, _s_gltf_lights_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->lights.count; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_write_light(w, (AkLight *)st->lights.items[i], st->unitScale);
  }

  gltf_w_ch(w, ']');
  gltf_w_ch(w, '}');
}
