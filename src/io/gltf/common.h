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

#ifndef gltf_commoh_h
#define gltf_commoh_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../json.h"
#include "strpool.h"

#include <string.h>
#include <stdlib.h>

/* JSON parser */
#include <json/json.h>

typedef struct AkBufferView {
  AkBuffer   *buffer;
  const char *name;
  size_t      byteOffset;
  size_t      byteLength;
  size_t      byteStride;
} AkBufferView;

typedef struct AkGLTFState {
  AkHeap       *heap;
  AkDoc        *doc;
  json_t       *root;
  void         *tmpParent;
  FListItem    *buffers;
  RBTree       *bufferMap;
  FListItem    *bufferViews;
  RBTree       *skinBound;
  RBTree       *meshTargets;
  void         *bindata;
  size_t        bindataLen;
  bool          stop;
  bool          isbinary;
} AkGLTFState;

#define GETCHILD(INITIAL, ITEM, INDEX)                                        \
  do {                                                                        \
    int i;                                                                    \
    ITEM = (void *)INITIAL;                                                   \
    i    = INDEX;                                                             \
    if (ITEM && i > 0) {                                                      \
      while (i > 0) {                                                         \
        if (!(ITEM = (void *)ITEM->base.next)) {                              \
          i     = -1;                                                         \
          ITEM  = NULL;                                                       \
          break;  /* not foud */                                              \
        }                                                                     \
        i--;                                                                  \
      }                                                                       \
    }                                                                         \
  } while (0)

#endif /* gltf_commoh_h */
