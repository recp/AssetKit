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

#ifndef ak_3mf_imp_internal_h
#define ak_3mf_imp_internal_h

#include "common.h"

typedef struct AkZipArchive AkZipArchive;
typedef struct AK3MFFastPreparedModel AK3MFFastPreparedModel;

typedef struct AK3MFPreparedModelEntry {
  const char              *path;
  AK3MFFastPreparedModel  *model;
} AK3MFPreparedModelEntry;

typedef enum AK3MFObjectKind {
  AK_3MF_OBJECT_EMPTY        = 0,
  AK_3MF_OBJECT_MESH         = 1,
  AK_3MF_OBJECT_COMPONENTS   = 2,
  AK_3MF_OBJECT_BOOLEAN      = 3,
  AK_3MF_OBJECT_DISPLACEMENT = 4,
  AK_3MF_OBJECT_LEVELSET     = 5
} AK3MFObjectKind;

typedef struct AK3MFComponent {
  const char *path;
  uint32_t    objectId;
  float       matrix[16];
} AK3MFComponent;

typedef struct AK3MFPropertyGroup {
  AkMaterialPropertySet *set;
  const char            *path;
  uint8_t               *colors;
  float                 *texcoords;
  uint32_t               id;
  uint32_t               count;
  bool                   hasAlpha;
  bool                   hasColors;
  bool                   hasTexcoords;
} AK3MFPropertyGroup;

typedef struct AK3MFObject {
  AK3MFComponent *components;
  const char     *path;
  uint32_t        id;
  AkGeometry     *geom;
  const char     *name;
  uint32_t        pid;
  uint32_t        pindex;
  uint32_t        componentCount;
  AK3MFObjectKind kind;
} AK3MFObject;

typedef struct AK3MFBambuOrcaPartMaterial {
  uint32_t objectId;
  uint32_t extruder;
} AK3MFBambuOrcaPartMaterial;

typedef struct AK3MFImportState {
  AkDoc                       *doc;
  AkPrintDocument             *print;
  AK3MFObject                 *objects;
  AK3MFPropertyGroup          *properties;
  AK3MFBambuOrcaPartMaterial  *bambuOrcaParts;
  AkMaterial                 **bambuOrcaMaterials;
  uint8_t                    (*bambuOrcaColors)[4];
  AK3MFPreparedModelEntry     *preparedModels;
  AkZipArchive                *package;
  const char                  *packagePath;
  const char                  *rootModelPath;
  const char                  *currentModelPath;
  const char                 **loadedModelPaths;
  size_t                       objectCount;
  size_t                       objectCapacity;
  size_t                       propertyCount;
  size_t                       propertyCapacity;
  size_t                       loadedModelCount;
  size_t                       loadedModelCapacity;
  size_t                       bambuOrcaPartCount;
  size_t                       bambuOrcaPartCapacity;
  size_t                       bambuOrcaColorCount;
  size_t                       preparedModelCount;
  size_t                       preparedModelCapacity;
} AK3MFImportState;

typedef enum AK3MFFastLoadResult {
  AK_3MF_FAST_LOAD_UNSUPPORTED = 0,
  AK_3MF_FAST_LOAD_LOADED      = 1,
  AK_3MF_FAST_LOAD_ERROR       = 2
} AK3MFFastLoadResult;

AK_HIDE
bool
ak_3mf_parse_color_slice(const char * __restrict p,
                         size_t                  len,
                         uint8_t                 rgba[4]);

AK_HIDE
AkMaterialInput*
ak_3mf_material_color_input(AkHeap     * __restrict heap,
                            void       * __restrict parent,
                            uint8_t                  rgba[4]);

AK_HIDE
AkMaterialInput*
ak_3mf_material_scalar_input(AkHeap      * __restrict heap,
                             void        * __restrict parent,
                             const char  * __restrict semantic,
                             float                    value);

AK_HIDE
AK3MFFastLoadResult
ak_3mf_fast_load_mesh_model_part(AK3MFImportState * __restrict st,
                                 const char       * __restrict modelPath,
                                 const char       * __restrict modelData,
                                 size_t                         modelSize);

AK_HIDE
AK3MFFastPreparedModel*
ak_3mf_fast_prepare_model_part(const char * __restrict modelData,
                               size_t                  modelSize,
                               bool                    preferVendorPaint);

AK_HIDE
void
ak_3mf_fast_prepared_model_free(AK3MFFastPreparedModel * __restrict prepared);

AK_HIDE
AK3MFFastLoadResult
ak_3mf_fast_commit_prepared_model_part(AK3MFImportState        * __restrict st,
                                       const char              * __restrict modelPath,
                                       AK3MFFastPreparedModel  * __restrict prepared);

#endif /* ak_3mf_imp_internal_h */
