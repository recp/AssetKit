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

#ifndef assetkit_node_h
#define assetkit_node_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

struct AkInstanceMorph;

typedef enum AkNodeFlags {
  AK_NODEF_FIXED_COORD = 1
} AkNodeFlags;

typedef enum AkNodeType {
  AK_NODE_TYPE_NODE        = 1,
  AK_NODE_TYPE_CAMERA_NODE = 2,
  AK_NODE_TYPE_JOINT       = 3
} AkNodeType;

typedef struct AkListIter {
  void *prev;
  void *next;
} AkListIter;

typedef struct AkTreeWithParentIter {
  void *prev;
  void *next;
  void *parent;
  void *chld;
} AkTreeWithParentIter;

typedef struct AkTreeIter {
  void *prev;
  void *next;
  void *chld;
} AkTreeIter;

typedef struct AkNode {
  /* const char * id;  */
  /* const char * sid; */

  const char           *name;
  AkNodeFlags           flags;
  AkNodeType            nodeType;
  AkStringArray        *layer;
  struct AkTransform   *transform;

  /* only avilable if library is forced to calculate them
     check these two matrix to avoid extra or same calculation
   */
  struct AkMatrix      *matrix;
  struct AkMatrix      *matrixWorld;
  struct AkBoundingBox *bbox;
  
  AkInstanceGeometry   *geometry;
  AkInstanceBase       *camera;
  AkInstanceBase       *light;
  AkInstanceNode       *node;

  AkTree               *extra;

  struct AkNode        *prev;
  struct AkNode        *next;
  struct AkNode        *chld;
  struct AkNode        *parent;
} AkNode;

AK_EXPORT
void
ak_addSubNode(AkNode * __restrict parent,
              AkNode * __restrict subnode,
              bool                fixCoordSys);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_node_h */
