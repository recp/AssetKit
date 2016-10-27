/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_coord_h
#define ak_coord_h

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
  AK_AXIS_ROT_DIRECTION_LH = -2,
  AK_AXIS_ROT_DIRECTION_RH = 0
} AkAxisRotDirection;

typedef struct AkCoordSys {
  AkAxis right;    /* +X */
  AkAxis up;       /* +Y */
  AkAxis fwd;      /* -Z */

  /*
   * the default value is AK_AXIS_ROT_DIRECTION_RH (Right Handed)
   */
  AkAxisRotDirection rotDirection;
} AkCoordSys;

/* Right Hand (Default) */
extern AkCoordSys * AK_ZUP;
extern AkCoordSys * AK_YUP;
extern AkCoordSys * AK_XUP;

/* Left Hand */
extern AkCoordSys * AK_ZUP_LH;
extern AkCoordSys * AK_YUP_LH;
extern AkCoordSys * AK_XUP_LH;

AK_EXPORT
AkCoordSys *
ak_defaultCoordSys();

AK_EXPORT
void
ak_defaultSetCoordSys(AkCoordSys * coordsys);

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
                     AkFloat4x4  oldTransform,
                     AkCoordSys *newCoordSystem,
                     AkFloat4x4  newTransform);

#endif /* ak_coord_h */