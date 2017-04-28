/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include "ak_profile.h"

#include <stdlib.h>
#include <string.h>

AkProfileType *ak__profileTypes;
uint32_t       ak__profileTypesCount;

AkProfileType
ak_profileType(struct AkEffect * __restrict effect) {
  AkProfileType defaultProfile;
  bool          useEffectProfile;
  uint32_t      i;
  bool          foundProfile;

  useEffectProfile = ak_opt_get(AK_OPT_EFFECT_PROFILE);
  foundProfile     = false;

  /* check if profile type is overridden by effect or not if option is set */
  /* to use this the AK_OPT_EFFECT_PROFILE option must set to true         */
  if (useEffectProfile) {
    for (i = 0; i < ak__profileTypesCount; i++) {
      if (effect->bestProfile == ak__profileTypes[i]) {
        foundProfile = true;
        break;
      }
    }

    if (foundProfile)
      return effect->bestProfile;
  }

  /* check default profile is supported or not */
  defaultProfile = (AkProfileType)ak_opt_get(AK_OPT_DEFAULT_PROFILE);
  for (i = 0; i < ak__profileTypesCount; i++) {
    if (defaultProfile == ak__profileTypes[i]) {
      foundProfile = true;
      break;
    }
  }

  if (foundProfile)
    return defaultProfile;

  /* return first supported profile */
  /* default is profile_COMMON -> [0] or [count - 1] */
  return ak__profileTypes[0];
}

uint32_t
ak_supportedProfiles(AkProfileType ** profileTypes) {
  *profileTypes = ak__profileTypes;
  return ak__profileTypesCount;
}

void
ak_setSupportedProfiles(AkProfileType profileTypes[],
                        uint32_t      count) {
  uint32_t i;
  bool     foundCommon;

  foundCommon = false;
  for (i = 0; i < count; i++) {
    if (profileTypes[i] == AK_PROFILE_TYPE_COMMON) {
      foundCommon = true;
      break;
    }
  }

  if (!foundCommon)
    count++;

  if (ak__profileTypes)
    ak_free(ak__profileTypes);

  ak__profileTypes = ak_malloc(NULL, sizeof(AkProfileType) * count);
  memcpy(ak__profileTypes, profileTypes, sizeof(AkProfileType) * count);

  /* make sure that common is exist */
  ak__profileTypes[count - 1] = AK_PROFILE_TYPE_COMMON;
  ak__profileTypesCount = count;
}

void _assetkit_hide
ak_profile_init() {
  ak__profileTypes = ak_malloc(NULL, sizeof(AkProfileType));
  *ak__profileTypes = AK_PROFILE_TYPE_COMMON;
  ak__profileTypesCount = 1;
}

void _assetkit_hide
ak_profile_deinit() {
  ak_free(ak__profileTypes);
}
