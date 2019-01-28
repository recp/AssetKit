/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_animation_h
#define ak_animation_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "type.h"
#include "source.h"
#include "url.h"

typedef enum AkSamplerBehavior {
  AK_SAMPLER_BEHAVIOR_UNDEFINED      = 0,
  AK_SAMPLER_BEHAVIOR_CONSTANT       = 1,
  AK_SAMPLER_BEHAVIOR_GRADIENT       = 2,
  AK_SAMPLER_BEHAVIOR_CYCLE          = 3,
  AK_SAMPLER_BEHAVIOR_OSCILLATE      = 4,
  AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE = 5
} AkSamplerBehavior;

typedef enum AkTargetPropertyType {
  AK_TARGET_UNKNOWN  = 0,
  AK_TARGET_X        = 1,
  AK_TARGET_Y        = 2,
  AK_TARGET_Z        = 3,
  AK_TARGET_XY       = 4,
  AK_TARGET_XYZ      = 5,
  AK_TARGET_ANGLE    = 6,
  AK_TARGET_POSITION = 7,
  AK_TARGET_SCALE    = 8,
  AK_TARGET_ROTATE   = 9,
  AK_TARGET_QUAT     = 10
} AkTargetPropertyType;

typedef enum AkInterpolationType {
  AK_INTERPOLATION_UNKNOWN  = 0,
  AK_INTERPOLATION_LINEAR   = 1,
  AK_INTERPOLATION_BEZIER   = 2,
  AK_INTERPOLATION_CARDINAL = 3,
  AK_INTERPOLATION_HERMITE  = 4,
  AK_INTERPOLATION_BSPLINE  = 5,
  AK_INTERPOLATION_STEP     = 6,

  AK_INTERPOLATION_MAXLEN   = 255
} AkInterpolationType;

typedef struct AkAnimSampler {
  struct AkAnimSampler *next;
  AkInput              *input;

  AkInput              *inputInput;
  AkInput              *outputInput;
  AkInput              *interpInput;
  AkInput              *inTangentInput;
  AkInput              *outTangentInput;

  AkInterpolationType   uniInterpolation;
  AkSamplerBehavior     pre;
  AkSamplerBehavior     post;
} AkAnimSampler;

typedef struct AkChannel {
  struct AkChannel    *next;
  const char          *target;
  void                *resolvedTarget;
  AkURL                source;
  AkTargetPropertyType targetType;
} AkChannel;

typedef struct AkAnimation {
  struct AkAnimation *prev;
  struct AkAnimation *next;
  struct AkAnimation *animation;
  AkAnimSampler      *sampler;
  AkSource           *source;
  AkChannel          *channel;
  const char         *name;
  AkTree             *extra;
} AkAnimation;

#ifdef __cplusplus
}
#endif
#endif /* ak_animation_h */
