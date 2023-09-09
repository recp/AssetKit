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
struct FListItem;

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

typedef enum AkMorphableType {
  AK_MORPHABLE_GEOMETRY, /* per-target geometry, must be same as base  */
  AK_MORPHABLE_MORPHABLE /* morph inputs if no geometry object is used */
} AkMorphableType;

/**
 * @brief input/attribute layout in shader orr in interleaved buffer
 * 
 *   currently two layouts are supported: 
 *     ------------------------------------------------------------------------
 *     P1 P2 P3    N1 N2 N3    T01 T02 T03 ...
 *     P1 N1 T01   P2 N2 T02   P3  N3  T03 ... (natural layout)
 * 
 *  IMPORTANT: in natural layout, input orders may not same as baseShape
 *             if you need same order as baseShape, use P1P2N1N2 layout
 *             or create an issue to bring this feature to here which is in TODO.
 */
typedef enum AkMorphInterleaveLayout {
  AK_MORPH_UNKNOWN  = 0,
  AK_MORPH_P1P2N1N2 = 1, /* each target's inputs are groupped by input type */
  AK_MORPH_NATURAL  = 3  /* P1N1 P2N2 but input orders are natural as target */
} AkMorphInterleaveLayout;

/* per-target inputs to morph */
typedef struct AkMorphable {
  struct AkMorphable  *next;
  AkInput             *input;  
  uint32_t             inputCount;
} AkMorphable;

typedef struct AkMorphTarget {
  struct AkMorphTarget *next;
  AkObject             *target;      /* AkGeometry or AkMorphable to morph */
  uint32_t              targetCount; /* number of mesh primitives to moprh */
  float                 weight;      /* per-target default weight          */
} AkMorphTarget;

typedef struct AkMorphInspectTargetInput {
  struct AkMorphInspectTargetInput *next;
  AkInput                          *input;
  uint32_t                          intrOffset;
  union {
    bool                            inBaseMesh;
    bool                            inTarget;
  };
} AkMorphInspectTargetInput;

typedef struct AkMorphInspectTargetView {
  struct AkMorphInspectTargetView *next;
  AkMorphInspectTargetInput       *input;
  AkMorphInspectTargetInput       *lastInput;
  uint32_t                         inputsCount;
  float                            weight;
} AkMorphInspectTargetView;

/* helper struct to show */
typedef struct AkMorphInspectView {
  /* first one is baseShape if includeBaseShape param is set to 'true' */
  AkMorphInspectTargetView *base;
  AkMorphInspectTargetView *targets;
  uint32_t                  nTargets;
  size_t                    interleaveBufferSize;
  size_t                    interleaveByteStride;
  uint32_t                  accessorAccessCount;
  bool                      includeBaseShape;
  bool                      ignoreUncommonInputs;
  AkMorphInterleaveLayout   layout;
} AkMorphInspectView;

typedef struct AkMorph {
  AkOneWayIterBase    base;
  AkMorphTarget      *target;
  AkMorphInspectView *inspectResult;
  AkMorphMethod       method;
  uint32_t            targetCount;
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
 *        you can send this buffer to GPU buffer (e.g. OpenGL) as interleaved 
 *        single buffer.
 *
 *        this func makes things easier if you want to send data in single 
 *        buffer to GPU like:
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
 *        interleaved morph buffer with desired inputs. Also returns a list of 
 *        inputs for each target. You can use this list to collect inputs from
 *        morph targets. 
 *
 *        inspected result will be stored in morph->inspectResult. You can use
 *        this result to collect inputs same order as baseShape's inputs' order
 *        from morph targets. Inputs that dont exists in baseShape will be ignored.
 *        If you need them, pass ignoreUncommonInputs = false.
 * 
 * @param[in]  baseMesh               base mesh to morph
 * @param[in]  morph                  AkMorph object
 * @param[in]  desiredInputs          desired inputs (other inputs will be ignored)
 *                                        or NULL to collect all inputs, desiredInputsCount must be 0 in this case
 * @param[in]  desiredInputsCount     desired inputs count or 0 to collect all inputs
 * @param[in]  includeBaseShape       if true, baseShape will be included in result e.g. bytes stride, buffer size etc.
 * @param[in]  ignoreUncommonInputs   if true, all inputs that dont exist in base mesh will be ignored
 */
AK_EXPORT
AkResult
ak_morphInspect(AkGeometry * __restrict baseMesh,
                AkMorph    * __restrict morph,
                AkInputSemantic         desiredInputs[],
                uint8_t                 desiredInputsCount,
                bool                    includeBaseShape,
                bool                    ignoreUncommonInputs);

/*!
  * @brief prepare morph inspect result to interleave morph object with desired inputs
  *        this prepares inputs order by specified layout parameter and sets intrOffset, 
  *        inBaseMesh etc. properties.
  *
  *        make sure that you called ak_morphInspect() to get buffSize
  *        and alloc a buffer with that size.
  *
  * @param[in]  inspectView  inspect result
  * @param[in]  layout       interleave layout e.g. p1p2n1n2 or p1n1p2n2
  */
AK_EXPORT
AkResult
ak_morphInspectPrepareLayout(AkMorphInspectView * __restrict inspectView, 
                             AkMorphInterleaveLayout         layout);

/*!
 * @brief interleave morph object with desired inputs with desired input orders.
 *
 *        Make sure that you called ak_morphInspect() to get buffSize
 *        and alloc a buffer with that size.
 *
 *        All inputs except desired inputs will be ignored. If morph object don't
 *        contain a desired input than it will be ignored too.
 *
 *        You can send this buffer to GPU and use directly.
 * 
 *        WARN: all inputs that dont exist in base mesh will be ignored
 *              if you need them, you can use ak_morphInspect() to get all 
 *              inputs for your own interleave() implementation. Create an issue 
 *              if you need bring this feature to here.
 *
 * @param[in]  baseMesh      base mesh to morph
 * @param[in]  morph         AkMorph object
 * @param[in]  layout        interleave layout e.g. p1p2n1n2 or p1n1p2n2
 * @param[out] destBuff      pre-allocated buffer to store interleaved data
 */
AK_EXPORT
AkResult
ak_morphInterleave(AkGeometry * __restrict baseMesh,
                   AkMorph    * __restrict morph, 
                   AkMorphInterleaveLayout layout,
                   void       * __restrict destBuff);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_controller_h */
