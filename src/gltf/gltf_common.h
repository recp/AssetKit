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
#include "../memory_common.h"
#include "../utils.h"
#include "../tree.h"
#include "../json.h"
#include "../id.h"
#include "../resc/curl.h"
#include "gltf_strpool.h"

#include <string.h>
#include <stdlib.h>

/* JSON parser */
#include <json/json.h>

typedef enum AkGLTFVersion {
  AK_GLTF_VERSION_10 = 1,
  AK_GLTF_VERSION_20 = 2
} AkGLTFVersion;

typedef struct AkGLTFState {
  AkHeap       *heap;
  AkDoc        *doc;
  json_t       *root;
  FListItem    *buffers;
  RBTree       *bufferViews; /* cache bufferViews to prevent dup */
  RBTree       *meshTargets;
  AkGLTFVersion version;
  bool          stop;
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
