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

#ifndef assetkit_types_h
#define assetkit_types_h

#include "common.h"
#include <stdbool.h>

typedef enum AkTypeId {
  AKT_UNKNOWN       =-1,
  AKT_NONE          = 0,
  AKT_CUSTOM        = 0,

  AKT_OBJECT        = 1,

  AKT_BOOL          = 2,
  AKT_BOOL2         = 3,
  AKT_BOOL3         = 4,
  AKT_BOOL4         = 5,
  AKT_INT           = 6,
  AKT_INT2          = 7,
  AKT_INT3          = 8,
  AKT_INT4          = 9,
  AKT_FLOAT         = 10,
  AKT_FLOAT2        = 11,
  AKT_FLOAT3        = 12,
  AKT_FLOAT4        = 13,
  AKT_FLOAT2x2      = 14,
  AKT_FLOAT3x3      = 15,
  AKT_FLOAT4x4      = 16,
  AKT_STRING        = 17,

  AKT_SAMPLER1D     = 18,
  AKT_SAMPLER2D     = 19,
  AKT_SAMPLER3D     = 20,
  AKT_SAMPLER_CUBE  = 21,
  AKT_SAMPLER_RECT  = 22,
  AKT_SAMPLER_DEPTH = 23,

  AKT_IDREF         = 24,
  AKT_NAME          = 25,
  AKT_SIDREF        = 26,
  AKT_TOKEN         = 27,

  AKT_UINT          = 28,
  AKT_BYTE          = 29,
  AKT_UBYTE         = 30,
  AKT_SHORT         = 31,
  AKT_USHORT        = 32,
  AKT_DOUBLE        = 33,
  AKT_INT64         = 34,
  AKT_UINT64        = 35,

  AKT_EFFECT,
  AKT_PROFILE,
  AKT_PARAM,
  AKT_NEWPARAM,
  AKT_SETPARAM,
  AKT_TECHNIQUE_FX,
  AKT_TECHNIQUE,
  AKT_SAMPLER,
  AKT_TEXTURE,
  AKT_TEXTURE_REF,
  AKT_TEXTURE_NAME,
  AKT_TEXCOORD,
  AKT_NODE,
  AKT_SCENE,
  AKT_SOURCE,
  AKT_ACCESSOR,
  AKT_BUFFER,
  AKT_GEOMETRY,
  AKT_MESH,
  AKT_CONTROLLER,
  AKT_SKIN,
  AKT_MORPH,

  AKT_LOOKAT,
  AKT_TRANSLATE,
  AKT_ROTATE,
  AKT_SCALE,
  AKT_SKEW,
  AKT_MATRIX,
  AKT_QUATERNION
} AkTypeId;

typedef struct AkTypeDesc {
  const char *typeName;
  AkTypeId    typeId;
  int         size;
  int         userData;
} AkTypeDesc;

AK_EXPORT
AkTypeId
ak_typeid(void * __restrict mem);

AK_EXPORT
AkTypeId
ak_typeidh(AkHeapNode * __restrict hnode);

AK_EXPORT
void
ak_setypeid(void * __restrict mem,
            AkTypeId tid);

AK_EXPORT
bool
ak_isKindOf(void * __restrict mem,
            void * __restrict other);

AK_EXPORT
AkTypeDesc*
ak_typeDesc(AkTypeId typeId);

AK_EXPORT
AkTypeDesc*
ak_typeDescByName(const char * __restrict name);

AK_EXPORT
void
ak_registerType(AkTypeId typeId, AkTypeDesc *desc);

#endif /* assetkit_types_h */
