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

#ifndef ak_data_h
#define ak_data_h

#include "common.h"

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
 * @param dctx     data context
 * @param buff     buffer
 * @param stride   bytes stride
 * @param buff     bytesgap gap between item stride
 *
 * @return array count
 */
size_t
ak_data_join(AkDataContext *dctx,
             void          *buff,
             size_t         stride,
             size_t         bytesgap);

/*!
 * @brief append item, this will copy data by size of itemsize into context
 *
 * @param dctx data context
 * @param item item
 *
 * @return index
 */
int
ak_data_append(AkDataContext *dctx, void *item);

/*!
 * @brief append if not exists and return appended item index
 *
 * @param dctx data context
 * @param item item
 *
 * @return index
 */
int
ak_data_append_unq(AkDataContext *dctx, void *item);

/*!
 * @brief walk through by walkFn
 *
 * @param dctx data context
 */
void
ak_data_walk(AkDataContext *dctx);

/*!
 * @brief check if item is exists
 *
 * @param dctx data context
 * @param item item
 *
 * @return returns item index
 */
int
ak_data_exists(AkDataContext *dctx, void *item);

#endif /* ak_data_h */
