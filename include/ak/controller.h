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
  uint32_t     *pCount;   /* joints count by index     */
  size_t       *pIndex;   /* offset of weight by index */
  AkBoneWeight *weights;
  AkTree       *extra;
  size_t        nWeights; /* cache: count of weights                    */
  size_t        nVertex;  /* cache: count of pJointsCount/pWeightsIndex */
} AkBoneWeights;

typedef struct AkSkin {
  struct AkNode **joints;  /* global joints, check instanceController   */
  AkBoneWeights **weights; /* per primitive                             */
  AkSource       *source;
  AkFloat4x4     *invBindMatrices;
  AkTree         *extra;
  void           *reserved[5];
  uint32_t        reserved2;
  AkURL           baseGeom;
  uint32_t        nJoints; /* cache: joint count     */
  uint32_t        nPrims;  /* cache: primitive count */
  uint32_t        nMaxJoints;
  AkFloat4x4      bindShapeMatrix;
} AkSkin;

typedef struct AkTargets {
  AkInput *input;
  AkTree  *extra;
} AkTargets;

typedef struct AkMorph {
  const char  * baseMesh;
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
  struct AkNode   **joints;
  AkBindMaterial   *bindMaterial;
  struct FListItem *reserved;
} AkInstanceController;


#ifdef __cplusplus
}
#endif
#endif /* ak_controller_h */
