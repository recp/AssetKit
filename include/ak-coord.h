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

typedef struct AkCoordSys {
  AkAxis right;    /* +X */
  AkAxis up;       /* +Y */
  AkAxis fwd;      /* -Z */
} AkCoordSys;

extern AkCoordSys akCoord_RH_ZUP;
extern AkCoordSys akCoord_RH_YUP;

AK_EXPORT
void
ak_coordCvtSequence(AkCoordSys *oldCoordSystem,
                    float      *sequence,
                    AkCoordSys *newCoordSystem);

AK_EXPORT
void
ak_coordCvtSequenceTo(AkCoordSys *oldCoordSystem,
                      float      *oldSequence,
                      AkCoordSys *newCoordSystem,
                      float      *newSequence);

AK_EXPORT
void
ak_coordCvtSequenceN(AkCoordSys *oldCoordSystem,
                     float      *sequenceArray,
                     size_t     len,
                     AkCoordSys *newCoordSystem);

#endif /* ak_coord_h */
