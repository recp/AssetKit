/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "lights.h"

static
AkLightType
gltf_ext_lightType(const json_t * __restrict jtype) {
  if (!jtype)
    return AK_LIGHT_TYPE_POINT;

  if (json_val_eq(jtype, _s_gltf_directional))
    return AK_LIGHT_TYPE_DIRECTIONAL;
  if (json_val_eq(jtype, _s_gltf_spot))
    return AK_LIGHT_TYPE_SPOT;
  if (json_val_eq(jtype, _s_gltf_point))
    return AK_LIGHT_TYPE_POINT;

  return AK_LIGHT_TYPE_POINT;
}

AK_HIDE
void
gltf_ext_lights(AkGLTFState * __restrict gst,
                json_t      * __restrict jlights) {
  const json_array_t *jarr;
  json_t             *jlight;
  json_t             *it;
  AkLight            *light;
  AkLightBase        *base;
  AkSpotLight        *spot;

  if (!gst || !(jarr = json_array(jlights)))
    return;

  jlight = jarr->base.value;
  while (jlight) {
    it    = json_get(jlight, _s_gltf_type);
    light = ak_lightMake(gst->doc, gst->doc, gltf_ext_lightType(it));
    if (!light)
      goto nxt;

    base = light->tcommon;
    if ((it = json_get(jlight, _s_gltf_name)))
      light->name = json_strdup(it, gst->heap, light);

    if ((it = json_get(jlight, _s_gltf_color))) {
      json_array_float(base->color.vec, it, 1.0f, 3, true);
      base->color.vec[3] = 1.0f;
    }

    base->intensity = json_float(json_get(jlight, _s_gltf_intensity), 1.0f);
    base->range     = json_float(json_get(jlight, _s_gltf_range),     0.0f);

    if (base->type == AK_LIGHT_TYPE_SPOT
        && (it = json_get(jlight, _s_gltf_spot))) {
      spot = (AkSpotLight *)base;
      spot->innerConeAngle = json_float(json_get(it, _s_gltf_innerConeAngle),
                                        0.0f);
      spot->outerConeAngle = json_float(json_get(it, _s_gltf_outerConeAngle),
                                        GLM_PI_4f);
    }

  nxt:
    jlight = jlight->next;
  }
}

AK_HIDE
bool
gltf_ext_nodeLight(AkGLTFState * __restrict gst,
                   AkNode      * __restrict node,
                   const json_t * __restrict jext) {
  json_t  *jpunctual;
  json_t  *jlight;
  AkLight *light;
  int32_t  lightIndex;

  jpunctual = json_get(jext, _s_gltf_KHR_lights_punctual);
  jlight    = jpunctual ? json_get(jpunctual, _s_gltf_light) : NULL;
  if (!jlight)
    return true;

  lightIndex = json_int32(jlight, -1);
  if (lightIndex < 0 || !gst->doc->lib.lights)
    return false;

  light = (void *)gst->doc->lib.lights->chld;
  while (light && lightIndex > 0) {
    light = light->next;
    lightIndex--;
  }

  if (!light)
    return false;

  return ak_nodeAttachLight(node, light) != NULL;
}
