/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_profile_h
#define ak_profile_h
#ifdef __cplusplus
extern "C" {
#endif

typedef struct AkTechniqueFx {
  ak_asset_base

  /* const char * id; */
  /* const char * sid; */
  AkAnnotate   * annotate;
  AkBlinn      * blinn;
  AkConstantFx * constant;
  AkLambert    * lambert;
  AkPhong      * phong;
  AkPass       * pass;
  AkTree       * extra;

  struct AkTechniqueFx * next;
} AkTechniqueFx;

typedef struct AkProfile {
  ak_asset_base

  AkProfileType   profileType;
  /* const char    * id; */
  AkNewParam    * newparam;
  AkTechniqueFx * technique;
  AkTree        * extra;

  struct AkProfile * next;
} AkProfile;

typedef AkProfile AkProfileCommon;

typedef struct AkProfileCG {
  AkProfile base;

  AkCode     * code;
  AkInclude  * include;
  const char * platform;
} AkProfileCG;

typedef struct AkProfileGLES {
  AkProfile base;

  const char * platform;
} AkProfileGLES;

typedef struct AkProfileGLES2 {
  AkProfile base;

  AkCode     * code;
  AkInclude  * include;
  const char * language;
  const char * platforms;
} AkProfileGLES2;

typedef struct AkProfileGLSL {
  AkProfile base;

  AkCode     * code;
  AkInclude  * include;
  const char * platform;
} AkProfileGLSL;

typedef struct AkProfileBridge {
  AkProfile base;

  const char * platform;
  const char * url;
} AkProfileBridge;

#ifdef __cplusplus
}
#endif
#endif /* ak_profile_h */
