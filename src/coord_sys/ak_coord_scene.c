/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "../../include/ak/coord-util.h"
#include "ak_coord_common.h"
#include <cglm/cglm.h>

AK_EXPORT
void
ak_fixSceneCoordSys(struct AkVisualScene * __restrict scene) {
  AkHeap         *heap;
  AkCoordSys     *newCoordSys;
  AkNode         *node;

  heap        = ak_heap_getheap(scene);
  newCoordSys = (void *)ak_opt_get(AK_OPT_COORD);
  node        = scene->node;
  while (node) {
    if (!node->transform)
      node->transform = ak_heap_calloc(heap,
                                       node,
                                       sizeof(*node->transform));

    /* fixup transform[s] */
    ak_coordFindTransform(node->transform,
                          ak_getCoordSys(node),
                          newCoordSys);

    node = node->next;
  }
}
