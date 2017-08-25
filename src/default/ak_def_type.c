/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_def_type.h"
#include "../ak_common.h"

AkTypeDesc ak_def_type_descs[] = {
  {"float",    AKT_FLOAT,    sizeof(AkFloat),     0},
  {"float2",   AKT_FLOAT2,   sizeof(AkFloat2),    0},
  {"float3",   AKT_FLOAT3,   sizeof(AkFloat3),    0},
  {"float4",   AKT_FLOAT4,   sizeof(AkFloat4),    0},
  {"float2x2", AKT_FLOAT2x2, sizeof(AkFloat2),    0},
  {"float3x3", AKT_FLOAT3x3, sizeof(AkFloat2[2]), 0},
  {"float4x4", AKT_FLOAT4x4, sizeof(AkFloat4[4]), 0},

  {"int",      AKT_INT,      sizeof(AkInt),       0},
  {"int2",     AKT_INT2,     sizeof(AkInt[2]),    0},
  {"int3",     AKT_INT3,     sizeof(AkInt[3]),    0},
  {"int4",     AKT_INT4,     sizeof(AkInt[4]),    0},

  {"bool",     AKT_BOOL,     sizeof(AkBool),      0},
  {"bool2",    AKT_BOOL2,    sizeof(AkBool[2]),   0},
  {"bool3",    AKT_BOOL3,    sizeof(AkBool[3]),   0},
  {"bool4",    AKT_BOOL4,    sizeof(AkBool[4]),   0},

  {"string",   AKT_STRING,   sizeof(AkString),    0},
  {NULL,       0,            0,                   0}
};

const AkTypeDesc*
ak_def_typedesc(void) {
  return ak_def_type_descs;
}
