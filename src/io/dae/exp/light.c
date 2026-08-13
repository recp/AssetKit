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

#include <math.h>

#define DAE_EXP_ATTENUATION_RANGE_EPSILON 0.01f

AK_HIDE
bool
dae_prepare_extra_light(DAEExpState * __restrict st,
                        AkLight     * __restrict light) {
  if (!light)
    return true;

  return dae_prepare_extra_object(st->lights,
                                  &st->lightCount,
                                  &st->extraLights,
                                  &st->lastExtraLight,
                                  light);
}

static
DAEExpName
dae_light_tag(AkLightBase * __restrict base) {
  if (!base)
    return DAE_EXP_NAME(point);

  switch (base->type) {
    case AK_LIGHT_TYPE_AMBIENT:     return DAE_EXP_NAME(ambient);
    case AK_LIGHT_TYPE_DIRECTIONAL: return DAE_EXP_NAME(directional);
    case AK_LIGHT_TYPE_POINT:       return DAE_EXP_NAME(point);
    case AK_LIGHT_TYPE_SPOT:        return DAE_EXP_NAME(spot);
    default:                        return DAE_EXP_NAME(point);
  }
}

static
void
dae_write_color3(DAEExpWriter * __restrict w,
                 AkColor      * __restrict color,
                 float                     intensity) {
  if (!isfinite(intensity))
    intensity = 1.0f;

  dae_w_ch(w, '<');
  dae_w_name(w, DAE_EXP_NAME(color));
  dae_w_ch(w, '>');
  dae_w_float_fast(w, (color ? color->rgba.R : 1.0f) * intensity);
  dae_w_ch(w, ' ');
  dae_w_float_fast(w, (color ? color->rgba.G : 1.0f) * intensity);
  dae_w_ch(w, ' ');
  dae_w_float_fast(w, (color ? color->rgba.B : 1.0f) * intensity);
  dae_w_lit(w, "</");
  dae_w_name(w, DAE_EXP_NAME(color));
  dae_w_ch(w, '>');
}

static
bool
dae_light_attenuation_default(AkLightAttenuation * __restrict attn) {
  return attn
         && attn->constant == 1.0f
         && attn->linear == 0.0f
         && attn->quadratic == 0.0f;
}

static
void
dae_write_attenuation(DAEExpWriter       * __restrict w,
                      AkLightAttenuation * __restrict attn);

static
void
dae_write_range_attenuation(DAEExpWriter * __restrict w, float range) {
  float quadratic;

  if (!isfinite(range) || range <= 0.0f) {
    dae_write_attenuation(w, NULL);
    return;
  }

  quadratic = ((1.0f / DAE_EXP_ATTENUATION_RANGE_EPSILON) - 1.0f)
              / (range * range);
  dae_write_float_elem(w, DAE_EXP_NAME(const_attn), 1.0f);
  dae_write_float_elem(w, DAE_EXP_NAME(linear_attn), 0.0f);
  dae_write_float_elem(w, DAE_EXP_NAME(quad_attn), quadratic);
}

static
void
dae_write_attenuation(DAEExpWriter       * __restrict w,
                      AkLightAttenuation * __restrict attn) {
  dae_write_float_elem(w, DAE_EXP_NAME(const_attn),
                       attn ? attn->constant : 1.0f);
  dae_write_float_elem(w, DAE_EXP_NAME(linear_attn),
                       attn ? attn->linear : 0.0f);
  dae_write_float_elem(w, DAE_EXP_NAME(quad_attn),
                       attn ? attn->quadratic : 0.0f);
}

static
void
dae_write_float_elem_sid(DAEExpWriter * __restrict w,
                         DAEExpName                tag,
                         const char   * __restrict sid,
                         float                     val) {
  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_lit(w, " sid=\"");
  dae_w_xml(w, sid, true);
  dae_w_lit(w, "\">");
  dae_w_float_fast(w, val);
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
}

AK_HIDE
void
dae_write_light(DAEExpState * __restrict st,
                AkLight     * __restrict light,
                uint32_t                 lightIdx) {
  DAEExpWriter *w;
  AkLightBase  *base;
  DAEExpName    tag;

  w    = &st->w;
  base = light ? light->data : NULL;
  tag  = dae_light_tag(base);

  dae_w_lit(w, "<light id=\"");
  dae_w_id(w, DAE_EXP_NAME(light), lightIdx);
  if (light && light->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, light->name, true);
  }
  dae_w_lit(w, "\"><technique_common><");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
  dae_write_color3(w,
                   base ? &base->color : NULL,
                   base ? base->intensity : 1.0f);

  if (base && base->type == AK_LIGHT_TYPE_POINT) {
    AkPointLight *point;

    point = (AkPointLight *)base;
    if (dae_light_attenuation_default(&point->attenuation) && base->range > 0.0f)
      dae_write_range_attenuation(w, base->range);
    else
      dae_write_attenuation(w, &point->attenuation);
  } else if (base && base->type == AK_LIGHT_TYPE_SPOT) {
    AkSpotLight *spot;
    const char  *falloffSid;

    spot = (AkSpotLight *)base;
    if (dae_light_attenuation_default(&spot->attenuation) && base->range > 0.0f)
      dae_write_range_attenuation(w, base->range);
    else
      dae_write_attenuation(w, &spot->attenuation);
    falloffSid = ak_sid_geta(spot, &spot->outerConeAngle);
    dae_write_float_elem_sid(w,
                             DAE_EXP_NAME(falloff_angle),
                             falloffSid ? falloffSid : "falloff_angle",
                             spot->outerConeAngle
                             * (2.0f * DAE_EXP_RAD_TO_DEG));
    dae_write_float_elem(w, DAE_EXP_NAME(falloff_exp),
                         spot->coneFalloffExponent);
  }

  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_lit(w, "></technique_common>");
  dae_write_extra(w, light ? light->extra : NULL);
  dae_w_lit(w, "</light>");
}
