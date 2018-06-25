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

typedef enum AkAnimSamplerBehavior {
  AK_SAMPLER_BEHAVIOR_UNDEFINED      = 0,
  AK_SAMPLER_BEHAVIOR_CONSTANT       = 1,
  AK_SAMPLER_BEHAVIOR_GRADIENT       = 2,
  AK_SAMPLER_BEHAVIOR_CYCLE          = 3,
  AK_SAMPLER_BEHAVIOR_OSCILLATE      = 4,
  AK_SAMPLER_BEHAVIOR_CYCLE_RELATIVE = 5
} AkAnimSamplerBehavior;

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
  AkInput              *input;
  AkAnimSamplerBehavior preBehavior;
  AkAnimSamplerBehavior postBehavior;
} AkAnimSampler;

typedef struct AkChannel {
  AkURL             source;
  const char       *target;
  struct AkChannel *next;
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
