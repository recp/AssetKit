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
             void  * __restrict memparent,
             AkLightType type) {
  AkHeap      *heap;
  AkLight     *light;
  AkLightBase *base;
  size_t       baseSize;

  if (!doc) return NULL;
  heap = ak_heap_getheap(doc);

  /* Pick the right concrete sub-struct for the type. Ambient and
     directional don't have extra fields beyond AkLightBase, so they
     share its allocation. Point and Spot extend the base with
     attenuation (and Spot adds falloff). */
  switch (type) {
    case AK_LIGHT_TYPE_POINT: baseSize = sizeof(AkPointLight); break;
    case AK_LIGHT_TYPE_SPOT:  baseSize = sizeof(AkSpotLight);  break;
    default:                  baseSize = sizeof(AkLightBase);  break;
  }

  light          = ak_heap_calloc(heap, memparent ? memparent : (void *)doc,
                                        sizeof(*light));
  light->tcommon = ak_heap_calloc(heap, light, baseSize);
  base           = light->tcommon;
  base->type     = type;

  /* White color, full alpha — sensible default for any light kind. */
  base->color.rgba.R = 1.0f;
  base->color.rgba.G = 1.0f;
  base->color.rgba.B = 1.0f;
  base->color.rgba.A = 1.0f;

  /* Point downward by convention. Ignored for ambient/point but
     harmless to set; directional/spot use it as the beam axis. */
  base->direction[0] =  0.0f;
  base->direction[1] = -1.0f;
  base->direction[2] =  0.0f;

  /* Per-type attenuation defaults. constAttn=1 means "no attenuation
     at the source"; linear/quad stay 0 so the light reaches forever
     until the consumer caps the range. Spot adds a 30° cone with
     linear falloff. */
  if (type == AK_LIGHT_TYPE_POINT) {
    AkPointLight *p = (AkPointLight *)base;
    p->constAttn  = 1.0f;
    p->linearAttn = 0.0f;
    p->quadAttn   = 0.0f;
  } else if (type == AK_LIGHT_TYPE_SPOT) {
    AkSpotLight *s = (AkSpotLight *)base;
    s->constAttn    = 1.0f;
    s->linearAttn   = 0.0f;
    s->quadAttn     = 0.0f;
    s->falloffAngle = 30.0f;   /* degrees per COLLADA convention */
    s->falloffExp   = 1.0f;
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

  light->tcommon = ak_heap_calloc(heap,
                                  light,
                                  sizeof(AkDirectionalLight));

  memcpy(light->tcommon,
         deflight->tcommon,
         sizeof(AkDirectionalLight));

  /* convert light direction */
  if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) != AK_COORD_CVT_DISABLED)
    coordsys = (void *)ak_opt_get(AK_OPT_COORD);
  else
    coordsys = doc->coordSys;

  ak_coordCvtVector(AK_YUP,
                    light->tcommon->direction,
                    coordsys);
  return light;
}
