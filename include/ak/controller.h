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
#include "geom.h"

struct AkNode;
struct FListItem;

typedef enum AkMorphMethod {
  /* Weights of blend shapes normalized to 1 (or 100%) */
  AK_MORPH_METHOD_NORMALIZED = 1,  

  /* Blend shapes defined as a difference from the base shape */
  AK_MORPH_METHOD_RELATIVE   = 2,  

  /* Alias for RELATIVE, treat additive as relative in this context */
  AK_MORPH_METHOD_ADDITIVE   = AK_MORPH_METHOD_RELATIVE,

  /* Each blend shape applied fully on top of the previous one */
  AK_MORPH_METHOD_ABSOLUTE   = 3
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

/**
 * Skin controller — vertex skinning data.
 *
 * Per-vertex bone data storage differs by source format:
 *   - DAE: variable-count CSR layout in `weights[primIdx]` (filled by
 *     dae_fixup_ctlr).
 *   - glTF: raw vec4 JOINTS_n / WEIGHTS_n accessors stay in `prim->input`
 *     (no aggregation — the format is already GPU-ready).
 *
 * Bridges should call ak_skinFillWeights() rather than touching `weights[]`
 * or `prim->input` directly. The helper hides this format divergence and
 * yields fixed-N (typically 4) flat buffers ready for upload.
 *
 * Default joints (`joints[]`) are populated for glTF; DAE leaves it NULL
 * and resolves joints per-instance via AkInstanceSkin.overrideJoints (DAE
 * lets the same skin bind to different skeletons per instance).
 *
 * `skeleton` is the closest common ancestor of joints, used by Apple
 * SCNSkinner.skeleton (and similar engines) as the coordinate-space
 * reference for bone lookups. Optional — when NULL, callers fall back
 * to joints[0] (typically the root joint by convention). glTF: filled
 * from the optional `skin.skeleton` JSON hint. DAE: filled from
 * <instance_controller>'s first <skeleton> URL when fixing the
 * instance.
 */
typedef struct AkSkin {
  AkOneWayIterBase base;
  AkFloat4x4      *invBindPoses;
  struct AkNode  **joints;   /* default joints (glTF; NULL for DAE)        */
  AkBoneWeights  **weights;  /* per primitive (DAE only; NULL for glTF)    */
  struct AkNode   *skeleton; /* common ancestor; NULL if not authored      */
  size_t           nJoints;  /* cache: joint count                         */
  uint32_t         nPrims;   /* cache: primitive count                     */
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

// typedef struct AkSparseMorphInfo {
//   uint32_t *affectedVertices;  /* indices of vertices that change            */
//   uint32_t  nAffectedVertices;
//   float     sparsityRatio;     /* 0.0-1.0, useful for optimization decisions */
// } AkSparseMorphInfo;

/* per-target inputs to morph */
typedef struct AkMorphable {
  struct AkMorphable  *next;
  AkInput             *input;  
  uint32_t             inputCount;
} AkMorphable;

/* TODO:
    AkMorphPreset presets[3] = {
      { "neutral", neutralWeights },
      { "smile",   smileWeights },
      { "blink",   blinkWeights }
    };
    morph.presets = presets;
    morph.presetCount = 3;
 */
typedef struct AkMorphPreset {
  const char   *name;      /* "neutral", "smile_max", ... */
  AkFloatArray *weights;   /* length = morph->targetCount */
} AkMorphPreset;

typedef struct AkMorphTarget {
  struct AkMorphTarget *next;
  AkObject             *target;         /* AkGeometry or AkMorphable to morph */
  uint32_t              primitiveCount; /* number of mesh primitives to morph */
} AkMorphTarget;

typedef struct AkMorphInspectInput {
  struct AkMorphInspectInput *next;
  AkInput                    *input;
  uint32_t                    intrOffset;
  union {
    bool                      inBaseMesh;
    bool                      inTarget;
  };
} AkMorphInspectInput;

typedef struct AkMorphInspectMorphable {
  struct AkMorphInspectMorphable *next;
  AkMorphInspectInput            *input;
  AkMorphInspectInput            *lastInput;
  uint32_t                        inputsCount;
  float                           weight;

  /* Per-primitive sizing. With multi-primitive morphs each
     primitive can have its own vertex count and stride; storing
     them here lets the interleave pass walk every morphable
     instead of treating primitive 0 as the canonical sample.
     `bufferOffset` is the byte offset of this primitive's data
     within its targetView's slice; `bufferSize`/`stridePerVertex`
     are derived from the input chain and `vertexCount`. */
  uint32_t                        vertexCount;
  uint32_t                        stridePerVertex;
  size_t                          bufferOffset;
  size_t                          bufferSize;
} AkMorphInspectMorphable;

typedef struct AkMorphInspectTargetView {
  struct AkMorphInspectTargetView *next;
  AkMorphInspectMorphable         *morphable;
  uint32_t                         nTargets;
  /* Total byte size of this target slice, summed across all
     per-primitive morphables. */
  size_t                           interleaveBufferSize;
} AkMorphInspectTargetView;

/*
  AkMorphInspectView
         o
         |
         o -> AkMorphInspectTargetView ( like Mesh )
                         o
                         | 
                         o ->  AkMorphInspectMorphable 1 ( like Mesh Primitive )
                                         o
                                         -> AkMorphInspectInput 1
                                         -> AkMorphInspectInput 2
                                         -> AkMorphInspectInput 3
                         o ->  AkMorphInspectMorphable 2
                         o ->  AkMorphInspectMorphable 3
        o -> AkMorphInspectTargetView
                         o
                         | 
                         o ->  AkMorphInspectMorphable 1
                         o ->  AkMorphInspectMorphable 2
                         o ->  AkMorphInspectMorphable 3
*/
typedef struct AkMorphInspectView {
  /* first one is baseShape if includeBaseShape param is set to 'true' */
  AkMorphInspectTargetView *base;
  AkMorphInspectTargetView *targets;
  AkFloatArray             *initialWeights;
  uint32_t                  nTargets;
  size_t                    interleaveTotalBufferSize;
  bool                      includeBaseShape;
  bool                      ignoreUncommonInputs;
  AkMorphInterleaveLayout   layout;
} AkMorphInspectView;

typedef struct AkMorph {
  AkOneWayIterBase    base;
  AkMorphTarget      *target;
  AkMorphInspectView *inspectResult;
  AkFloatArray       *defaultWeights; /* this overrides mesh.weights        */
  const char        **targetNames;    /* optional, length = targetCount     */
  AkMorphMethod       method;
  uint32_t            targetCount;
} AkMorph;

// TODO: multi-morph-per-mesh just thought loudly ?
// typedef struct AkPrimitiveMorph {
//   AkOneWayIterBase    base;
//   AkMorphTarget      *target;
//   AkMorphInspectView *inspectResult;
//   AkFloatArray       *weights; /* default weights or NULL to zero */
//   AkMorphMethod       method;
//   uint32_t            targetCount; 
// } AkPrimitiveMorph;
// 
// typedef struct AkMeshMorph {
//   AkOneWayIterBase  base;
//   AkPrimitiveMorph *morph;
//   float             weight;
// } AkMeshMorph;

typedef struct AkInstanceMorph {
  AkMorph      *morph;
  AkFloatArray *overrideWeights;  /* override morph.weights and mesh.weight or NULL */
} AkInstanceMorph;

typedef struct AkInstanceSkin {
  AkSkin         *skin;
  struct AkNode **overrideJoints; /* override default joints or NULL  */
} AkInstanceSkin;

/*!
 * @brief format-agnostic per-vertex bone-data extraction with INTERLEAVED
 *        output, suitable for a single GPU vertex buffer (OpenGL/Metal/
 *        Vulkan typical layout).
 *
 *        Per-vertex layout in the output buffer:
 *
 *          | JointIDs[maxJoint] (uint16) | Weights[maxJoint] (float) |
 *
 *        Matches a shader vertex input like:
 *
 *           in uvec4 JOINTS;  (backed by uint16 vertex input)
 *           in vec4  WEIGHTS;
 *
 *        Quality is identical to ak_skinFillWeights() — same format-
 *        agnostic core (DAE CSR + glTF accessors), same top-N selection
 *        when authored joint count exceeds maxJoint, same renormalize so
 *        weights sum to 1. Only the output packing differs (interleaved
 *        single buffer vs. separate idx/wgt arrays).
 *
 *        For the SceneKit/RealityKit path use ak_skinFillWeights()
 *        instead — Apple's APIs require separate boneIndices and
 *        boneWeights SCNGeometrySources.
 *
 *        Layout: per vertex `[J0..J(N-1) (uint16) W0..W(N-1) (float)]`.
 *
 * @param skin      AkSkin (for both DAE-CSR and glTF-accessor inputs)
 * @param prim      mesh primitive being skinned (vertex order source)
 * @param primIdx   primitive index in the mesh (DAE skin->weights[])
 * @param maxJoint  storage slots per vertex (typically 4)
 * @param buff      destination buffer; if *buff is NULL one is alloc'd
 *                  with ak_calloc(NULL, ...) and stored back into *buff
 *                  (caller frees with ak_free)
 * @return  total bytes written, or 0 on failure
 */
AK_EXPORT
size_t
ak_skinInterleave(AkSkin          * __restrict skin,
                  AkMeshPrimitive * __restrict prim,
                  uint32_t                     primIdx,
                  uint32_t                     maxJoint,
                  void           ** __restrict buff);

/*!
 * @brief format-agnostic per-vertex bone-data extraction for one primitive.
 *
 *        Fills caller-provided fixed-N flat buffers (joint indices + weights)
 *        regardless of the asset's source format:
 *
 *          - DAE  primitives use the CSR layout in skin->weights[primIdx];
 *                 variable joint count per vertex -> top-N selected by weight,
 *                 zero-padded if count<N, normalized so weights sum to 1.
 *          - glTF primitives keep JOINTS_n / WEIGHTS_n sets as raw accessors
 *                 in prim->input; all sets are merged, top-N is selected, and
 *                 UBYTE/USHORT joint indices are written as uint16_t.
 *
 *        Bridges should call this rather than reading skin->weights[] or
 *        prim->input directly — the dual storage is an implementation
 *        detail of the parsers.
 *
 * @param[in]  skin        skin owning the bone data
 * @param[in]  prim        mesh primitive at index `primIdx`
 * @param[in]  primIdx     primitive index in mesh
 * @param[in]  maxJoint    fixed slot count per vertex (typically 4)
 * @param[out] outIndices  buffer for vertexCount × maxJoint × sizeof(uint16_t)
 * @param[out] outWeights  buffer for vertexCount × maxJoint × sizeof(float)
 * @return     vertex count on success, 0 on error.
 */
AK_EXPORT
size_t
ak_skinFillWeights(AkSkin          * __restrict skin,
                   AkMeshPrimitive * __restrict prim,
                   uint32_t                     primIdx,
                   uint32_t                     maxJoint,
                   uint16_t        * __restrict outIndices,
                   float           * __restrict outWeights);

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

AK_INLINE
bool
ak_morphHasOverride(const AkInstanceMorph* inst) {
  return inst && inst->overrideWeights && inst->overrideWeights->count > 0;
}

/* TODO: CPU morph evaluator (utility, low priority)
 *
 * Computes the final blended vertex data on the CPU using base mesh + targets
 * + weights. Output is a "final deformed mesh" ready to be uploaded as a
 * single static draw — no GPU-side blending required.
 *
 * Use cases:
 *   - mesh export / bake tools (e.g., bake "smile_max" pose as a static asset)
 *   - software renderers without GPU shader blending
 *   - pre-bake / asset pipeline tooling
 *
 * Modern engines (SceneKit, RealityKit, custom Metal/Vulkan) do NOT need this:
 * they consume ak_morphInterleave output + a weights uniform array and blend
 * on the GPU. This evaluator is purely for the niche cases above.
 *
 * Sketch:
 *
 * AK_EXPORT
 * AkResult
 * ak_morphEvaluate(AkGeometry            * __restrict baseMesh,
 *                  AkMorph               * __restrict morph,
 *                  const AkInstanceMorph * __restrict inst,     // NULL → defaults
 *                  void                  * __restrict destBuff);// pre-alloc base layout
 */

//AkResult
//ak_morphEvaluateWeights(const AkMorph         * __restrict morph,
//                        const AkInstanceMorph * __restrict inst,
//                        AkFloat                ** __restrict out /* len = morph->targetCount */) {
//  AkFloatArray *ov;
//  AkFloatArray *def;
//  AkMesh       *mesh;
//  uint32_t      n;
//
//  if (!morph || !out)
//    return AK_ERR;
//
//  n   = morph->targetCount;
//  ov  = (inst) ? inst->overrideWeights : NULL;
//
//  /* 1) Kaynak seçimi ve kopyalama (override > default > zeros) */
//  if (ov && ov->count) {
//    *out = ov->items;
//    return AK_OK;
//  } else if ((def = morph->defaultWeights) && def->count) {
//    *out = def->items;
//    return AK_OK;
//  } else if ((ak_objGet(morph->target->target))) {
//
//
//    return mesh->weights;
//  }
//
//  return AK_OK;
//}

/*
typedef void (*AkMorphProgressCallback)(float progress, void *userData);

AK_EXPORT
AkResult
ak_morphInterleaveWithProgress(AkGeometry * __restrict baseMesh,
                               AkMorph    * __restrict morph,
                               AkMorphInterleaveLayout layout,
                               void       * __restrict destBuff,
                               AkMorphProgressCallback callback,
                               void       *            userData);

typedef enum AkMorphResult {
  AK_MORPH_OK = 0,
  AK_MORPH_INCOMPATIBLE_TOPOLOGY,
  AK_MORPH_MISSING_INPUT,
  AK_MORPH_INVALID_WEIGHT_COUNT,
  /* ... * /
} AkMorphResult;

AK_EXPORT
AkResult
ak_morphValidateTarget(AkGeometry    * __restrict baseMesh,
                       AkMorphTarget * __restrict target);

AK_EXPORT
bool
ak_morphIsCompatible(AkGeometry * __restrict mesh1,
                     AkGeometry * __restrict mesh2);
*/
#ifdef __cplusplus
}
#endif
#endif /* assetkit_controller_h */
