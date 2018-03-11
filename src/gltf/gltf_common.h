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
#include "../ak_json.h"
#include "../ak_id.h"
#include "../resc/ak_curl.h"
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
  FListItem    *buffers;
  RBTree       *bufferViews; /* cache bufferViews to prevent dup */
  AkGLTFVersion version;
} AkGLTFState;

/* for vectors: item count,
   for matrics: item count | matrix size 
 */
typedef enum AkGLTFType {
  AK_GLTF_SCALAR = 1,
  AK_GLTF_VEC2   = 2,
  AK_GLTF_VEC3   = 3,
  AK_GLTF_VEC4   = 4,
  AK_GLTF_MAT2   = (4  << 3) | 2,
  AK_GLTF_MAT3   = (9  << 3) | 3,
  AK_GLTF_MAT4   = (16 << 3) | 4
} AkGLTFType;

#endif /* ak_gltf_commoh_h */
