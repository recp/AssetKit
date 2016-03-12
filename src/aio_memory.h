/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__memory__h_
#define __libassetio__memory__h_

#include <stddef.h>
#include <sys/types.h>

typedef struct aio_heapnode_s aio_heapnode;
typedef struct aio_heap_s     aio_heap;

#ifdef aio__align_size
#  undef aio__align_size
#endif

#define aio__align_size    8
#define aio__heapnode_size sizeof(aio_heapnode)

#define aio__aligned_node_size ((aio__heapnode_size + aio__align_size - 1)    \
            &~ (uintptr_t)(aio__align_size - 1))

#define aio__alignof(p) ((aio_heapnode *)(((char *)p)-aio__aligned_node_size))
#define aio__alignas(m) ((void *)(((char *)m)+aio__aligned_node_size))

struct aio_heapnode_s {
  aio_heapnode *prev; /* parent */
  aio_heapnode *next; /* right  */
  aio_heapnode *chld; /* left   */

  char data[];
};

struct aio_heap_s {
  aio_heapnode *root;
};

void*
aio_heap_alloc(aio_heap *heap,
               void * __restrict parent,
               size_t size);

void
aio_heap_free(aio_heapnode * __restrict heapNode);

void* aio_malloc(size_t size);
void* aio_calloc(size_t size, size_t count);
char* aio_strdup(const char * __restrict str);
void* aio_realloc(void * __restrict memptr, size_t newsize);

void  aio_free(void * __restrict memptr);
void  aio_cleanup();

#endif /* __libassetio__memory__h_ */
