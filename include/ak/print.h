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

#ifndef assetkit_print_h
#define assetkit_print_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

struct AkDoc;

typedef uint32_t AkPrintFeatureFlags;
typedef uint32_t AkPrintValidationFlags;

typedef enum AkPrintFeature {
  AK_PRINT_FEATURE_CORE           = 1u << 0,
  AK_PRINT_FEATURE_MATERIALS      = 1u << 1,
  AK_PRINT_FEATURE_PRODUCTION     = 1u << 2,
  AK_PRINT_FEATURE_SLICE          = 1u << 3,
  AK_PRINT_FEATURE_BEAM_LATTICE   = 1u << 4,
  AK_PRINT_FEATURE_BOOLEAN        = 1u << 5,
  AK_PRINT_FEATURE_DISPLACEMENT   = 1u << 6,
  AK_PRINT_FEATURE_VOLUMETRIC     = 1u << 7,
  AK_PRINT_FEATURE_SECURE_CONTENT = 1u << 8,
  AK_PRINT_FEATURE_PACKAGE        = 1u << 9,
  AK_PRINT_FEATURE_THUMBNAIL      = 1u << 10,
  AK_PRINT_FEATURE_TEXTURES       = 1u << 11,
  AK_PRINT_FEATURE_UNKNOWN        = 1u << 31
} AkPrintFeature;

typedef enum AkPrintValidationFlag {
  AK_PRINT_VALIDATION_NONE                 = 0,
  AK_PRINT_VALIDATION_NON_MANIFOLD         = 1u << 0,
  AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES = 1u << 1,
  AK_PRINT_VALIDATION_OPEN_BOUNDARY        = 1u << 2,
  AK_PRINT_VALIDATION_NEGATIVE_SCALE       = 1u << 3,
  AK_PRINT_VALIDATION_UNSUPPORTED_FEATURE  = 1u << 4,
  AK_PRINT_VALIDATION_LOSSY_IMPORT         = 1u << 5,
  AK_PRINT_VALIDATION_LOSSY_EXPORT         = 1u << 6
} AkPrintValidationFlag;

typedef enum AkPrintPackagePartType {
  AK_PRINT_PACKAGE_PART_MODEL = 1,
  AK_PRINT_PACKAGE_PART_RELATIONSHIPS,
  AK_PRINT_PACKAGE_PART_METADATA,
  AK_PRINT_PACKAGE_PART_THUMBNAIL,
  AK_PRINT_PACKAGE_PART_TEXTURE,
  AK_PRINT_PACKAGE_PART_SLICE,
  AK_PRINT_PACKAGE_PART_OTHER
} AkPrintPackagePartType;

typedef struct AkPrintPackagePart {
  struct AkPrintPackagePart *next;
  const char                *name;
  const char                *contentType;
  const char                *relationshipType;
  AkPrintPackagePartType     type;
  uint32_t                   flags;
} AkPrintPackagePart;

typedef struct AkPrintDocument {
  AkPrintPackagePart  *parts;
  AkPrintPackagePart  *lastPart;
  AkTree              *extra;
  const char          *profileName;
  const char          *printerModel;
  void                *reserved;
  AkPrintFeatureFlags  features;
  AkPrintFeatureFlags  requiredFeatures;
  AkPrintFeatureFlags  unsupportedFeatures;
  AkPrintValidationFlags validationFlags;
  uint32_t             packagePartCount;
  uint32_t             buildItemCount;
  uint32_t             objectCount;
  uint32_t             meshObjectCount;
  uint32_t             componentObjectCount;
  uint32_t             materialGroupCount;
  uint32_t             materialPropertyCount;
  uint32_t             unknownExtensionCount;
} AkPrintDocument;

AK_EXPORT
AkPrintDocument*
ak_printDocument(struct AkDoc * __restrict doc);

AK_EXPORT
AkPrintDocument*
ak_printDocumentEnsure(struct AkDoc * __restrict doc);

AK_EXPORT
bool
ak_printHasFeature(const AkPrintDocument * __restrict print,
                   AkPrintFeatureFlags                 features);

AK_EXPORT
void
ak_printSetFeature(AkPrintDocument   * __restrict print,
                   AkPrintFeatureFlags             features);

AK_EXPORT
void
ak_printSetUnsupportedFeature(AkPrintDocument   * __restrict print,
                              AkPrintFeatureFlags             features);

AK_EXPORT
AkPrintPackagePart*
ak_printAddPackagePart(struct AkDoc           * __restrict doc,
                       AkPrintPackagePartType              type,
                       const char            * __restrict name,
                       const char            * __restrict contentType,
                       const char            * __restrict relationshipType);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_print_h */
