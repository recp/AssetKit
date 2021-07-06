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

#ifndef assetkit_animation_h
#define assetkit_animation_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
  
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
  AK_TARGET_QUAT     = 10,
  AK_TARGET_WEIGHTS  = 11
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
  AkOneWayIterBase      base;
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
  AkOneWayIterBase    base;
  struct AkAnimation *animation;
  AkAnimSampler      *sampler;
  AkChannel          *channel;
  const char         *name;
  AkTree             *extra;
  
  /* TODO: WILL BE DELETED */
  AkSource           *source;
} AkAnimation;

#ifdef __cplusplus
}
#endif
#endif /* assetkit_animation_h */
