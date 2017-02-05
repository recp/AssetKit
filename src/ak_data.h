/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_data_h
#define ak_data_h

#include "ak_common.h"

struct AkDataContext;

typedef void (*AkDataContextWalkFn)(struct AkDataContext *dctx,
                                    void                 *item);

typedef struct AkDataChunk {
  struct AkDataChunk *next;
  size_t              usedsize;
  char                data[];
} AkDataChunk;

typedef struct AkDataContext {
  void                *memparent;
  AkHeap              *heap;
  AkDataChunk         *data;
  AkDataChunk         *last;
  AkCmpFn              cmp;
  AkDataContextWalkFn  walkFn;
  size_t               nodesize;
  size_t               size;
  size_t               usedsize;
  size_t               itemsize;
  size_t               itemcount;
} AkDataContext;

AkDataContext*
ak_data_new(void   *memparent,
            size_t  nodeitems,
            size_t  itemsize,
            AkCmpFn cmp);

/*!
 * @brief join batch data into continued array
 *
 * @param dctx data context
 * @param buff buffer
 *
 * @return array count
 */
size_t
ak_data_join(AkDataContext *dctx,
             void          *buff);

void
ak_data_append(AkDataContext *dctx, void *data);

bool
ak_data_append_unq(AkDataContext *dctx, void *item);

void
ak_data_walk(AkDataContext *dctx);

bool
ak_data_exists(AkDataContext *dctx, void *item);

#endif /* ak_data_h */
