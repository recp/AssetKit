/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_coord_h
#define ak_coord_h
#ifdef __cplusplus
extern "C" {
#endif

typedef enum AkCoordCvtType {
  AK_COORD_CVT_FIX_ROTATION = 0, /* add only rotation               */
  AK_COORD_CVT_ALL          = 1, /* change positions, all things... */
  AK_COORD_CVT_DEFAULT      = AK_COORD_CVT_FIX_ROTATION
} AkCoordCvtType;

typedef struct AkAxisAccessor {
  int8_t right;
  int8_t up;
  int8_t fwd;

  int8_t s_right;
  int8_t s_up;
  int8_t s_fwd;
} AkAxisAccessor;

typedef enum AkAxis {
  AK_AXIS_NEGATIVE_X =-1,
  AK_AXIS_NEGATIVE_Y =-2,
  AK_AXIS_NEGATIVE_Z =-3,

  AK_AXIS_POSITIVE_X = 1,
  AK_AXIS_POSITIVE_Y = 2,
  AK_AXIS_POSITIVE_Z = 3,
} AkAxis;

typedef enum AkAxisRotDirection {
  AK_AXIS_ROT_DIR_LH = -2,
  AK_AXIS_ROT_DIR_RH = 0
} AkAxisRotDirection;

typedef struct AkAxisOrientation {
  AkAxis right;    /* +X */
  AkAxis up;       /* +Y */
  AkAxis fwd;      /* -Z */
} AkAxisOrientation;

typedef struct AkCoordSys {
  AkAxisOrientation  axis;

  /* the default value is AK_AXIS_ROT_DIRECTION_RH (Right Handed) */
  AkAxisRotDirection rotDirection;

  /* when creating custom coord sys, this value must be set correctly, 
     there is no default value */
  AkAxisOrientation  cameraOrientation;
} AkCoordSys;

/* Right Hand (Default) */
extern AkCoordSys * AK_ZUP;
extern AkCoordSys * AK_YUP;
extern AkCoordSys * AK_XUP;

/* Left Hand */
extern AkCoordSys * AK_ZUP_LH;
extern AkCoordSys * AK_YUP_LH;
extern AkCoordSys * AK_XUP_LH;

static
AK_INLINE
void
ak_coordAxisToiVec3(AkAxisOrientation axisOri, int32_t vec[3]) {
  vec[0] = axisOri.right;
  vec[1] = axisOri.up;
  vec[2] = axisOri.fwd;
}

static
AK_INLINE
void
ak_coordToiVec3(AkCoordSys * __restrict coordSys, int32_t vec[3]) {
  vec[0] = coordSys->axis.right;
  vec[1] = coordSys->axis.up;
  vec[2] = coordSys->axis.fwd;
}

static
AK_INLINE
bool
ak_coordOrientationIsEq(AkCoordSys *c1, AkCoordSys *c2) {
  return  c1->axis.right == c2->axis.right
            && c1->axis.up == c2->axis.up
            && c1->axis.fwd == c2->axis.fwd;
}

AK_EXPORT
void
ak_coordCvtVector(AkCoordSys *oldCoordSystem,
                  float      *vector,
                  AkCoordSys *newCoordSystem);

AK_EXPORT
void
ak_coordCvtVectorTo(AkCoordSys *oldCoordSystem,
                    float      *oldVector,
                    AkCoordSys *newCoordSystem,
                    float      *newVector);

AK_EXPORT
void
ak_coordCvtVectors(AkCoordSys *oldCoordSystem,
                   float      *vectorArray,
                   size_t      len,
                   AkCoordSys *newCoordSystem);

AK_EXPORT
void
ak_coordCvtTransform(AkCoordSys *oldCoordSystem,
                     AkFloat4x4  transform,
                     AkCoordSys *newCoordSystem);

AK_EXPORT
void
ak_coordFixCamOri(AkCoordSys *oldCoordSys,
                  AkCoordSys *newCoordSys,
                  AkFloat4x4  transform);

struct AkDoc;
struct AkNode;

AK_EXPORT
void
ak_coordRotNodeForFixCamOri(struct AkDoc *doc,
                            void         *memparent,
                            AkObject    **destTransform);

AK_EXPORT
void
ak_coordCvtNodeTransforms(struct AkDoc  * __restrict doc,
                          struct AkNode * __restrict node);

#ifdef __cplusplus
}
#endif
#endif /* ak_coord_h */
