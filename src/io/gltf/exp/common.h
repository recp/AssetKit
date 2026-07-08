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

#ifndef assetkit_gltf_exp_common_h
#define assetkit_gltf_exp_common_h

#include "../../../../include/ak/assetkit.h"
#include "../../../image/export.h"
#include "../../../string_fast.h"

#include <ds/rb.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t GLTFExpIndex;

#define GLTF_EXP_INDEX_NONE UINT32_MAX
#define GLTF_EXP_MINMAX_COMPONENT_MAX 4u

typedef struct GLTFExpNodeOut {
  AkNode     *node;
  AkFloatArray *morphWeights;
  const char *name;
  GLTFExpIndex childOffset;
  GLTFExpIndex meshIndex;
  GLTFExpIndex cameraIndex;
  GLTFExpIndex lightIndex;
  GLTFExpIndex skinIndex;
  uint32_t    childCount;
  bool        hasMesh;
  bool        hasCamera;
  bool        hasLight;
  bool        hasSkin;
  bool        forceTRS;
  bool        bakeLocalTransform;
  bool        forceVisibilityExtension;
} GLTFExpNodeOut;

typedef struct GLTFExpNodeTable {
  GLTFExpNodeOut *items;
  size_t          count;
  size_t          capacity;
} GLTFExpNodeTable;

typedef struct GLTFExpIndexList {
  GLTFExpIndex *items;
  size_t        count;
  size_t        capacity;
} GLTFExpIndexList;

typedef struct GLTFExpMeshOut {
  AkGeometry         *geom;
  AkInstanceGeometry *instance;
  AkNode             *bakeNode;
  GLTFExpIndex        skinAttrOffset;
  GLTFExpIndex        morphAttrOffset;
  uint32_t            skinAttrCount;
  uint32_t            morphAttrCount;
  uint32_t            morphAttrPrimCount;
} GLTFExpMeshOut;

typedef struct GLTFExpMeshTable {
  GLTFExpMeshOut *items;
  RBTree         *map;
  size_t          count;
  size_t          capacity;
} GLTFExpMeshTable;

typedef struct GLTFExpSkinOut {
  AkSkin         *skin;
  AkInstanceSkin *instance;
  GLTFExpIndex    inverseBindAccessorIndex;
} GLTFExpSkinOut;

typedef struct GLTFExpSkinTable {
  GLTFExpSkinOut *items;
  RBTree         *map;
  size_t          count;
  size_t          capacity;
} GLTFExpSkinTable;

typedef struct GLTFExpSkinAttrOut {
  void         *data;
  uint16_t     *joints;
  float        *weights;
  GLTFExpIndex  jointsAccessorIndex;
  GLTFExpIndex  weightsAccessorIndex;
  uint32_t      vertexCount;
} GLTFExpSkinAttrOut;

typedef struct GLTFExpSkinAttrTable {
  GLTFExpSkinAttrOut *items;
  size_t              count;
  size_t              capacity;
} GLTFExpSkinAttrTable;

typedef struct GLTFExpMorphAttrOut {
  void         *positionData;
  void         *normalData;
  void         *tangentData;
  GLTFExpIndex  positionAccessorIndex;
  GLTFExpIndex  normalAccessorIndex;
  GLTFExpIndex  tangentAccessorIndex;
  uint32_t      vertexCount;
} GLTFExpMorphAttrOut;

typedef struct GLTFExpMorphAttrTable {
  GLTFExpMorphAttrOut *items;
  size_t               count;
  size_t               capacity;
} GLTFExpMorphAttrTable;

typedef struct GLTFExpPositionAttrOut {
  AkMeshPrimitive *primitive;
  void            *data;
  GLTFExpIndex     accessorIndex;
  uint32_t         vertexCount;
} GLTFExpPositionAttrOut;

typedef struct GLTFExpPositionAttrTable {
  GLTFExpPositionAttrOut *items;
  size_t                  count;
  size_t                  capacity;
} GLTFExpPositionAttrTable;

typedef struct GLTFExpBakedPrimAttrOut {
  AkNode          *node;
  AkMeshPrimitive *primitive;
  void            *positionData;
  void            *normalData;
  GLTFExpIndex     positionAccessorIndex;
  GLTFExpIndex     normalAccessorIndex;
  uint32_t         vertexCount;
} GLTFExpBakedPrimAttrOut;

typedef struct GLTFExpBakedAttrTable {
  GLTFExpBakedPrimAttrOut *items;
  size_t                   count;
  size_t                   capacity;
} GLTFExpBakedAttrTable;

typedef enum GLTFExpAnimPath {
  GLTF_EXP_ANIM_TRANSLATION,
  GLTF_EXP_ANIM_ROTATION,
  GLTF_EXP_ANIM_SCALE,
  GLTF_EXP_ANIM_WEIGHTS,
  GLTF_EXP_ANIM_POINTER_NODE_VISIBLE,
  GLTF_EXP_ANIM_POINTER_MATERIAL_VALUE,
  GLTF_EXP_ANIM_POINTER_TEXTURE_TRANSFORM
} GLTFExpAnimPath;

typedef struct GLTFExpAnimSamplerOut {
  AkAnimSampler       *sampler;
  GLTFExpIndex         inputAccessorIndex;
  GLTFExpIndex         outputAccessorIndex;
  AkInterpolationType  interpolation;
} GLTFExpAnimSamplerOut;

typedef struct GLTFExpAnimChannelOut {
  GLTFExpIndex   samplerIndex;
  GLTFExpIndex   nodeIndex;
  GLTFExpAnimPath path;
  uint32_t        pointerRole;
  uint32_t        pointerProp;
} GLTFExpAnimChannelOut;

typedef struct GLTFExpAnimOut {
  AkAnimation *animation;
  const char  *name;
  GLTFExpIndex samplerOffset;
  GLTFExpIndex channelOffset;
  uint32_t     samplerCount;
  uint32_t     channelCount;
} GLTFExpAnimOut;

typedef struct GLTFExpAnimTable {
  GLTFExpAnimOut *items;
  size_t          count;
  size_t          capacity;
} GLTFExpAnimTable;

typedef struct GLTFExpAnimSamplerTable {
  GLTFExpAnimSamplerOut *items;
  size_t                 count;
  size_t                 capacity;
} GLTFExpAnimSamplerTable;

typedef struct GLTFExpAnimChannelTable {
  GLTFExpAnimChannelOut *items;
  size_t                 count;
  size_t                 capacity;
} GLTFExpAnimChannelTable;

typedef struct GLTFExpSceneOut {
  AkScene      *scene;
  GLTFExpIndex  rootOffset;
  uint32_t      rootCount;
} GLTFExpSceneOut;

typedef struct GLTFExpSceneTable {
  GLTFExpSceneOut *items;
  size_t           count;
  size_t           capacity;
} GLTFExpSceneTable;

typedef struct GLTFExpPtrTable {
  void  **items;
  RBTree *map;
  size_t  count;
  size_t  capacity;
} GLTFExpPtrTable;

typedef struct GLTFExpStringOut {
  const char *name;
  size_t      nameLen;
} GLTFExpStringOut;

typedef struct GLTFExpStringTable {
  GLTFExpStringOut *items;
  size_t            count;
  size_t            capacity;
} GLTFExpStringTable;

typedef struct GLTFExpMaterialOut {
  AkMaterial         *material;
  AkMeshPrimitive    *primitive;
  AkInstanceGeometry *instance;
} GLTFExpMaterialOut;

typedef struct GLTFExpMaterialTable {
  GLTFExpMaterialOut *items;
  size_t              count;
  size_t              capacity;
} GLTFExpMaterialTable;

typedef enum GLTFExpAccessorKind {
  GLTF_EXP_ACCESSOR_ASSETKIT,
  GLTF_EXP_ACCESSOR_INDEX_ARRAY,
  GLTF_EXP_ACCESSOR_RAW,
  GLTF_EXP_ACCESSOR_RAW_VIEW,
  GLTF_EXP_ACCESSOR_RAW_FILE_VIEW
} GLTFExpAccessorKind;

typedef struct GLTFExpAccessorOut {
  AkAccessor         *accessor;
  AkMeshPrimitive    *primitive;
  AkIndexArray       *indices;
  const void         *rawData;
  char               *rawPath;
  size_t              byteOffset;
  size_t              byteLength;
  size_t              rawByteLength;
  double              min[GLTF_EXP_MINMAX_COMPONENT_MAX];
  double              max[GLTF_EXP_MINMAX_COMPONENT_MAX];
  GLTFExpAccessorKind kind;
  GLTFExpIndex        jsonIndex;
  uint32_t            bufferViewTarget;
  AkTypeId            indexComponentType;
  AkTypeId            rawComponentType;
  AkComponentSize     rawComponentSize;
  uint32_t            rawComponentCount;
  uint32_t            rawCount;
  uint32_t            minMaxCount;
  bool                minMaxRequired;
  bool                hasMinMax;
  bool                normalizeVec3;
} GLTFExpAccessorOut;

typedef struct GLTFExpAccessorTable {
  GLTFExpAccessorOut *items;
  RBTree             *accessorMap;
  RBTree             *primitiveMap;
  RBTree             *rawMap;
  size_t              count;
  size_t              capacity;
  GLTFExpIndex        jsonCount;
} GLTFExpAccessorTable;

typedef struct GLTFExpState {
  AkDoc                *doc;
  GLTFExpNodeTable     nodes;
  GLTFExpSceneTable    scenes;
  GLTFExpIndexList     nodeChildren;
  GLTFExpIndexList     sceneRoots;
  GLTFExpMeshTable     meshes;
  GLTFExpMaterialTable materials;
  GLTFExpPtrTable      textures;
  GLTFExpPtrTable      images;
  GLTFExpIndex        *imageBufferViews;
  const char         **imageMimeTypes;
  AkImageExportPayload *imagePayloads;
  char               **imageExportUris;
  GLTFExpPtrTable      samplers;
  GLTFExpPtrTable      cameras;
  GLTFExpPtrTable      lights;
  GLTFExpSkinTable     skins;
  GLTFExpPtrTable      skinJointRoots;
  GLTFExpPtrTable      sceneSkinJointRoots;
  GLTFExpSkinAttrTable skinAttrs;
  GLTFExpMorphAttrTable morphAttrs;
  GLTFExpPositionAttrTable positionAttrs;
  GLTFExpBakedAttrTable bakedAttrs;
  GLTFExpAnimTable     animations;
  GLTFExpAnimSamplerTable animSamplers;
  GLTFExpAnimChannelTable animChannels;
  GLTFExpAccessorTable accessors;
  GLTFExpStringTable   preservedExtensions;
  RBTree              *nodeStack;
  RBTree              *nodeMap;
  char                *outDir;
  char                *binPath;
  char                *binUri;
  size_t               binByteLength;
  GLTFExpIndex         defaultSceneIndex;
  GLTFExpIndex         materialVariantCount;
  AkResult             failResult;
  bool                 failed;
  bool                 glb;
  bool                 usesNodeVisibility;
  bool                 usesGpuInstancing;
  bool                 usesMeshQuantization;
  bool                 usesAnimationPointer;
  bool                 usesMaterialVariants;
} GLTFExpState;

#endif /* assetkit_gltf_exp_common_h */
