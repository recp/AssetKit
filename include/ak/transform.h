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

#ifndef assetkit_transform_h
#define assetkit_transform_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef struct AkTransform {
  AkObject *base; /* fixup transform */
  AkObject *item;
} AkTransform;

typedef struct AkLookAt {
  /* const char * sid; */

  AkFloat3     val[3];
} AkLookAt;

typedef struct AkMatrix {
  /* const char * sid; */

  AK_ALIGN(16) AkFloat val[4][4];
} AkMatrix;

typedef struct AkRotate {
  /* const char * sid; */
  AK_ALIGN(16) AkFloat val[4];
} AkRotate;

typedef struct AkScale {
  /* const char * sid; */
  AkFloat      val[3];
} AkScale;

typedef struct AkSkew {
  /* const char * sid; */
  AkFloat      angle;
  AkFloat3     rotateAxis;
  AkFloat3     aroundAxis;
} AkSkew;

typedef struct AkTranslate {
  /* const char * sid; */
  AkFloat      val[3];
} AkTranslate;

typedef struct AkQuaternion {
  AK_ALIGN(16) AkFloat val[4];
} AkQuaternion;

/*!
 * @brief build skew matrix from AkSkew
 *
 * @param skew   skew element
 * @param matrix skew matrix (must be aligned 16)
 */
AK_EXPORT
void
ak_transformSkewMatrix(AkSkew * __restrict skew,
                       float  * matrix);

/*!
 * @brief combines all node's transform elements
 *
 * if there is no transform then this returns identity matrix
 *
 * @param node   node
 * @param matrix combined transform (must be aligned 16)
 */
AK_EXPORT
void
ak_transformCombine(AkNode * __restrict node,
                    float  * matrix);

/*!
 * @brief combines all node's transform elements in **world coord sys**
 *
 * this func walks/traverses to top node to get world coord
 * this is not cheap op, obviously.
 *
 * @param node   node
 * @param matrix combined transform (must be aligned 16)
 */
AK_EXPORT
void
ak_transformCombineWorld(AkNode * __restrict node,
                         float  * matrix);

/*!
 * @brief duplicate all transforms of node1 to node2
 * 
 * @warning duplicated transform will alloc extra memory
 */
AK_EXPORT
void
ak_transformDup(AkNode * __restrict srcNode,
                AkNode * __restrict destNode);

AK_EXPORT
void
ak_transformFreeBase(AkTransform * __restrict transform);

AK_EXPORT
AkObject*
ak_getTransformTRS(AkNode *node, AkTypeId transformType);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_transform_h */
