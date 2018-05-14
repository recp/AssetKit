/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_types_h
#define ak_types_h

#include <stdbool.h>

typedef enum AkTypeId {
  AKT_UNKNOWN       =-1,
  AKT_NONE          = 0,
  AKT_CUSTOM        = 0,

  AKT_BOOL          = 1,
  AKT_BOOL2         = 2,
  AKT_BOOL3         = 3,
  AKT_BOOL4         = 4,
  AKT_INT           = 5,
  AKT_INT2          = 6,
  AKT_INT3          = 7,
  AKT_INT4          = 8,
  AKT_FLOAT         = 9,
  AKT_FLOAT2        = 10,
  AKT_FLOAT3        = 11,
  AKT_FLOAT4        = 12,
  AKT_FLOAT2x2      = 13,
  AKT_FLOAT3x3      = 14,
  AKT_FLOAT4x4      = 15,
  AKT_STRING        = 16,

  AKT_SAMPLER1D     = 17,
  AKT_SAMPLER2D     = 18,
  AKT_SAMPLER3D     = 19,
  AKT_SAMPLER_CUBE  = 20,
  AKT_SAMPLER_RECT  = 21,
  AKT_SAMPLER_DEPTH = 22,

  AKT_IDREF         = 23,
  AKT_NAME          = 24,
  AKT_SIDREF        = 25,
  AKT_TOKEN         = 26,

  AKT_UINT          = 27,
  AKT_BYTE          = 28,
  AKT_UBYTE         = 29,
  AKT_SHORT         = 30,
  AKT_USHORT        = 31,

  AKT_EFFECT,
  AKT_PROFILE,
  AKT_PARAM,
  AKT_NEWPARAM,
  AKT_SETPARAM,
  AKT_TECHNIQUE_FX,
  AKT_TECHNIQUE,
  AKT_SAMPLER,
  AKT_TEXTURE,
  AKT_TEXTURE_NAME,
  AKT_TEXCOORD,
  AKT_NODE,
  AKT_SCENE,
  AKT_SOURCE,
  AKT_ACCESSOR,
  AKT_BUFFER
} AkTypeId;

typedef struct AkTypeDesc {
  const char *typeName;
  AkTypeId    typeId;
  int         size;
  int         userData;
} AkTypeDesc;

AkTypeId
ak_typeid(void * __restrict mem);

AkTypeId
ak_typeidh(AkHeapNode * __restrict hnode);

void
ak_setypeid(void * __restrict mem,
            AkTypeId tid);

bool
ak_isKindOf(void * __restrict mem,
            void * __restrict other);

AkTypeDesc*
ak_typeDesc(AkTypeId typeId);

void
ak_registerType(AkTypeId typeId, AkTypeDesc *desc);

#endif /* ak_types_h */
