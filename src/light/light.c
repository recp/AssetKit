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

#include "../common.h"
#include "../default/light.h"
#include <cglm/cglm.h>

/* this duplicates default light to new light,
   because we want to keep default not modified,
   users may want to modify imported light,
   they don't know this is default or not,
   and we don't wan to force them to check lights are default or not.
 */
AK_EXPORT
AkLight *
ak_lightMake(AkDoc * __restrict doc,
             void  * __restrict memp,
             AkLightType type) {
  AkHeap      *heap;
  AkLight     *light;
  AkLightBase *base;
  size_t       baseSize;

  if (!doc) return NULL;
  heap = ak_heap_getheap(doc);

  /* Pick the right concrete sub-struct for the type. */
  switch (type) {
    case AK_LIGHT_TYPE_POINT:
      baseSize = sizeof(AkPointLight);
      break;
    case AK_LIGHT_TYPE_SPOT:
      baseSize = sizeof(AkSpotLight);
      break;
    default:
      baseSize = sizeof(AkLightBase);
      break;
  }

  light          = ak_heap_calloc(heap, memp ? memp : (void *)doc, sizeof(*light));
  light->data    = ak_heap_calloc(heap, light, baseSize);
  base           = light->data;
  base->type     = type;

  /* White color, full alpha — sensible default for any light kind. */
  base->color.rgba.R = 1.0f;
  base->color.rgba.G = 1.0f;
  base->color.rgba.B = 1.0f;
  base->color.rgba.A = 1.0f;
  base->intensity    = 1.0f;
  base->range        = 0.0f;

  /* Point downward by convention. Ignored for ambient/point but
     harmless to set; directional/spot use it as the beam axis. */
  base->direction[0] =  0.0f;
  base->direction[1] = -1.0f;
  base->direction[2] =  0.0f;

  if (type == AK_LIGHT_TYPE_POINT) {
    AkPointLight *p = (AkPointLight *)base;
    p->attenuation.constant  = 1.0f;
    p->attenuation.linear    = 0.0f;
    p->attenuation.quadratic = 0.0f;
  } else if (type == AK_LIGHT_TYPE_SPOT) {
    AkSpotLight *s = (AkSpotLight *)base;
    s->attenuation.constant  = 1.0f;
    s->attenuation.linear    = 0.0f;
    s->attenuation.quadratic = 0.0f;
    s->innerConeAngle = 0.0f;
    s->outerConeAngle = GLM_PI_4f;
    s->coneFalloffExponent = 1.0f;
  }

  ak_libAddLight(doc, light);
  return light;
}

AK_EXPORT
AkLight*
ak_defaultLight(void * __restrict memparent) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkLight       *light;
  AkCoordSys    *coordsys;
  const AkLight *deflight;

  deflight = ak_def_light();

  if (memparent)
    heap  = ak_heap_getheap(memparent);
  else
    heap = ak_heap_default();

  doc = ak_heap_data(heap);

  light = ak_heap_calloc(heap,
                         memparent,
                         sizeof(*light));
  memcpy(light, deflight, sizeof(*deflight));

  light->data = ak_heap_calloc(heap, light, sizeof(AkDirectionalLight));
  memcpy(light->data, deflight->data, sizeof(AkDirectionalLight));

  /* convert light direction */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
    coordsys = (void *)ak_opt_get(AK_OPT_COORD);
  else
    coordsys = doc->coordSys;

  ak_coordCvtVector(AK_YUP,
                    light->data->direction,
                    coordsys);
  return light;
}
