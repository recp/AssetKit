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

#ifndef assetkit_options_h
#define assetkit_options_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/* TODO: */
typedef enum AkOption {
  AK_OPT_INDICES_DEFAULT            = 0,  /* false    */
  AK_OPT_INDICES_SINGLE_INTERLEAVED = 1,  /* false    */
  AK_OPT_INDICES_SINGLE_SEPARATE    = 2,  /* false    */
  AK_OPT_INDICES_SINGLE             = 3,  /* false    */
  AK_OPT_NOINDEX_INTERLEAVED        = 4,  /* true     */
  AK_OPT_NOINDEX_SEPARATE           = 5,  /* true     */
  AK_OPT_COORD                      = 6,  /* Y_UP     */
  AK_OPT_DEFAULT_ID_PREFIX          = 7,  /* id-      */
  AK_OPT_COMPUTE_BBOX               = 8,  /* false    */
  AK_OPT_TRIANGULATE                = 9,  /* true     */
  AK_OPT_GEN_NORMALS_IF_NEEDED      = 10, /* true     */
  AK_OPT_DEFAULT_PROFILE            = 11, /* COMMON   */
  AK_OPT_EFFECT_PROFILE             = 12, /* true     */
  AK_OPT_TECHNIQUE                  = 13, /* "common" */
  AK_OPT_TECHNIQUE_FX               = 14, /* "common" */
  AK_OPT_ZERO_INDEXED_INPUT         = 15, /* false    */
  AK_OPT_IMAGE_LOAD_FLIP_VERTICALLY = 16, /* true     */
  AK_OPT_ADD_DEFAULT_CAMERA         = 17, /* true     */
  AK_OPT_ADD_DEFAULT_LIGHT          = 18, /* false    */
  AK_OPT_COORD_CONVERT_TYPE         = 19, /* DEFAULT  */
  AK_OPT_BUGFIXES                   = 20, /* TRUE     */
  AK_OPT_GLTF_EXT_SPEC_GLOSS        = 21, /* TRUE     */
  AK_OPT_COMPUTE_EXACT_CENTER       = 22, /* FALSE    */
  AK_OPT_USE_MMAP                   = 23  /* TRUE     */
} AkOption;

AK_EXPORT
void
ak_opt_set(AkOption option, uintptr_t value);

AK_EXPORT
uintptr_t
ak_opt_get(AkOption option);

AK_EXPORT
void
ak_opt_set_str(AkOption option, const char *value);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_options_h */
