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

struct AkNode;

typedef enum AkControllerType {
  AK_CONTROLLER_MORPH = 1,
  AK_CONTROLLER_SKIN  = 2
} AkControllerType;

typedef enum AkMorphMethod {
  AK_MORPH_METHOD_NORMALIZED = 1,
  AK_MORPH_METHOD_RELATIVE   = 2
} AkMorphMethod;

typedef struct AkBoneWeight {
  uint32_t joint;
  float    weight;
} AkBoneWeight;

typedef struct AkBoneWeights {
  uint32_t     *counts;    /* joint count per vertex                     */
  size_t       *indexes;   /* offset of weight at buffer by index        */
  AkBoneWeight *weights;
  AkTree       *extra;
  size_t        nWeights;  /* cache: count of weights                    */
  size_t        nVertex;   /* cache: count of pJointsCount/pWeightsIndex */
} AkBoneWeights;

typedef struct AkSkin {
  struct AkNode **joints;  /* global joints, check instanceController    */
  AkFloat4x4     *invBindPoses;
  AkBoneWeights **weights; /* per primitive                              */
  AkSource       *source;
  AkTree         *extra;
  void           *reserved[5];
  uint32_t        reserved2;
  AkURL           baseGeom;
  size_t          nJoints; /* cache: joint count                        */
  uint32_t        nPrims;  /* cache: primitive count                    */
  uint32_t        nMaxJoints;
  AkFloat4x4      bindShapeMatrix;
} AkSkin;

typedef struct AkTargets {
  AkInput *input;
  AkTree  *extra;
} AkTargets;

typedef struct AkMorph {
  AkURL         baseGeom;
  AkMorphMethod method;

  AkSource    * source;
  AkTargets   * targets;
  AkTree      * extra;
} AkMorph;

typedef struct AkController {
  /* const char * id; */
  const char          *name;
  AkObject            *data;
  AkTree              *extra;
  struct AkController *next;
} AkController;

typedef struct AkInstanceController {
  AkInstanceBase    base;
  AkURL             geometry;
  struct AkNode   **joints;
  AkBindMaterial   *bindMaterial;
  struct FListItem *reserved;
} AkInstanceController;

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
 *        AkBoneWeights provides a struct JointID|HointWeight, if that is enough
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
AkGeometry*
ak_baseGeometry(AkController * __restrict ctlr);

AK_EXPORT
AkGeometry*
ak_skinBaseGeometry(AkSkin * __restrict skin);

AK_EXPORT
AkGeometry*
ak_morphBaseGeometry(AkMorph * __restrict morph);

#ifdef __cplusplus
}
#endif
#endif /* ak_controller_h */
