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
#include "list.h"
#include "../../include/ak/util.h"
#include <stdio.h>
#include <assert.h>

AK_EXPORT
void
ak_instanceListAdd(AkInstanceList *list,
                   AkInstanceBase *inst) {
  AkHeap             *heap;
  AkInstanceListItem *ili;

  heap = ak_heap_getheap(list);
  ili  = ak_heap_alloc(heap, list, sizeof(*ili));

  ili->instance = inst;
  ili->index    = ++list->lastindex;

  list->count++;

  ili->prev     = list->last;
  ili->next     = NULL;

  if (!list->first)
    list->first = ili;

  if (list->last)
    list->last->next = ili;

  list->last = ili;
}

AK_EXPORT
void
ak_instanceListDel(AkInstanceList *list,
                   AkInstanceListItem *item) {
  if (list->first == item)
    list->first = item->next;

  if (list->last == item)
    list->last = item->prev;

  if (item->prev)
    item->prev->next = item->next;

  if (item->next)
    item->next->prev = item->prev;

  list->count--;

  ak_free(item);
}

AK_EXPORT
void
ak_instanceListEmpty(AkInstanceList *list) {
  AkInstanceListItem *item, *tofree;
  item = list->first;

  while (item) {
    tofree = item;
    item = item->next;
    ak_free(tofree);
  }

  list->first     = NULL;
  list->last      = NULL;
  list->count     = 0;
  list->lastindex = 0;
}

AK_HIDE
void
ak_sceneAddCamera(AkScene *scene, AkInstanceBase *inst) {
  AkSceneCamera *entry;
  AkCamera      *camera;
  AkHeap        *heap;

  if (!scene || !inst || !(camera = ak_instanceObject(inst)))
    return;

  scene->cameras.useCount++;

  for (entry = scene->cameras.first; entry; entry = entry->next) {
    if (entry->camera == camera) {
      entry->useCount++;
      if (!entry->firstInstance)              entry->firstInstance = inst;
      if (!scene->firstCamNode && inst->node) scene->firstCamNode  = inst->node;
      return;
    }
  }

  heap                 = ak_heap_getheap(scene);
  entry                = ak_heap_calloc(heap, scene, sizeof(*entry));
  entry->camera        = camera;
  entry->firstInstance = inst;
  entry->useCount      = 1;

  if (!scene->cameras.first) scene->cameras.first      = entry;
  if (scene->cameras.last)   scene->cameras.last->next = entry;

  scene->cameras.last = entry;
  scene->cameras.count++;

  if (!scene->firstCamNode && inst->node)
    scene->firstCamNode = inst->node;
}

AK_HIDE
void
ak_sceneAddLight(AkScene *scene, AkInstanceBase *inst) {
  AkSceneLight  *entry;
  AkLight       *light;
  AkHeap        *heap;

  if (!scene || !inst || !(light = ak_instanceObject(inst)))
    return;

  scene->lights.useCount++;

  for (entry = scene->lights.first; entry; entry = entry->next) {
    if (entry->light == light) {
      entry->useCount++;
      if (!entry->firstInstance)
        entry->firstInstance = inst;
      return;
    }
  }

  heap                 = ak_heap_getheap(scene);
  entry                = ak_heap_calloc(heap, scene, sizeof(*entry));
  entry->light         = light;
  entry->firstInstance = inst;
  entry->useCount      = 1;

  if (!scene->lights.first) scene->lights.first      = entry;
  if (scene->lights.last)   scene->lights.last->next = entry;

  scene->lights.last = entry;
  scene->lights.count++;
}

AK_HIDE
void
ak_sceneAddItems(AkScene *scene, AkNode *node) {
  AkInstanceBase *inst;

  if (!scene || !node)
    return;

  for (inst = node->camera; inst; inst = inst->next)
    ak_sceneAddCamera(scene, inst);

  for (inst = node->light; inst; inst = inst->next)
    ak_sceneAddLight(scene, inst);
}

char*
ak_instanceName(AkInstanceListItem *item) {
  AkHeap         *heap;
  AkInstanceBase *inst;
  char           *name, *objId;
  void           *obj;
  uint32_t        indexDigit;
  size_t          idlen;

  inst = item->instance;
  assert(inst);

  heap = ak_heap_getheap(item);

  /* instance name */
  if (inst->name)
    return ak_heap_strdup(heap, item, inst->name);

  /* we can use node.name or node.id for instance name */
  if (inst->node) {
    char *nodeid;

    if (inst->node->name)
      return ak_heap_strdup(heap,
                            item,
                            inst->node->name);

    nodeid = ak_getId(inst->node);
    if (nodeid)
      return ak_heap_strdup(heap, item, nodeid);
  }

  obj = ak_instanceObject(inst);
  if (!obj)
    return NULL;

  objId      = ak_getId(obj);
  idlen      = strlen(objId);

  indexDigit = ak_digitsize(item->index);
  name       = ak_heap_alloc(heap,
                             item,
                             idlen + indexDigit + 2);

  memcpy(name, objId, idlen);
  sprintf(name + idlen, "-%zu", item->index);
  name[idlen + indexDigit + 1] = '\0';

  return name;
}
