/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
#include "../mem_common.h"
#include "../default/def_light.h"
#include <cglm/cglm.h>

/* this duplicates default light to new light,
   because we want to keep default not modified,
   users may want to modify imported light,
   they don't know this is default or not,
   and we don't wan to force them to check lights are default or not.
 */
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
