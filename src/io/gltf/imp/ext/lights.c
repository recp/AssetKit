/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "lights.h"
#include "../extra.h"

static
AkLightType
gltf_ext_lightType(const json_t * __restrict jtype) {
  if (!jtype)
    return AK_LIGHT_TYPE_POINT;

  if (GLTF_JSON_VAL_EQ(jtype, directional))
    return AK_LIGHT_TYPE_DIRECTIONAL;
  if (GLTF_JSON_VAL_EQ8(jtype, spot))
    return AK_LIGHT_TYPE_SPOT;
  if (GLTF_JSON_VAL_EQ8(jtype, point))
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
    it    = GLTF_JSON_GET8(jlight, type);
    light = ak_lightMake(gst->doc, gst->doc, gltf_ext_lightType(it));
    if (!light)
      goto nxt;

    gltf_extra(gst,
               light,
               GLTF_JSON_GET8(jlight, extras),
               GLTF_JSON_GET(jlight, extensions));

    base = light->data;
    if ((it = GLTF_JSON_GET8(jlight, name)))
      light->name = json_strdup(it, gst->heap, light);

    if ((it = GLTF_JSON_GET8(jlight, color))) {
      json_array_float(base->color.vec, it, 1.0f, 3, true);
      base->color.vec[3] = 1.0f;
    }

    base->intensity = json_float(GLTF_JSON_GET(jlight, intensity), 1.0f);
    base->range     = json_float(GLTF_JSON_GET8(jlight, range),     0.0f);

    if (base->type == AK_LIGHT_TYPE_SPOT
        && (it = GLTF_JSON_GET8(jlight, spot))) {
      spot = (AkSpotLight *)base;
      spot->innerConeAngle = json_float(GLTF_JSON_GET(it, innerConeAngle),
                                        0.0f);
      spot->outerConeAngle = json_float(GLTF_JSON_GET(it, outerConeAngle),
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

  jpunctual = GLTF_JSON_GET(jext, KHR_lights_punctual);
  jlight    = jpunctual ? GLTF_JSON_GET8(jpunctual, light) : NULL;
  if (!jlight)
    return true;

  lightIndex = json_int32(jlight, -1);
  if (lightIndex < 0 || !gst->doc->lib.lights.first)
    return false;

  light = gst->doc->lib.lights.first;
  while (light && lightIndex > 0) {
    light = light->next;
    lightIndex--;
  }

  if (!light)
    return false;

  return ak_nodeAttachLight(node, light) != NULL;
}
