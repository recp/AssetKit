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

/* TODO: use mutex */

AK_EXPORT
void
ak_changeCoordSysMesh(AkMesh * __restrict mesh,
                      AkCoordSys * newCoordSys) {
  AkMeshPrimitive *primi;
  AkDoc           *doc;
  AkHeap          *heap;
  AkInput         *input;
  AkMap           *map;
  AkMapItem       *mapi;

  heap = ak_heap_getheap(mesh->geom);
  doc  = heap->data;
  map  = ak_map_new(NULL);

  /* find sources to update */
  if (!(primi = mesh->primitive))
    return;

  while (primi) {
    input = primi->input;
    while (input) {
      /* TODO: other semantics which are depend on coord sys */
      if (input->semantic == AK_INPUT_SEMANTIC_POSITION
          || input->semantic == AK_INPUT_SEMANTIC_NORMAL) {
        if (!input->accessor) {
          input = input->next;
          continue;
        }
        
        ak_map_addptr(map, input->accessor);
      }
      input = input->next;
    }
    primi = primi->next;
  }

  mapi = map->root;
  while (mapi) {
    AkAccessor *acci;
    AkBuffer   *buffi;

    acci  = ak_getId(mapi);
    buffi = acci->buffer;
    if (!buffi) {
      mapi = mapi->next;
      continue;
    }

    /* TODO: INT, DOUBLE.. */
    if (acci->componentType == AKT_FLOAT) {
      ak_coordCvtVectors(doc->coordSys,
                         buffi->data,
                         buffi->length / acci->componentBytes,
                         newCoordSys);
    }
    mapi = mapi->next;
  }

  ak_map_destroy(map);
}
