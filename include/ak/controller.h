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
 * Per-vertex bone data can arrive in two canonical layouts:
 *   - variable-count CSR layout in `weights[]`, preserving authored
 *     primitive order.
 *   - raw fixed-width joint/weight accessors in `prim->input` when the
 *     source data is already GPU-ready.
 *
 * Bridges should call ak_skinFillWeights() rather than touching `weights[]`
 * or `prim->input` directly. The helper hides this format divergence and
 * yields fixed-N (typically 4) flat buffers ready for upload.
 *
 * Default joints (`joints[]`) are populated when the skin has one stable
 * bind skeleton. Formats that allow per-instance skeleton binding may leave
 * it NULL and resolve via AkInstanceSkin.overrideJoints.
 *
 * `skeleton` is the closest common ancestor of joints, used by Apple
 * SCNSkinner.skeleton (and similar engines) as the coordinate-space
 * reference for bone lookups. Optional — when NULL, callers fall back
 * to joints[0] (typically the root joint by convention).
 */
typedef struct AkSkin {
  struct AkSkin   *next;
  AkFloat4x4      *invBindPoses;
  struct AkNode  **joints;   /* default bind joints, if format-stable      */
  AkBoneWeights  **weights;  /* optional per-primitive CSR weights         */
  struct AkNode   *skeleton; /* common ancestor; NULL if not authored      */
  void            *inspectResult; /* private cache for raw accessor pairs   */
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
 * @brief input/attribute layout in shader or interleaved buffers.
 *
 * Two layouts are supported:
 *   P1 P2 P3    N1 N2 N3    T01 T02 T03 ...
 *   P1 N1 T01   P2 N2 T02   P3  N3  T03 ... (natural layout)
 *
 * In natural layout, input order follows the target data and may differ from
 * the base shape. Use AK_MORPH_P1P2N1N2 when matching base-shape grouping is
 * required.
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

  /* Per-primitive slice inside a target view. Multi-primitive meshes can
     have different vertex counts and strides per primitive. */
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
  struct AkMorph     *next;
  AkMorphTarget      *target;
  AkMorphInspectView *inspectResult;
  AkFloatArray       *defaultWeights; /* this overrides mesh.weights        */
  const char        **targetNames;    /* optional, length = targetCount     */
  AkMorphPreset      *presets;        /* optional named weight sets         */
  AkMorphMethod       method;
  uint32_t            targetCount;
  uint32_t            presetCount;
} AkMorph;

typedef bool
(*AkMorphProgressFn)(AkMorph * __restrict morph,
                     uint32_t             targetIndex,
                     uint32_t             targetCount,
                     void   * __restrict userdata);

typedef struct AkInstanceMorph {
  AkMorph      *morph;
  AkFloatArray *overrideWeights;  /* override morph.weights and mesh.weight or NULL */
} AkInstanceMorph;

typedef struct AkInstanceSkin {
  AkSkin         *skin;
  struct AkNode **overrideJoints; /* override default joints or NULL  */
  struct AkNode  *overrideSkeleton; /* per-instance skeleton root or NULL */
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
 *        agnostic core (CSR + accessor-backed inputs), same top-N selection
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
 * @param skin      AkSkin
 * @param prim      mesh primitive being skinned (vertex order source)
 * @param primIdx   primitive index fallback; prim pointer is authoritative
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
 *          - CSR primitives use skin->weights[]; variable joint count per
 *            vertex -> top-N selected by weight, zero-padded if count<N,
 *            normalized so weights sum to 1.
 *          - accessor-backed primitives keep joint/weight sets in
 *            prim->input; all sets are merged, top-N is selected, and
 *            UBYTE/USHORT joint indices are written as uint16_t.
 *
 *        Bridges should call this rather than reading skin->weights[] or
 *        prim->input directly — the dual storage is an implementation
 *        detail of the parsers.
 *
 * @param[in]  skin        skin owning the bone data
 * @param[in]  prim        mesh primitive being skinned
 * @param[in]  primIdx     primitive index fallback; prim pointer is authoritative
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
 * @brief collect vertex indices affected by one joint for one primitive.
 *
 *        Works for both CSR skin weights and accessor-backed joint/weight
 *        inputs. The function returns the total matching vertex count even
 *        when `outVertices` is NULL or `capacity` is smaller than the result,
 *        so callers can first query size and then fill a buffer.
 *
 * @param[in]  skin         skin owning the bone data
 * @param[in]  prim         mesh primitive at index `primIdx`
 * @param[in]  primIdx      primitive index in mesh
 * @param[in]  jointIdx     joint index to scan for
 * @param[out] outVertices  optional vertex-index buffer
 * @param[in]  capacity     number of uint32_t slots in outVertices
 * @return     total number of affected vertices.
 */
AK_EXPORT
size_t
ak_skinVerticesForJoint(AkSkin          * __restrict skin,
                        AkMeshPrimitive * __restrict prim,
                        uint32_t                     primIdx,
                        uint32_t                     jointIdx,
                        uint32_t        * __restrict outVertices,
                        size_t                       capacity);

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
 *        Inputs that do not exist in the base mesh are ignored. Use
 *        ak_morphInspect() when a custom interleave layout must preserve every
 *        authored target input.
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

AK_EXPORT
AkResult
ak_morphInterleaveWithProgress(AkGeometry         * __restrict baseMesh,
                               AkMorph            * __restrict morph,
                               AkMorphInterleaveLayout         layout,
                               void               * __restrict destBuff,
                               AkMorphProgressFn                progress,
                               void               * __restrict userdata);

AK_EXPORT
const AkMorphPreset*
ak_morphPresetByName(AkMorph    * __restrict morph,
                     const char * __restrict name);

AK_EXPORT
bool
ak_morphApplyPreset(AkMorph    * __restrict morph,
                    const char * __restrict presetName,
                    float      * __restrict outWeights,
                    uint32_t                capacity);

AK_INLINE
bool
ak_morphHasOverride(const AkInstanceMorph* inst) {
  return inst && inst->overrideWeights && inst->overrideWeights->count > 0;
}

#ifdef __cplusplus
}
#endif
#endif /* assetkit_controller_h */
