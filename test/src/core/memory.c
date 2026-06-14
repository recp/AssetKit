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

#include "../test_common.h"
#include "../../../src/mem/common.h"
#include "../../../src/mem/lt.h"

#include <ak/path.h>

#include <limits.h>

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

TEST_IMPL(heap) {
  AkHeap  *heap, *other, staticHeap;
  AkHeapAllocator *defaultAllocator;
  AkHeapStats stats;
  uint32_t heapid, data;

  defaultAllocator = ak_heap_allocator(ak_heap_default());
  heap = ak_heap_new(NULL, NULL, NULL);
  ASSERT(heap->allocator == defaultAllocator);
  ASSERT(ak_heap_allocator(heap) == defaultAllocator);
  ak_heap_getStats(heap, &stats);
  ASSERT(stats.allocCalls == 0);
  ASSERT(stats.peakNodes == 0);

  heapid = heap->heapid;
  ASSERT(heapid > 0);
  ASSERT(ak_heap_lt_find(heap->heapid) == heap);

  data = 0;
  {
    void *mem;

    mem = ak_heap_calloc(heap, NULL, 16);
    ak_heap_getStats(heap, &stats);
    ASSERT(stats.allocCalls == 1);
    ASSERT(stats.callocCalls == 1);
    ASSERT(stats.allocBytes == 16);
    ASSERT(stats.callocBytes == 16);
    ASSERT(stats.liveNodes == 1);
    ASSERT(stats.peakNodes == 1);

    ak_free(mem);
    ak_heap_getStats(heap, &stats);
    ASSERT(stats.freeCalls == 1);
    ASSERT(stats.liveNodes == 0);
  }

  other = ak_heap_new(NULL, NULL, NULL);

  ak_heap_attach(heap, other);
  ASSERT(heap->chld == other);

  ak_heap_dettach(heap, other);
  ASSERT(heap->chld == NULL);

  ak_heap_attach(heap, other);
  ASSERT(heap->chld == other);

  ak_heap_setdata(heap, &data);
  ASSERT(ak_heap_data(heap) == &data);

  ak_heap_destroy(heap);
  ASSERT(ak_heap_lt_find(heapid) == NULL);

  ak_heap_init(&staticHeap, NULL, NULL, NULL);
  ASSERT(staticHeap.heapid > 0);

  ak_heap_lt_remove(staticHeap.heapid);
  ASSERT(ak_heap_lt_find(staticHeap.heapid) == NULL);

  TEST_SUCCESS
}

TEST_IMPL(heap_multiple) {
  AkHeap  *heap, *root;
  uint32_t i;

  root = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc, leak */
  for (i = 0; i < 1000; i++)
    heap = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc 2, leak */
  for (i = 0; i < 1000; i++)
    heap = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc-free 1 */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_destroy(heap);
  }

  /* multiple alloc-free 2 */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_destroy(heap);
  }

  /* multiple alloc, attach to parent */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_attach(root, heap);
  }

  /* multiple alloc, attach to parent */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_attach(root, heap);
  }

  ak_heap_destroy(root);

  root = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc, attach-detach to parent */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_attach(root, heap);
    ak_heap_dettach(root, heap);
  }

  TEST_SUCCESS
}

TEST_IMPL(mesh_triangulate_polygon_sets_triangle_mode) {
  AkHeap       *heap;
  AkPolygon    *poly;
  AkUIntArray  *vcount;
  uint8_t      *items;

  heap  = ak_heap_new(NULL, NULL, NULL);
  poly  = ak_heap_calloc(heap, NULL, sizeof(*poly));

  vcount        = ak_heap_alloc(heap, poly, sizeof(*vcount) + sizeof(AkUInt));
  vcount->count = 1;
  vcount->items[0] = 4;

  poly->base.type        = AK_PRIMITIVE_POLYGONS;
  poly->base.indexStride = 1;
  poly->vcount           = vcount;
  poly->base.indices     = ak_indexArrayAlloc(heap, poly, 4, AKT_UBYTE);
  ASSERT(poly->base.indices != NULL);

  poly->base.indices->max = 3;
  items = poly->base.indices->items;
  items[0] = 0;
  items[1] = 1;
  items[2] = 2;
  items[3] = 3;

  ASSERT(ak_meshTriangulatePoly(poly) == 2);
  ASSERT(poly->base.type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(poly->base.nPolygons == 2);
  ASSERT(poly->base.indexAccessor == NULL);
  ASSERT(poly->base.indices != NULL);
  ASSERT(poly->base.indices->count == 6);
  ASSERT(ak_meshPrimitiveIndexComponentType(&poly->base) == AKT_UBYTE);
  ASSERT(((AkTriangles *)poly)->mode == AK_TRIANGLES);
  ASSERT(ak_indexArrayGet(poly->base.indices, 0) == 0);
  ASSERT(ak_indexArrayGet(poly->base.indices, 1) == 1);
  ASSERT(ak_indexArrayGet(poly->base.indices, 2) == 2);
  ASSERT(ak_indexArrayGet(poly->base.indices, 3) == 0);
  ASSERT(ak_indexArrayGet(poly->base.indices, 4) == 2);
  ASSERT(ak_indexArrayGet(poly->base.indices, 5) == 3);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}
