/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_controller_h
#define ak_controller_h
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
  uint32_t     *counts;    /* joint count per vertex                     */
  size_t       *indexes;   /* offset of weight at buffer by index        */
  AkBoneWeight *weights;
  size_t        nWeights;  /* cache: count of weights                    */
  size_t        nVertex;   /* cache: count of pJointsCount/pWeightsIndex */
  
  /* TODO: Will be removed */
  AkTree       *extra;
} AkBoneWeights;

typedef struct AkSkin {
  AkOneWayIterBase base;
  struct AkNode  **joints;  /* global joints, check instanceController    */
  AkFloat4x4      *invBindPoses;
  AkBoneWeights  **weights; /* per primitive                              */
  size_t           nJoints; /* cache: joint count                         */
  uint32_t         nPrims;  /* cache: primitive count                     */
  uint32_t         nMaxJoints;
  AkFloat4x4       bindShapeMatrix;
} AkSkin;

typedef struct AkMorphTarget {
  struct AkMorphTarget *next;
  AkMeshPrimitive      *prim;   /* mesh primitive             */
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
  AkFloatArray *overrideWeights; /* override default weights or NULL */
} AkInstanceMorph;

typedef struct AkInstanceSkin {
  AkSkin         *skin;
  struct AkNode **overrideJoints;
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
ak_skinFill(AkBoneWeights * __restrict source,
            uint32_t                   maxJoint,
            uint32_t                   itemCount,
            void         ** __restrict buff);

AK_EXPORT
void
ak_morphInterleaveInspect(size_t  * __restrict bufferSize,
                          size_t  * __restrict byteStride,
                          AkMorph * __restrict morph,
                          AkInputSemantic      desiredInputs[],
                          uint8_t              desiredInputsCount);

AK_EXPORT
void
ak_morphInterleave(void    * __restrict buff,
                   AkMorph * __restrict morph,
                   AkInputSemantic      desiredInputs[],
                   uint32_t             desiredInputsCount);

#ifdef __cplusplus
}
#endif
#endif /* ak_controller_h */
