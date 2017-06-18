/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_node_h
#define ak_node_h
#ifdef __cplusplus
extern "C" {
#endif

#include "ak-common.h"

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
  AkNodeType            nodeType;
  AkStringArray        *layer;
  struct AkTransform   *transform;

  /* only avilable if library is forced to calculate them
     check these two matrix to avoid extra or same calculation
   */
  struct AkMatrix      *matrix;
  struct AkMatrix      *matrixWorld;
  struct AkBoundingBox *bbox;

  AkInstanceBase       *camera;
  AkInstanceController *controller;
  AkInstanceGeometry   *geometry;
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
#endif /* ak_node_h */
