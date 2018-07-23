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

typedef enum AkMorphMethod {
  AK_MORPH_METHOD_NORMALIZED = 1,
  AK_MORPH_METHOD_RELATIVE   = 2
} AkMorphMethod;

typedef struct AkJoints {
  AkInput *input;
  AkTree  *extra;
} AkJoints;

typedef struct AkVertexWeights {
  AkInput       * input;
  AkUIntArray   * vcount;
  AkDoubleArray * v;
  AkTree        * extra;
  AkUInt          count;
} AkVertexWeights;

typedef struct AkSkin {
  const char      * baseMesh;
  AkDoubleArray   * bindShapeMatrix;
  AkSource        * source;
  AkJoints        * joints;
  AkVertexWeights * vertexWeights;
  AkTree          * extra;
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
  const char * name;
  AkObject   * data;
  AkTree     * extra;

  struct AkController * next;
} AkController;

typedef struct AkSkeleton {
  const char * val;

  struct AkSkeleton * next;
} AkSkeleton;

#ifdef __cplusplus
}
#endif
#endif /* ak_controller_h */
