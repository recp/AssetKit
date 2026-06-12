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

typedef enum AkPrintProductionItemType {
  AK_PRINT_PRODUCTION_BUILD = 1,
  AK_PRINT_PRODUCTION_ITEM,
  AK_PRINT_PRODUCTION_OBJECT,
  AK_PRINT_PRODUCTION_COMPONENT,
  AK_PRINT_PRODUCTION_ALTERNATIVE
} AkPrintProductionItemType;

typedef enum AkPrintBeamLatticeFlag {
  AK_PRINT_BEAM_LATTICE_HAS_BALL_RADIUS         = 1u << 0,
  AK_PRINT_BEAM_LATTICE_HAS_CLIPPING_MESH       = 1u << 1,
  AK_PRINT_BEAM_LATTICE_HAS_REPRESENTATION_MESH = 1u << 2,
  AK_PRINT_BEAM_LATTICE_HAS_PID                 = 1u << 3,
  AK_PRINT_BEAM_LATTICE_HAS_PINDEX              = 1u << 4
} AkPrintBeamLatticeFlag;

typedef enum AkPrintBeamFlag {
  AK_PRINT_BEAM_HAS_R1   = 1u << 0,
  AK_PRINT_BEAM_HAS_R2   = 1u << 1,
  AK_PRINT_BEAM_HAS_P1   = 1u << 2,
  AK_PRINT_BEAM_HAS_P2   = 1u << 3,
  AK_PRINT_BEAM_HAS_PID  = 1u << 4,
  AK_PRINT_BEAM_HAS_CAP1 = 1u << 5,
  AK_PRINT_BEAM_HAS_CAP2 = 1u << 6
} AkPrintBeamFlag;

typedef enum AkPrintBeamBallFlag {
  AK_PRINT_BEAM_BALL_HAS_RADIUS = 1u << 0,
  AK_PRINT_BEAM_BALL_HAS_P      = 1u << 1,
  AK_PRINT_BEAM_BALL_HAS_PID    = 1u << 2
} AkPrintBeamBallFlag;

typedef struct AkPrintPackagePart {
  struct AkPrintPackagePart *next;
  const char                *name;
  const char                *contentType;
  const char                *relationshipType;
  const void                *data;
  size_t                     size;
  AkPrintPackagePartType     type;
  uint32_t                   flags;
} AkPrintPackagePart;

typedef struct AkPrintProductionItem {
  struct AkPrintProductionItem *next;
  const char                   *uuid;
  const char                   *path;
  const char                   *partNumber;
  const char                   *modelResolution;
  uint32_t                      objectId;
  uint32_t                      parentObjectId;
  AkPrintProductionItemType     type;
  uint32_t                      flags;
} AkPrintProductionItem;

typedef struct AkPrintSliceStack {
  struct AkPrintSliceStack *next;
  const char               *path;
  float                     zBottom;
  uint32_t                  id;
  uint32_t                  sliceCount;
  uint32_t                  sliceRefCount;
  uint32_t                  flags;
} AkPrintSliceStack;

typedef struct AkPrintSliceRef {
  struct AkPrintSliceRef *next;
  const char             *path;
  float                   zTop;
  uint32_t                stackId;
  uint32_t                flags;
} AkPrintSliceRef;

typedef struct AkPrintSlice {
  struct AkPrintSlice *next;
  const char          *path;
  float                zTop;
  uint32_t             stackId;
  uint32_t             vertexCount;
  uint32_t             polygonCount;
  uint32_t             segmentCount;
  uint32_t             flags;
} AkPrintSlice;

typedef struct AkPrintSliceObject {
  struct AkPrintSliceObject *next;
  const char                *path;
  const char                *slicePath;
  const char                *meshResolution;
  uint32_t                   objectId;
  uint32_t                   sliceStackId;
  uint32_t                   flags;
} AkPrintSliceObject;

typedef struct AkPrintBeamLattice {
  struct AkPrintBeamLattice *next;
  const char                *path;
  const char                *clippingMode;
  const char                *cap;
  const char                *ballMode;
  float                      minLength;
  float                      radius;
  float                      ballRadius;
  uint32_t                   objectId;
  uint32_t                   clippingMesh;
  uint32_t                   representationMesh;
  uint32_t                   pid;
  uint32_t                   pindex;
  uint32_t                   beamCount;
  uint32_t                   ballCount;
  uint32_t                   beamSetCount;
  uint32_t                   flags;
} AkPrintBeamLattice;

typedef struct AkPrintBeam {
  struct AkPrintBeam *next;
  const char         *cap1;
  const char         *cap2;
  float               r1;
  float               r2;
  uint32_t            v1;
  uint32_t            v2;
  uint32_t            p1;
  uint32_t            p2;
  uint32_t            pid;
  uint32_t            flags;
} AkPrintBeam;

typedef struct AkPrintBeamBall {
  struct AkPrintBeamBall *next;
  float                   radius;
  uint32_t                vindex;
  uint32_t                p;
  uint32_t                pid;
  uint32_t                flags;
} AkPrintBeamBall;

typedef struct AkPrintBeamSet {
  struct AkPrintBeamSet *next;
  const char            *name;
  const char            *identifier;
  uint32_t               refCount;
  uint32_t               ballRefCount;
  uint32_t               flags;
} AkPrintBeamSet;

typedef struct AkPrintDocument {
  AkPrintPackagePart    *parts;
  AkPrintPackagePart    *lastPart;
  AkPrintProductionItem *productionItems;
  AkPrintProductionItem *lastProductionItem;
  AkPrintSliceStack     *sliceStacks;
  AkPrintSliceStack     *lastSliceStack;
  AkPrintSliceRef       *sliceRefs;
  AkPrintSliceRef       *lastSliceRef;
  AkPrintSlice          *slices;
  AkPrintSlice          *lastSlice;
  AkPrintSliceObject    *sliceObjects;
  AkPrintSliceObject    *lastSliceObject;
  AkPrintBeamLattice    *beamLattices;
  AkPrintBeamLattice    *lastBeamLattice;
  AkPrintBeam           *beams;
  AkPrintBeam           *lastBeam;
  AkPrintBeamBall       *beamBalls;
  AkPrintBeamBall       *lastBeamBall;
  AkPrintBeamSet        *beamSets;
  AkPrintBeamSet        *lastBeamSet;
  AkTree                *extra;
  const char            *profileName;
  const char            *printerModel;
  void                  *reserved;
  AkPrintFeatureFlags    features;
  AkPrintFeatureFlags    requiredFeatures;
  AkPrintFeatureFlags    unsupportedFeatures;
  AkPrintValidationFlags validationFlags;
  uint32_t               packagePartCount;
  uint32_t               buildItemCount;
  uint32_t               objectCount;
  uint32_t               meshObjectCount;
  uint32_t               componentObjectCount;
  uint32_t               materialGroupCount;
  uint32_t               materialPropertyCount;
  uint32_t               unknownExtensionCount;
  uint32_t               productionItemCount;
  uint32_t               sliceStackCount;
  uint32_t               sliceRefCount;
  uint32_t               sliceCount;
  uint32_t               sliceObjectCount;
  uint32_t               beamLatticeCount;
  uint32_t               beamCount;
  uint32_t               beamBallCount;
  uint32_t               beamSetCount;
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

AK_EXPORT
bool
ak_printSetPackagePartData(struct AkDoc           * __restrict doc,
                           AkPrintPackagePart     * __restrict part,
                           const void             * __restrict data,
                           size_t                              size);

AK_EXPORT
AkPrintPackagePart*
ak_printAddPackagePartData(struct AkDoc           * __restrict doc,
                           AkPrintPackagePartType              type,
                           const char            * __restrict name,
                           const char            * __restrict contentType,
                           const char            * __restrict relationshipType,
                           const void            * __restrict data,
                           size_t                             size);

AK_EXPORT
AkPrintProductionItem*
ak_printAddProductionItem(struct AkDoc             * __restrict doc,
                          AkPrintProductionItemType             type,
                          const char              * __restrict uuid,
                          const char              * __restrict path,
                          const char              * __restrict partNumber,
                          const char              * __restrict modelResolution,
                          uint32_t                             objectId,
                          uint32_t                             parentObjectId);

AK_EXPORT
AkPrintSliceStack*
ak_printAddSliceStack(struct AkDoc  * __restrict doc,
                      const char    * __restrict path,
                      uint32_t                   id,
                      float                      zBottom);

AK_EXPORT
AkPrintSliceRef*
ak_printAddSliceRef(struct AkDoc  * __restrict doc,
                    const char    * __restrict path,
                    uint32_t                   stackId,
                    float                      zTop);

AK_EXPORT
AkPrintSlice*
ak_printAddSlice(struct AkDoc  * __restrict doc,
                 const char    * __restrict path,
                 uint32_t                   stackId,
                 float                      zTop,
                 uint32_t                   vertexCount,
                 uint32_t                   polygonCount,
                 uint32_t                   segmentCount);

AK_EXPORT
AkPrintSliceObject*
ak_printAddSliceObject(struct AkDoc  * __restrict doc,
                       const char    * __restrict path,
                       const char    * __restrict slicePath,
                       const char    * __restrict meshResolution,
                       uint32_t                   objectId,
                       uint32_t                   sliceStackId);

AK_EXPORT
AkPrintBeamLattice*
ak_printAddBeamLattice(struct AkDoc  * __restrict doc,
                       const char    * __restrict path,
                       uint32_t                   objectId,
                       float                      minLength,
                       float                      radius,
                       const char    * __restrict clippingMode,
                       const char    * __restrict cap,
                       const char    * __restrict ballMode,
                       float                      ballRadius,
                       uint32_t                   clippingMesh,
                       uint32_t                   representationMesh,
                       uint32_t                   pid,
                       uint32_t                   pindex,
                       uint32_t                   flags);

AK_EXPORT
AkPrintBeam*
ak_printAddBeam(struct AkDoc            * __restrict doc,
                AkPrintBeamLattice      * __restrict lattice,
                uint32_t                             v1,
                uint32_t                             v2,
                float                                r1,
                float                                r2,
                uint32_t                             p1,
                uint32_t                             p2,
                uint32_t                             pid,
                const char              * __restrict cap1,
                const char              * __restrict cap2,
                uint32_t                             flags);

AK_EXPORT
AkPrintBeamBall*
ak_printAddBeamBall(struct AkDoc            * __restrict doc,
                    AkPrintBeamLattice      * __restrict lattice,
                    uint32_t                             vindex,
                    float                                radius,
                    uint32_t                             p,
                    uint32_t                             pid,
                    uint32_t                             flags);

AK_EXPORT
AkPrintBeamSet*
ak_printAddBeamSet(struct AkDoc            * __restrict doc,
                   AkPrintBeamLattice      * __restrict lattice,
                   const char              * __restrict name,
                   const char              * __restrict identifier,
                   uint32_t                             refCount,
                   uint32_t                             ballRefCount);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_print_h */
