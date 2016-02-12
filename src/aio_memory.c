/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_memory.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct aio_memlist_s aio_memlist;
typedef struct aio_memnode_s aio_memnode;

#ifdef _AIO_ALIGN_SIZE
#  undef _AIO_ALIGN_SIZE
#endif

#define _AIO_ALIGN_SIZE        8
#define _AIO_MEM_NODE_SIZE     sizeof(aio_memnode)

#define _AIO_ALIGNED_NODE_SIZE ((_AIO_MEM_NODE_SIZE + _AIO_ALIGN_SIZE - 1)    \
            &~ (uintptr_t)(_AIO_ALIGN_SIZE - 1))

#define _AIO_ALIGN_OF(p) ((aio_memnode *)(((char *)p)-_AIO_ALIGNED_NODE_SIZE))
#define _AIO_ALIGN_AS(m)        ((void *)(((char *)m)+_AIO_ALIGNED_NODE_SIZE))

static aio_memlist * _AIO_MEM_LST = NULL;
static int           _AIO_MEM_INITIALIZED = 0;

struct aio_memlist_s {
  size_t        ml_nodeCount;
  size_t        ml_memUsage;
  aio_memnode * ml_back;
};

struct aio_memnode_s {
  aio_memnode * mn_prev;
  aio_memnode * mn_next;
  aio_memnode * mn_chld;
  size_t        mn_size;
  unsigned long mn_refCount;

  char data[];
};

void
aio_init() {
  aio_memnode * tmpNode;

  if (_AIO_MEM_INITIALIZED == 1)
    return;

  tmpNode = calloc(sizeof(*tmpNode), 1);

  _AIO_MEM_LST = calloc(sizeof(*_AIO_MEM_LST), 1);
  _AIO_MEM_LST->ml_back = tmpNode;
  _AIO_MEM_INITIALIZED = 1;
}

static
void
aio_memlst_append(aio_memnode * __restrict mem_node) {
  assert(mem_node && "mem_node connot be NULL");

  mem_node->mn_prev = _AIO_MEM_LST->ml_back;
  _AIO_MEM_LST->ml_back->mn_next = mem_node;
  _AIO_MEM_LST->ml_back = mem_node;

  _AIO_MEM_LST->ml_nodeCount += 1;
}

static
void
aio_memlst_rm(aio_memnode * __restrict mem_node) {
  aio_memnode * prev_node;

  assert(mem_node && "mem_node connot be NULL");

  prev_node = mem_node->mn_prev;

  if (prev_node)
    prev_node->mn_next = mem_node->mn_next;

  if (_AIO_MEM_LST->ml_back == mem_node)
    _AIO_MEM_LST->ml_back = prev_node;

  _AIO_MEM_LST->ml_nodeCount -= 1;
}

void *
aio_malloc(size_t size) {
  aio_memnode * mn;

  if (_AIO_MEM_INITIALIZED == 0)
    aio_init();

  mn = malloc(_AIO_MEM_NODE_SIZE + size);

  mn->mn_refCount = 1;
  mn->mn_size     = size;
  mn->mn_prev     = NULL;
  mn->mn_chld     = NULL;

  aio_memlst_append(mn);

  return _AIO_ALIGN_AS(mn);
}

void *
aio_calloc(size_t size, size_t count) {
  aio_memnode * mn;
  size_t        memsize;

  if (_AIO_MEM_INITIALIZED == 0)
    aio_init();

  memsize = size * count;
  mn = malloc(_AIO_MEM_NODE_SIZE + memsize);

  memset(_AIO_ALIGN_AS(mn), '\0', memsize);

  mn->mn_refCount = 1;
  mn->mn_size     = memsize;
  mn->mn_prev     = NULL;
  mn->mn_next     = NULL;
  mn->mn_chld     = NULL;

  aio_memlst_append(mn);

  return _AIO_ALIGN_AS(mn);
}

void *
aio_realloc(void * __restrict mem, size_t size) {
  aio_memnode * old_mn;
  aio_memnode * new_mn;

  if (_AIO_MEM_INITIALIZED == 0)
    aio_init();

  if (mem == NULL)
    return aio_malloc(size);

  if (size == 0) {
    aio_free(mem);
    return NULL;
  }

  old_mn = _AIO_ALIGN_OF(mem);
  new_mn = realloc(old_mn, _AIO_MEM_NODE_SIZE + size);
  new_mn->mn_size = size;

  return _AIO_ALIGN_AS(new_mn);
}

char *
aio_strdup(const char * __restrict s) {
  aio_memnode * mn;
  void        * mem;
  size_t        size;
  size_t        len;
  size_t        null_size;

  if (_AIO_MEM_INITIALIZED == 0)
    aio_init();

  null_size = sizeof(char);
  len = strlen(s) + null_size; // +1 for NULL
  size = len * sizeof(*s);
  mn = malloc(_AIO_MEM_NODE_SIZE + size);

  mn->mn_refCount = 1;
  mn->mn_size     = size;
  mn->mn_prev     = NULL;
  mn->mn_next     = NULL;
  mn->mn_chld     = NULL;

  mem = _AIO_ALIGN_AS(mn);
  memcpy(mem, s, size - null_size);

  // NULL
  memset(((char *)mem) + size - null_size, '\0', 1);

  aio_memlst_append(mn);

  return mem;
}

void
aio_free(void * __restrict mem) {
  aio_memnode * mn;

  if (_AIO_MEM_INITIALIZED == 0)
    aio_init();

  mn = _AIO_ALIGN_OF(mem);
  aio_memlst_rm(mn);

  if (mn)
    free(mn);
}

void
aio_cleanup() {
  aio_memnode * mem_node;

  if (_AIO_MEM_INITIALIZED == 0)
    return;

  mem_node = _AIO_MEM_LST->ml_back;

  while (mem_node) {
    aio_memnode * tmp;
    tmp = mem_node->mn_prev;
    free(mem_node);
    mem_node = tmp;
  }

  free(_AIO_MEM_LST);
}
