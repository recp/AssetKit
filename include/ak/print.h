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
  AK_PRINT_PACKAGE_PART_OTHER,
  AK_PRINT_PACKAGE_PART_GCODE
} AkPrintPackagePartType;

typedef enum AkPrintPackagePartFlag {
  AK_PRINT_PACKAGE_PART_DATA_MUTATED = 1u << 0
} AkPrintPackagePartFlag;

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

typedef enum AkPrintBooleanOperation {
  AK_PRINT_BOOLEAN_OPERATION_UNKNOWN      = 0,
  AK_PRINT_BOOLEAN_OPERATION_UNION        = 1,
  AK_PRINT_BOOLEAN_OPERATION_DIFFERENCE   = 2,
  AK_PRINT_BOOLEAN_OPERATION_INTERSECTION = 3
} AkPrintBooleanOperation;

typedef enum AkPrintBooleanShapeFlag {
  AK_PRINT_BOOLEAN_SHAPE_HAS_TRANSFORM = 1u << 0
} AkPrintBooleanShapeFlag;

typedef enum AkPrintBooleanOperandFlag {
  AK_PRINT_BOOLEAN_OPERAND_HAS_TRANSFORM = 1u << 0
} AkPrintBooleanOperandFlag;

typedef enum AkPrintDisplacement2DFlag {
  AK_PRINT_DISPLACEMENT_2D_HAS_CHANNEL     = 1u << 0,
  AK_PRINT_DISPLACEMENT_2D_HAS_TILESTYLE_U = 1u << 1,
  AK_PRINT_DISPLACEMENT_2D_HAS_TILESTYLE_V = 1u << 2,
  AK_PRINT_DISPLACEMENT_2D_HAS_FILTER      = 1u << 3
} AkPrintDisplacement2DFlag;

typedef enum AkPrintDisp2DGroupFlag {
  AK_PRINT_DISP2D_GROUP_HAS_OFFSET = 1u << 0
} AkPrintDisp2DGroupFlag;

typedef enum AkPrintDisp2DCoordFlag {
  AK_PRINT_DISP2D_COORD_HAS_FACTOR = 1u << 0
} AkPrintDisp2DCoordFlag;

typedef enum AkPrintDisplacementMeshFlag {
  AK_PRINT_DISPLACEMENT_MESH_HAS_DEFAULT_GROUP = 1u << 0
} AkPrintDisplacementMeshFlag;

typedef enum AkPrintDisplacementTriangleFlag {
  AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_GROUP = 1u << 0,
  AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D1    = 1u << 1,
  AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D2    = 1u << 2,
  AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D3    = 1u << 3
} AkPrintDisplacementTriangleFlag;

typedef enum AkPrintFunctionFromImage3DFlag {
  AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_OFFSET = 1u << 0,
  AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_SCALE  = 1u << 1,
  AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_FILTER       = 1u << 2,
  AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_U  = 1u << 3,
  AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_V  = 1u << 4,
  AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_W  = 1u << 5
} AkPrintFunctionFromImage3DFlag;

typedef enum AkPrintVolumeDataFlag {
  AK_PRINT_VOLUME_DATA_HAS_BASE_MATERIAL_ID = 1u << 0
} AkPrintVolumeDataFlag;

typedef enum AkPrintVolumetricElementType {
  AK_PRINT_VOLUMETRIC_ELEMENT_MATERIAL_MAPPING = 1,
  AK_PRINT_VOLUMETRIC_ELEMENT_COLOR            = 2,
  AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY         = 3
} AkPrintVolumetricElementType;

typedef enum AkPrintVolumetricElementFlag {
  AK_PRINT_VOLUMETRIC_ELEMENT_HAS_TRANSFORM        = 1u << 0,
  AK_PRINT_VOLUMETRIC_ELEMENT_HAS_MIN_FEATURE_SIZE = 1u << 1,
  AK_PRINT_VOLUMETRIC_ELEMENT_HAS_FALLBACK_VALUE   = 1u << 2,
  AK_PRINT_VOLUMETRIC_ELEMENT_REQUIRED             = 1u << 3
} AkPrintVolumetricElementFlag;

typedef enum AkPrintVolumetricMeshFlag {
  AK_PRINT_VOLUMETRIC_MESH_HAS_VOLUME_ID = 1u << 0
} AkPrintVolumetricMeshFlag;

typedef enum AkPrintLevelSetFlag {
  AK_PRINT_LEVEL_SET_HAS_TRANSFORM        = 1u << 0,
  AK_PRINT_LEVEL_SET_HAS_MIN_FEATURE_SIZE = 1u << 1,
  AK_PRINT_LEVEL_SET_HAS_MESH_BBOX_ONLY   = 1u << 2,
  AK_PRINT_LEVEL_SET_HAS_FALLBACK_VALUE   = 1u << 3,
  AK_PRINT_LEVEL_SET_HAS_VOLUME_ID        = 1u << 4
} AkPrintLevelSetFlag;

typedef enum AkPrintImplicitFunctionFlag {
  AK_PRINT_IMPLICIT_FUNCTION_HAS_XML = 1u << 0
} AkPrintImplicitFunctionFlag;

typedef struct AkPrintPackagePart {
  struct AkPrintPackagePart *next;
  const char                *name;
  const char                *contentType;
  const char                *relationshipType;
  const char                *relationshipId;
  const char                *relationshipTargetMode;
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

typedef struct AkPrintBooleanShape {
  struct AkPrintBooleanShape *next;
  const char                 *path;
  const char                 *basePath;
  float                       matrix[16];
  uint32_t                    objectId;
  uint32_t                    baseObjectId;
  uint32_t                    operandCount;
  AkPrintBooleanOperation     operation;
  uint32_t                    flags;
} AkPrintBooleanShape;

typedef struct AkPrintBooleanOperand {
  struct AkPrintBooleanOperand *next;
  const char                   *path;
  float                         matrix[16];
  uint32_t                      objectId;
  uint32_t                      flags;
} AkPrintBooleanOperand;

typedef struct AkPrintDisplacement2D {
  struct AkPrintDisplacement2D *next;
  const char                   *path;
  const char                   *imagePath;
  const char                   *channel;
  const char                   *tileStyleU;
  const char                   *tileStyleV;
  const char                   *filter;
  uint32_t                      id;
  uint32_t                      flags;
} AkPrintDisplacement2D;

typedef struct AkPrintNormVectorGroup {
  struct AkPrintNormVectorGroup *next;
  const char                    *path;
  uint32_t                       id;
  uint32_t                       vectorCount;
  uint32_t                       flags;
} AkPrintNormVectorGroup;

typedef struct AkPrintNormVector {
  struct AkPrintNormVector *next;
  float                     x;
  float                     y;
  float                     z;
  uint32_t                  flags;
} AkPrintNormVector;

typedef struct AkPrintDisp2DGroup {
  struct AkPrintDisp2DGroup *next;
  const char                *path;
  float                      height;
  float                      offset;
  uint32_t                   id;
  uint32_t                   displacementId;
  uint32_t                   normVectorGroupId;
  uint32_t                   coordCount;
  uint32_t                   flags;
} AkPrintDisp2DGroup;

typedef struct AkPrintDisp2DCoord {
  struct AkPrintDisp2DCoord *next;
  float                      u;
  float                      v;
  float                      factor;
  uint32_t                   normVectorIndex;
  uint32_t                   flags;
} AkPrintDisp2DCoord;

typedef struct AkPrintDisplacementMesh {
  struct AkPrintDisplacementMesh *next;
  const char                     *path;
  uint32_t                        objectId;
  uint32_t                        defaultGroupId;
  uint32_t                        triangleCount;
  uint32_t                        flags;
} AkPrintDisplacementMesh;

typedef struct AkPrintDisplacementTriangle {
  struct AkPrintDisplacementTriangle *next;
  uint32_t                            groupId;
  uint32_t                            d1;
  uint32_t                            d2;
  uint32_t                            d3;
  uint32_t                            flags;
} AkPrintDisplacementTriangle;

typedef struct AkPrintImage3D {
  struct AkPrintImage3D *next;
  const char            *path;
  const char            *name;
  uint32_t               id;
  uint32_t               rowCount;
  uint32_t               columnCount;
  uint32_t               sheetCount;
  uint32_t               imageSheetCount;
  uint32_t               flags;
} AkPrintImage3D;

typedef struct AkPrintImageSheet {
  struct AkPrintImageSheet *next;
  const char               *path;
  uint32_t                  flags;
} AkPrintImageSheet;

typedef struct AkPrintFunctionFromImage3D {
  struct AkPrintFunctionFromImage3D *next;
  const char                       *path;
  const char                       *displayName;
  const char                       *filter;
  const char                       *tileStyleU;
  const char                       *tileStyleV;
  const char                       *tileStyleW;
  float                             valueOffset;
  float                             valueScale;
  uint32_t                          id;
  uint32_t                          image3DId;
  uint32_t                          flags;
} AkPrintFunctionFromImage3D;

typedef struct AkPrintImplicitFunction {
  struct AkPrintImplicitFunction *next;
  const char                     *path;
  const char                     *xml;
  const char                     *displayName;
  uint32_t                        id;
  uint32_t                        flags;
} AkPrintImplicitFunction;

typedef struct AkPrintVolumeData {
  struct AkPrintVolumeData *next;
  const char               *path;
  uint32_t                  id;
  uint32_t                  baseMaterialId;
  uint32_t                  materialMappingCount;
  uint32_t                  colorCount;
  uint32_t                  propertyCount;
  uint32_t                  flags;
} AkPrintVolumeData;

typedef struct AkPrintVolumetricElement {
  struct AkPrintVolumetricElement *next;
  const char                      *channel;
  const char                      *name;
  float                            matrix[16];
  float                            minFeatureSize;
  float                            fallbackValue;
  uint32_t                         functionId;
  AkPrintVolumetricElementType     type;
  uint32_t                         flags;
} AkPrintVolumetricElement;

typedef struct AkPrintVolumetricMesh {
  struct AkPrintVolumetricMesh *next;
  const char                   *path;
  uint32_t                      objectId;
  uint32_t                      volumeId;
  uint32_t                      flags;
} AkPrintVolumetricMesh;

typedef struct AkPrintLevelSet {
  struct AkPrintLevelSet *next;
  const char             *path;
  const char             *channel;
  float                   matrix[16];
  float                   minFeatureSize;
  float                   fallbackValue;
  uint32_t                objectId;
  uint32_t                functionId;
  uint32_t                meshId;
  uint32_t                volumeId;
  uint32_t                flags;
} AkPrintLevelSet;

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
  AkPrintBooleanShape   *booleanShapes;
  AkPrintBooleanShape   *lastBooleanShape;
  AkPrintBooleanOperand *booleanOperands;
  AkPrintBooleanOperand *lastBooleanOperand;
  AkPrintDisplacement2D *displacement2Ds;
  AkPrintDisplacement2D *lastDisplacement2D;
  AkPrintNormVectorGroup *normVectorGroups;
  AkPrintNormVectorGroup *lastNormVectorGroup;
  AkPrintNormVector     *normVectors;
  AkPrintNormVector     *lastNormVector;
  AkPrintDisp2DGroup    *disp2DGroups;
  AkPrintDisp2DGroup    *lastDisp2DGroup;
  AkPrintDisp2DCoord    *disp2DCoords;
  AkPrintDisp2DCoord    *lastDisp2DCoord;
  AkPrintDisplacementMesh *displacementMeshes;
  AkPrintDisplacementMesh *lastDisplacementMesh;
  AkPrintDisplacementTriangle *displacementTriangles;
  AkPrintDisplacementTriangle *lastDisplacementTriangle;
  AkPrintImage3D       *image3Ds;
  AkPrintImage3D       *lastImage3D;
  AkPrintImageSheet    *imageSheets;
  AkPrintImageSheet    *lastImageSheet;
  AkPrintFunctionFromImage3D *functionFromImage3Ds;
  AkPrintFunctionFromImage3D *lastFunctionFromImage3D;
  AkPrintImplicitFunction *implicitFunctions;
  AkPrintImplicitFunction *lastImplicitFunction;
  AkPrintVolumeData    *volumeData;
  AkPrintVolumeData    *lastVolumeData;
  AkPrintVolumetricElement *volumetricElements;
  AkPrintVolumetricElement *lastVolumetricElement;
  AkPrintVolumetricMesh *volumetricMeshes;
  AkPrintVolumetricMesh *lastVolumetricMesh;
  AkPrintLevelSet      *levelSets;
  AkPrintLevelSet      *lastLevelSet;
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
  uint32_t               booleanShapeCount;
  uint32_t               booleanOperandCount;
  uint32_t               displacement2DCount;
  uint32_t               normVectorGroupCount;
  uint32_t               normVectorCount;
  uint32_t               disp2DGroupCount;
  uint32_t               disp2DCoordCount;
  uint32_t               displacementMeshCount;
  uint32_t               displacementTriangleCount;
  uint32_t               image3DCount;
  uint32_t               imageSheetCount;
  uint32_t               functionFromImage3DCount;
  uint32_t               implicitFunctionCount;
  uint32_t               volumeDataCount;
  uint32_t               volumetricElementCount;
  uint32_t               volumetricMeshCount;
  uint32_t               levelSetCount;
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

/* Validate core mesh printability and update AkPrintDocument.validationFlags.
   Passing AK_PRINT_VALIDATION_NONE runs all implemented mesh checks. */
AK_EXPORT
AkPrintValidationFlags
ak_printValidate(struct AkDoc        * __restrict doc,
                 AkPrintValidationFlags          checks);

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
bool
ak_printSetPackagePartRelationship(struct AkDoc           * __restrict doc,
                                   AkPrintPackagePart     * __restrict part,
                                   const char             * __restrict relationshipId,
                                   const char             * __restrict targetMode);

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

AK_EXPORT
AkPrintBooleanShape*
ak_printAddBooleanShape(struct AkDoc              * __restrict doc,
                        const char                * __restrict path,
                        const char                * __restrict basePath,
                        uint32_t                               objectId,
                        uint32_t                               baseObjectId,
                        AkPrintBooleanOperation                operation,
                        const float               * __restrict matrix,
                        uint32_t                               flags);

AK_EXPORT
AkPrintBooleanOperand*
ak_printAddBooleanOperand(struct AkDoc              * __restrict doc,
                          AkPrintBooleanShape       * __restrict shape,
                          const char                * __restrict path,
                          uint32_t                               objectId,
                          const float               * __restrict matrix,
                          uint32_t                               flags);

AK_EXPORT
AkPrintDisplacement2D*
ak_printAddDisplacement2D(struct AkDoc  * __restrict doc,
                          const char    * __restrict path,
                          uint32_t                   id,
                          const char    * __restrict imagePath,
                          const char    * __restrict channel,
                          const char    * __restrict tileStyleU,
                          const char    * __restrict tileStyleV,
                          const char    * __restrict filter,
                          uint32_t                   flags);

AK_EXPORT
AkPrintNormVectorGroup*
ak_printAddNormVectorGroup(struct AkDoc * __restrict doc,
                           const char   * __restrict path,
                           uint32_t                  id);

AK_EXPORT
AkPrintNormVector*
ak_printAddNormVector(struct AkDoc            * __restrict doc,
                      AkPrintNormVectorGroup  * __restrict group,
                      float                                x,
                      float                                y,
                      float                                z);

AK_EXPORT
AkPrintDisp2DGroup*
ak_printAddDisp2DGroup(struct AkDoc  * __restrict doc,
                       const char    * __restrict path,
                       uint32_t                   id,
                       uint32_t                   displacementId,
                       uint32_t                   normVectorGroupId,
                       float                      height,
                       float                      offset,
                       uint32_t                   flags);

AK_EXPORT
AkPrintDisp2DCoord*
ak_printAddDisp2DCoord(struct AkDoc          * __restrict doc,
                       AkPrintDisp2DGroup    * __restrict group,
                       float                              u,
                       float                              v,
                       uint32_t                           normVectorIndex,
                       float                              factor,
                       uint32_t                           flags);

AK_EXPORT
AkPrintDisplacementMesh*
ak_printAddDisplacementMesh(struct AkDoc * __restrict doc,
                            const char   * __restrict path,
                            uint32_t                  objectId,
                            uint32_t                  defaultGroupId,
                            uint32_t                  flags);

AK_EXPORT
AkPrintDisplacementTriangle*
ak_printAddDisplacementTriangle(struct AkDoc              * __restrict doc,
                                AkPrintDisplacementMesh   * __restrict mesh,
                                uint32_t                               groupId,
                                uint32_t                               d1,
                                uint32_t                               d2,
                                uint32_t                               d3,
                                uint32_t                               flags);

AK_EXPORT
AkPrintImage3D*
ak_printAddImage3D(struct AkDoc  * __restrict doc,
                   const char    * __restrict path,
                   uint32_t                   id,
                   const char    * __restrict name,
                   uint32_t                   rowCount,
                   uint32_t                   columnCount,
                   uint32_t                   sheetCount);

AK_EXPORT
AkPrintImageSheet*
ak_printAddImageSheet(struct AkDoc       * __restrict doc,
                      AkPrintImage3D     * __restrict image,
                      const char         * __restrict path);

AK_EXPORT
AkPrintFunctionFromImage3D*
ak_printAddFunctionFromImage3D(struct AkDoc  * __restrict doc,
                               const char    * __restrict path,
                               uint32_t                   id,
                               const char    * __restrict displayName,
                               uint32_t                   image3DId,
                               float                      valueOffset,
                               float                      valueScale,
                               const char    * __restrict filter,
                               const char    * __restrict tileStyleU,
                               const char    * __restrict tileStyleV,
                               const char    * __restrict tileStyleW,
                               uint32_t                   flags);

AK_EXPORT
AkPrintImplicitFunction*
ak_printAddImplicitFunction(struct AkDoc  * __restrict doc,
                            const char    * __restrict path,
                            uint32_t                   id,
                            const char    * __restrict displayName,
                            const char    * __restrict xml,
                            uint32_t                   flags);

AK_EXPORT
AkPrintVolumeData*
ak_printAddVolumeData(struct AkDoc  * __restrict doc,
                      const char    * __restrict path,
                      uint32_t                   id,
                      uint32_t                   baseMaterialId,
                      uint32_t                   flags);

AK_EXPORT
AkPrintVolumetricElement*
ak_printAddVolumetricElement(struct AkDoc              * __restrict doc,
                             AkPrintVolumeData         * __restrict volume,
                             AkPrintVolumetricElementType          type,
                             uint32_t                               functionId,
                             const char                * __restrict channel,
                             const char                * __restrict name,
                             const float               * __restrict matrix,
                             float                                  minFeatureSize,
                             float                                  fallbackValue,
                             uint32_t                               flags);

AK_EXPORT
AkPrintVolumetricMesh*
ak_printAddVolumetricMesh(struct AkDoc  * __restrict doc,
                          const char    * __restrict path,
                          uint32_t                   objectId,
                          uint32_t                   volumeId,
                          uint32_t                   flags);

AK_EXPORT
AkPrintLevelSet*
ak_printAddLevelSet(struct AkDoc  * __restrict doc,
                    const char    * __restrict path,
                    uint32_t                   objectId,
                    uint32_t                   functionId,
                    const char    * __restrict channel,
                    uint32_t                   meshId,
                    uint32_t                   volumeId,
                    const float   * __restrict matrix,
                    float                      minFeatureSize,
                    float                      fallbackValue,
                    uint32_t                   flags);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_print_h */
