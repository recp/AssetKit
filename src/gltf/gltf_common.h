/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_commoh_h
#define gltf_commoh_h

#include "../../include/ak/assetkit.h"
#include "../common.h"
#include "../utils.h"
#include "../tree.h"
#include "../json.h"
#include "gltf_strpool.h"

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
  RBTree       *meshTargets;
  void         *bindata;
  size_t        bindataLen;
  bool          stop;
  bool          isbinary;
} AkGLTFState;

#define I2P (void *)(intptr_t)

#define GETCHILD(INITIAL, ITEM, INDEX)                                        \
  do {                                                                        \
    int i;                                                                    \
    ITEM = INITIAL;                                                           \
    i    = INDEX;                                                             \
    if (ITEM && i > 0) {                                                      \
      while (i > 0) {                                                         \
        if (!(ITEM = ITEM->next)) {                                           \
          i     = -1;                                                         \
          ITEM  = NULL;                                                       \
          break;  /* not foud */                                              \
        }                                                                     \
        i--;                                                                  \
      }                                                                       \
    }                                                                         \
  } while (0)

#endif /* gltf_commoh_h */
