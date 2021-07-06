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

#ifndef assetkit_controller_h
#define assetkit_controller_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

struct AkNode;

typedef enum AkMorphMethod {
  AK_MORPH_METHOD_NORMALIZED = 1,
  AK_MORPH_METHOD_RELATIVE   = 2,
  AK_MORPH_METHOD_ADDITIVE   = AK_MORPH_METHOD_RELATIVE
} AkMorphMethod;

typedef struct AkBoneWeight {
  uint32_t joint;
  float    weight;
} AkBoneWeight;

typedef struct AkBoneWeights {
  uint32_t     *counts;     /* joint count per vertex                     */
  size_t       *indexes;    /* offset of weight at buffer by index        */
  AkBoneWeight *weights;
  size_t        nWeights;   /* cache: count of weights                    */
  size_t        nVertex;    /* cache: count of pJointsCount/pWeightsIndex */
} AkBoneWeights;

typedef struct AkSkin {
  AkOneWayIterBase base;
  AkFloat4x4      *invBindPoses;
  struct AkNode  **joints;  /* default joints                             */
  AkBoneWeights  **weights; /* per primitive                              */
  size_t           nJoints; /* cache: joint count                         */
  uint32_t         nPrims;  /* cache: primitive count                     */
  uint32_t         nMaxJoints;
  AkFloat4x4       bindShapeMatrix;
} AkSkin;

typedef struct AkMorphTarget {
  struct AkMorphTarget *next;
  AkInput              *input;  /* per-target inputs to morph */
  float                 weight; /* per-target default weight  */
  uint32_t              inputCount;
} AkMorphTarget;

typedef struct AkMorph {
  AkOneWayIterBase base;
  AkMorphTarget   *target;
  AkMorphMethod    method;
  uint32_t         targetCount;
  uint32_t         maxInputCount;
  /* uint32_t         maxIterleavedStride; */
} AkMorph;

typedef struct AkInstanceMorph {
  AkMorph      *morph;
  AkFloatArray *overrideWeights;  /* override default weights or NULL */
} AkInstanceMorph;

typedef struct AkInstanceSkin {
  AkSkin         *skin;
  struct AkNode **overrideJoints; /* override default joints or NULL  */
} AkInstanceSkin;

/*!
 * @brief fill a buffer with JointID and JointWeight to feed GPU buffer
 *        you can send this buffer to GPU buffer (e.g. OpenGL) directly
 *
 *        this func makes things easier if you want to send buffer to GPU like:
 *          | JointIDs (ivec4) | Weights(vec4) |
 *
 *        or:
 *           in ivec4 JOINTS;
 *           in vec4  WEIGHTS;
 *
 *        AkBoneWeights provides a struct JointID|JointWeight, if that is enough
 *        for you then you do not need to use this func.
 *
 * @param source    source weights buffer
 * @param maxJoint  max joint count, 4 is ideal
 * @param itemCount component count per VERTEX attribute
 * @param buff      destination buffer to send GPU
 */
AK_EXPORT
size_t
ak_skinInterleave(AkBoneWeights * __restrict source,
                  uint32_t                   maxJoint,
                  uint32_t                   itemCount,
                  void         ** __restrict buff);

/*!
* @brief inspect a morph to get bufferSize and bufferStride to alloc memory for
*        interleaved morph buffer with desired inputs,
*        (other inputs will be ignored)
*
* @param[out] bufferSize         buffer size in bytes
* @param[out] byteStride         target byte stride in bytes
* @param[in]  morph              AkMorph object
* @param[in]  desiredInputs      desired inputs (other inputs will be ignored)
* @param[in]  desiredInputsCount desired inputs count
*/
AK_EXPORT
void
ak_morphInterleaveInspect(size_t  * __restrict bufferSize,
                          size_t  * __restrict byteStride,
                          AkMorph * __restrict morph,
                          AkInputSemantic      desiredInputs[],
                          uint8_t              desiredInputsCount);

/*!
* @brief interleave morph object with desired inputs with desired input orders.
*
*        Make sure that you called ak_morphInterleaveInspect() to get buffSize
*        and alloc a buffer with that size.
*
*        All inputs except desired inputs will be ignored. If morph object don't
*        contain a desired input than it will be ignored too.
*
*        You can send this buffer to GPU and use directly.
*
* @param[out] buff               pre-allocated buffer
* @param[in]  morph              AkMorph object
* @param[in]  desiredInputs      desired inputs (other inputs will be ignored)
* @param[in]  desiredInputsCount desired inputs count
*/
AK_EXPORT
void
ak_morphInterleave(void    * __restrict buff,
                   AkMorph * __restrict morph,
                   AkInputSemantic      desiredInputs[],
                   uint32_t             desiredInputsCount);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_controller_h */
