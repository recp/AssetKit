/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "test_memory.h"
#include "../../src/ak_memory_common.h"
#include "../../src/ak_memory_lt.h"

extern AkHeapAllocator ak__allocator;

void
test_heap(void **state) {
  AkHeap *heap, *other;

  (void)state;

  heap = ak_heap_new(NULL, NULL, NULL);
  assert_true(heap->allocator == &ak__allocator);

  assert_ptr_equal(ak_heap_lt_find(heap->heapid), heap);

  other = ak_heap_new(NULL, NULL, NULL);

  ak_heap_attach(heap, other);
  assert_true(heap->chld == other);

  ak_heap_dettach(heap, other);
  assert_true(heap->chld == NULL);

  ak_heap_attach(heap, other);
  assert_true(heap->chld == other);

  ak_heap_destroy(heap);
}
