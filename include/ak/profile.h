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

#ifndef assetkit_profile_h
#define assetkit_profile_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "material.h"

struct AkEffect;

typedef enum AkProfileType {
  AK_PROFILE_TYPE_UNKOWN =-1,
  AK_PROFILE_TYPE_COMMON = 0,
  AK_PROFILE_TYPE_GLTF   = 6
} AkProfileType;

typedef struct AkTechniqueFx {
  /* const char * id; */
  /* const char * sid; */
  AkTechniqueFxCommon  *common;
  AkTree               *extra;
  struct AkTechniqueFx *next;
} AkTechniqueFx;

typedef struct AkTechniqueOverride {
  const char * ref;
  const char * pass;
} AkTechniqueOverride;

typedef struct AkTechniqueHint {
  struct AkTechniqueHint *next;
  const char             *platform;
  const char             *ref;
  const char             *profile;
  AkProfileType           profileType;
} AkTechniqueHint;

struct AkNewParam;

typedef struct AkProfile {
  /* const char * id; */
  AkProfileType     type;
  struct AkNewParam       *newparam;
  AkTechniqueFx    *technique;
  AkTree           *extra;
  struct AkProfile *next;
} AkProfile;

typedef AkProfile AkProfileCommon;
typedef AkProfile AkProfileGLTF;

AkProfile*
ak_profile(struct AkEffect * __restrict effect,
           AkProfile       * __restrict after);

AkProfileType
ak_profileType(struct AkEffect * __restrict effect);

uint32_t
ak_supportedProfiles(AkProfileType ** profileTypes);

void
ak_setSupportedProfiles(AkProfileType profileTypes[],
                        uint32_t      count);

const char*
ak_platform(void);

void
ak_setPlatform(const char platform[64]);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_profile_h */
