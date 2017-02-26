/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/ak-map.h"
#include "ak_memory_common.h"

AkMap *
ak_map_new(AkMapCmp cmp) {
  AkMap  *map;
  AkHeap *heap;
  heap = ak_heap_new(NULL, cmp ? cmp : ak_cmp_ptr, NULL);

  map = ak_heap_alloc(heap, NULL, sizeof(*map));
  ak_heap_setdata(heap, map);

  map->heap = heap;
  map->root = NULL;
  return map;
}

void
ak_map_addptr(AkMap *map, void *ptr) {
  AkHeapNode *hnode;
  AkMapItem  *mi;
  AkResult    ret;

  ret = ak_heap_getNodeById(map->heap, ptr, &hnode);
  if (ret == AK_OK)
    return;

  mi = ak_heap_alloc(map->heap, NULL, sizeof(*mi));
  ak_heap_setId(map->heap, ak__alignof(mi), ptr);

  mi->prev = NULL;
  mi->next = map->root;
  if (map->root)
    map->root->prev = mi;
  map->root = mi;
}

void*
ak_map_find(AkMap *map, void *id) {
  void       *mem;
  AkResult    ret;

  ret = ak_heap_getMemById(map->heap, id, &mem);
  if (ret == AK_OK)
    return ((AkMapItem *)mem)->data;

  return NULL;
}

void
ak_map_add(AkMap *map,
           void  *value,
           size_t size,
           void  *id) {
  AkHeapNode *hnode;
  AkMapItem  *mi;
  AkResult    ret;

  ret = ak_heap_getNodeById(map->heap, id, &hnode);
  if (ret == AK_OK)
    return;

  mi = ak_heap_alloc(map->heap, NULL, sizeof(*mi) + size);
  if (size > 0)
    memcpy(mi->data, value, size);

  ak_heap_setId(map->heap, ak__alignof(mi), id);

  mi->prev = NULL;
  mi->next = map->root;
  if (map->root)
    map->root->prev = mi;
  map->root = mi;
}

void
ak_multimap_add(AkMap *map,
                void  *value,
                size_t size,
                void  *id) {
  AkHeapNode *hnode;
  AkMapItem  *mi;
  AkMapItem  *subItm;
  AkResult    ret;

  ret = ak_heap_getNodeById(map->heap, id, &hnode);
  if (ret == AK_OK) {
    AkMapItem *mii;

    mi = ak__alignas(hnode);
    subItm = ak_heap_calloc(map->heap, NULL, sizeof(*mi) + size);
    if (size > 0)
      memcpy(subItm->data, value, size);

    mii = (AkMapItem *)mi->data;
    if (mii)
      mii->prev = subItm;

    subItm->next = mii;
    *(AkMapItem **)mi->data = subItm;
    return;
  }

  mi     = ak_heap_calloc(map->heap, NULL, sizeof(*mi) + sizeof(void *));
  subItm = ak_heap_calloc(map->heap, NULL, sizeof(*mi) + size);
  if (size > 0)
    memcpy(subItm->data, value, size);
  *(AkMapItem **)mi->data = subItm;

  ak_heap_setId(map->heap, ak__alignof(mi), id);

  mi->prev = NULL;
  mi->next = map->root;
  if (map->root)
    map->root->prev = mi;
  map->root = mi;
}

void
ak_map_destroy(AkMap *map) {
  ak_free(map);
}
