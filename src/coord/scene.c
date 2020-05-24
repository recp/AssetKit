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
#include "../../include/ak/coord-util.h"
#include "common.h"
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
