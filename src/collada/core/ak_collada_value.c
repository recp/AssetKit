/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_value.h"
#include "../fx/ak_collada_fx_sampler.h"

typedef struct {
  const char *key;
  AkValueType val;
  int         m;
  int         n;
  int         size;
} ak_value_pair;

static
int
valuePairCmp(const void *, const void *);

static
int
valuePairCmp2(const void *, const void *);

static ak_value_pair valueMap[] = {
  {_s_dae_string,   AK_VALUE_STRING,   1, 1, sizeof(char *)},
  {_s_dae_bool,     AK_VALUE_BOOL,     1, 1, sizeof(bool)},
  {_s_dae_bool2,    AK_VALUE_BOOL2,    1, 2, sizeof(bool[2])},
  {_s_dae_bool3,    AK_VALUE_BOOL3,    1, 3, sizeof(bool[3])},
  {_s_dae_bool4,    AK_VALUE_BOOL4,    1, 4, sizeof(bool[4])},
  {_s_dae_int,      AK_VALUE_INT,      1, 1, sizeof(int)},
  {_s_dae_int2,     AK_VALUE_INT2,     1, 2, sizeof(int[2])},
  {_s_dae_int3,     AK_VALUE_INT3,     1, 3, sizeof(int[3])},
  {_s_dae_int4,     AK_VALUE_INT4,     1, 4, sizeof(int[4])},
  {_s_dae_float,    AK_VALUE_FLOAT,    1, 1, sizeof(float)},
  {_s_dae_float2,   AK_VALUE_FLOAT2,   1, 2, sizeof(float[2])},
  {_s_dae_float3,   AK_VALUE_FLOAT3,   1, 3, sizeof(float[3])},
  {_s_dae_float4,   AK_VALUE_FLOAT4,   1, 4, sizeof(float[4])},
  {_s_dae_float2x2, AK_VALUE_FLOAT2x2, 2, 2, sizeof(float[2][2])},
  {_s_dae_float3x3, AK_VALUE_FLOAT3x3, 3, 3, sizeof(float[3][3])},
  {_s_dae_float4x4, AK_VALUE_FLOAT4x4, 4, 4, sizeof(float[4][4])},

  /* samplers */
  {_s_dae_sampler1d,     AK_VALUE_SAMPLER1D,     1, 1, sizeof(AkSampler1D)},
  {_s_dae_sampler2d,     AK_VALUE_SAMPLER2D,     1, 1, sizeof(AkSampler2D)},
  {_s_dae_sampler3d,     AK_VALUE_SAMPLER3D,     1, 1, sizeof(AkSampler3D)},
  {_s_dae_sampler_cube,  AK_VALUE_SAMPLER_CUBE,  1, 1, sizeof(AkSamplerCUBE)},
  {_s_dae_sampler_rect,  AK_VALUE_SAMPLER_RECT,  1, 1, sizeof(AkSamplerRECT)},
  {_s_dae_sampler_depth, AK_VALUE_SAMPLER_DEPTH, 1, 1, sizeof(AkSamplerDEPTH)}
};

static size_t valueMapLen = 0;

void _assetkit_hide
ak_dae_dataType(const char *typeName,
                AkTypeDesc *type) {
  ak_value_pair *found;

  if (valueMapLen == 0) {
    valueMapLen = AK_ARRAY_LEN(valueMap);
    qsort(valueMap,
          valueMapLen,
          sizeof(valueMap[0]),
          valuePairCmp);
  }

  found = bsearch(typeName,
                  valueMap,
                  valueMapLen,
                  sizeof(valueMap[0]),
                  valuePairCmp2);

  if (!found) {
    type->type     = AK_VALUE_UNKNOWN;
    type->typeName = NULL;
    type->size     = 0;
    return;
  }

  type->size     = found->size;
  type->type     = found->val;
  type->typeName = found->key;
}

AkResult _assetkit_hide
ak_dae_value(AkXmlState * __restrict xst,
             void       * __restrict memParent,
             AkValue   ** __restrict dest) {
  AkValue       *val;
  ak_value_pair *found;
  AkXmlElmState  xest;

  if (valueMapLen == 0) {
    valueMapLen = AK_ARRAY_LEN(valueMap);
    qsort(valueMap,
          valueMapLen,
          sizeof(valueMap[0]),
          valuePairCmp);
  }

  found = bsearch(xst->nodeName,
                  valueMap,
                  valueMapLen,
                  sizeof(valueMap[0]),
                  valuePairCmp2);

  if (!found) {
    *dest = NULL;
    return AK_EFOUND;
  }

  val = ak_heap_calloc(xst->heap, memParent, sizeof(*val));
  val->type.size     = found->size;
  val->type.type     = found->val;
  val->type.typeName = found->key;

  ak_xest_init(xest, found->key)

  switch (found->val) {
    case AK_VALUE_STRING:
      val->value = ak_xml_val(xst, NULL);
      break;
    case AK_VALUE_BOOL:
    case AK_VALUE_BOOL2:
    case AK_VALUE_BOOL3:
    case AK_VALUE_BOOL4:{
      AkBool *boolVal;
      char   *nodeVal;

      nodeVal = ak_xml_rawcval(xst);
      boolVal = ak_heap_calloc(xst->heap,
                               memParent,
                               sizeof(*boolVal) * found->m * found->n);
      ak_strtomb(&nodeVal, boolVal, found->m, found->n);

      val->value = boolVal;
      break;
    }
    case AK_VALUE_INT:
    case AK_VALUE_INT2:
    case AK_VALUE_INT3:
    case AK_VALUE_INT4:{
      AkInt *intVal;
      char  *nodeVal;

      nodeVal = ak_xml_rawcval(xst);
      intVal = ak_heap_calloc(xst->heap,
                              memParent,
                              sizeof(*intVal) * found->m * found->n);
      ak_strtomi(&nodeVal, intVal, found->m, found->n);

      val->value = intVal;
      break;
    }
    case AK_VALUE_FLOAT:
    case AK_VALUE_FLOAT2:
    case AK_VALUE_FLOAT3:
    case AK_VALUE_FLOAT4:
    case AK_VALUE_FLOAT2x2:
    case AK_VALUE_FLOAT3x3:
    case AK_VALUE_FLOAT4x4:{
      AkFloat *floatVal;
      char    *nodeVal;

      nodeVal  = ak_xml_rawcval(xst);
      floatVal = ak_heap_calloc(xst->heap,
                                memParent,
                                sizeof(*floatVal) * found->m * found->n);
      ak_strtomf(&nodeVal, floatVal, found->m, found->n);

      val->value = floatVal;
      break;
    }
    case AK_VALUE_SAMPLER1D:
    case AK_VALUE_SAMPLER2D:
    case AK_VALUE_SAMPLER3D:
    case AK_VALUE_SAMPLER_CUBE:
    case AK_VALUE_SAMPLER_RECT:
    case AK_VALUE_SAMPLER_DEPTH: {
      AkFxSamplerCommon *sampler;
      AkResult           ret;

      sampler = NULL;
      ret = ak_dae_fxSampler(xst,
                             memParent,
                             found->key,
                             &sampler);

      if (ret == AK_OK)
        val->value = sampler;

      break;
    }
    default:
      break;
  }

  /* end element */
  ak_xml_end(&xest);

  *dest = val;

  return AK_OK;
}

static
int
valuePairCmp(const void * a, const void * b) {
  const ak_value_pair * _a = a;
  const ak_value_pair * _b = b;

  return strcmp(_a->key, _b->key);
}

static
int
valuePairCmp2(const void * a, const void * b) {
  const char * _a = a;
  const ak_value_pair * _b = b;
  
  return strcmp(_a, _b->key);
}
