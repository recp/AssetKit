/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "test_memory.h"
#include "../../src/ak_memory_common.h"
#include "../../src/ak_memory_lt.h"

#include <stdint.h>

extern AkHeapAllocator ak__allocator;

void
test_heap(void **state) {
  AkHeap  *heap, *other, staticHeap;
  uint32_t heapid, data;

  (void)state;

  heap = ak_heap_new(NULL, NULL, NULL);
  assert_true(heap->allocator == &ak__allocator);
  assert_true(ak_heap_allocator(heap) == &ak__allocator);

  heapid = heap->heapid;
  assert_true(heapid > 0);
  assert_ptr_equal(ak_heap_lt_find(heap->heapid), heap);

  other = ak_heap_new(NULL, NULL, NULL);

  ak_heap_attach(heap, other);
  assert_true(heap->chld == other);

  ak_heap_dettach(heap, other);
  assert_true(heap->chld == NULL);

  ak_heap_attach(heap, other);
  assert_true(heap->chld == other);

  ak_heap_setdata(heap, &data);
  assert_true(ak_heap_data(heap) == &data);

  ak_heap_destroy(heap);
  assert_true(ak_heap_lt_find(heapid) == NULL);

  ak_heap_init(&staticHeap, NULL, NULL, NULL);
  assert_true(staticHeap.heapid > 0);

  ak_heap_lt_remove(staticHeap.heapid);
  assert_true(ak_heap_lt_find(staticHeap.heapid) == NULL);
}

void
test_heap_multiple(void **state) {
  AkHeap  *heap, *root;
  uint32_t count, i;

  (void)state;

  root = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc, leak */
  for (i = 0; i < 1000; i++)
    heap = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc 2, leak */
  for (i = 0; i < 10000; i++)
    heap = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc-free 1 */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_destroy(heap);
  }

  /* multiple alloc-free 2 */
  for (i = 0; i < 10000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_destroy(heap);
  }

  /* multiple alloc, attach to parent */
  for (i = 0; i < 10000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_attach(root, heap);
  }

  /* multiple alloc, attach to parent */
  for (i = 0; i < 10000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_attach(root, heap);
  }

  ak_heap_destroy(root);

  /* multiple alloc, attach-detach to parent */
  for (i = 0; i < 10000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_attach(root, heap);
    ak_heap_dettach(root, heap);
  }
}
