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

#ifndef assetkit_obj_exp_common_h
#define assetkit_obj_exp_common_h

#include "../../../common.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct WOBJExpWriter {
  FILE          *file;
  size_t         len;
  AkResult       result;
  unsigned char  buffer[64u * 1024u];
} WOBJExpWriter;

typedef struct WOBJExpMaterial {
  AkMaterial *material;
  char       *name;
  bool        used;
} WOBJExpMaterial;

typedef struct WOBJExpMaterialSlot {
  AkMaterial *material;
  uint32_t    index;
} WOBJExpMaterialSlot;

typedef struct WOBJExpState {
  AkDoc           *doc;
  WOBJExpMaterial *materials;
  WOBJExpMaterialSlot *materialLookup;
  char           **imageUris;
  bool            *imageUriFailed;
  char            *outDir;
  char            *mtlPath;
  char            *mtlBaseName;
  WOBJExpWriter    w;
  uint32_t         materialCount;
  uint32_t         materialUsedCount;
  uint32_t         materialLookupCapacity;
  uint32_t         imageUriCount;
  uint32_t         objectCount;
  uint32_t         vCount;
  uint32_t         vtCount;
  uint32_t         vnCount;
  bool             hasSmoothState;
  bool             smoothState;
  bool             wroteMtllib;
} WOBJExpState;

#endif /* assetkit_obj_exp_common_h */
