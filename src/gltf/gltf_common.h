/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_gltf_commoh_h
#define ak_gltf_commoh_h

#include "../../include/assetkit.h"
#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "../ak_utils.h"
#include "../ak_tree.h"
#include "gltf_strpool.h"

#include <string.h>
#include <stdlib.h>

/* JSON parser */
#include <jansson.h>

typedef enum AkGLTFVersion {
  AK_GLTF_VERSION_10 = 1,
  AK_GLTF_VERSION_20 = 2
} AkGLTFVersion;

typedef struct AkGLTFState {
  AkHeap       *heap;
  AkDoc        *doc;
  json_t       *root;
  AkGLTFVersion version;
} AkGLTFState;

#endif /* ak_gltf_commoh_h */
