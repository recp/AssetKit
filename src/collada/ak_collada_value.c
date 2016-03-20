/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_value.h"

typedef struct {
  const char * key;
  AkValueType val;
  int m;
  int n;
} ak_value_pair;

static
int _assetkit_hide
valuePairCmp(const void *, const void *);

static
int _assetkit_hide
valuePairCmp2(const void *, const void *);

static ak_value_pair valueMap[] = {
  {_s_dae_string,   AK_VALUE_TYPE_STRING,   1, 1},
  {_s_dae_bool,     AK_VALUE_TYPE_BOOL,     1, 1},
  {_s_dae_bool2,    AK_VALUE_TYPE_BOOL2,    1, 2},
  {_s_dae_bool3,    AK_VALUE_TYPE_BOOL3,    1, 3},
  {_s_dae_bool4,    AK_VALUE_TYPE_BOOL4,    1, 4},
  {_s_dae_int,      AK_VALUE_TYPE_INT,      1, 1},
  {_s_dae_int2,     AK_VALUE_TYPE_INT2,     1, 2},
  {_s_dae_int3,     AK_VALUE_TYPE_INT3,     1, 3},
  {_s_dae_int4,     AK_VALUE_TYPE_INT4,     1, 4},
  {_s_dae_float,    AK_VALUE_TYPE_FLOAT,    1, 1},
  {_s_dae_float2,   AK_VALUE_TYPE_FLOAT2,   1, 2},
  {_s_dae_float3,   AK_VALUE_TYPE_FLOAT3,   1, 3},
  {_s_dae_float4,   AK_VALUE_TYPE_FLOAT4,   1, 4},
  {_s_dae_float2x2, AK_VALUE_TYPE_FLOAT2x2, 2, 2},
  {_s_dae_float3x3, AK_VALUE_TYPE_FLOAT3x3, 3, 3},
  {_s_dae_float4x4, AK_VALUE_TYPE_FLOAT4x4, 4, 4}
};

static size_t valueMapLen = 0;

AkEnum _assetkit_hide
ak_dae_valueType(const char * typeName) {
  ak_value_pair *found;

  if (valueMapLen == 0) {
    valueMapLen = ak_ARRAY_LEN(valueMap);
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

  if (!found)
    return AK_VALUE_TYPE_UNKNOWN;

  return found->val;
}

AkResult _assetkit_hide
ak_dae_value(void * __restrict memParent,
              xmlTextReaderPtr reader,
              void ** __restrict dest,
              AkValueType * __restrict val_type) {
  ak_value_pair *found;
  char           *nodeVal;
  const xmlChar  *nodeName;
  int nodeType;
  int nodeRet;

  nodeName = xmlTextReaderConstName(reader);

  if (valueMapLen == 0) {
    valueMapLen = ak_ARRAY_LEN(valueMap);
    qsort(valueMap,
          valueMapLen,
          sizeof(valueMap[0]),
          valuePairCmp);
  }

  found = bsearch(nodeName,
                  valueMap,
                  valueMapLen,
                  sizeof(valueMap[0]),
                  valuePairCmp2);

  if (!found) {
    *val_type = AK_VALUE_TYPE_UNKNOWN;
    return AK_ERR;
  }
  
  *val_type = found->val;

  _xml_readText(NULL, nodeVal);

  switch (found->val) {
    case AK_VALUE_TYPE_STRING:
      
      *dest = nodeVal;
      break;
    case AK_VALUE_TYPE_BOOL:
    case AK_VALUE_TYPE_BOOL2:
    case AK_VALUE_TYPE_BOOL3:
    case AK_VALUE_TYPE_BOOL4:{
      ak_bool * val;

      val = ak_calloc(memParent, sizeof(*val) * found->m * found->n, 1);
      ak_strtomb(&nodeVal, val, found->m, found->n);

      *dest = val;
      break;
    }
    case AK_VALUE_TYPE_INT:
    case AK_VALUE_TYPE_INT2:
    case AK_VALUE_TYPE_INT3:
    case AK_VALUE_TYPE_INT4:{
      ak_int * val;

      val = ak_calloc(memParent, sizeof(*val) * found->m * found->n, 1);
      ak_strtomi(&nodeVal, val, found->m, found->n);

      *dest = val;
      break;
    }
    case AK_VALUE_TYPE_FLOAT:
    case AK_VALUE_TYPE_FLOAT2:
    case AK_VALUE_TYPE_FLOAT3:
    case AK_VALUE_TYPE_FLOAT4:
    case AK_VALUE_TYPE_FLOAT2x2:
    case AK_VALUE_TYPE_FLOAT3x3:
    case AK_VALUE_TYPE_FLOAT4x4:{
      ak_float * val;

      val = ak_calloc(memParent, sizeof(*val) * found->m * found->n, 1);
      ak_strtomf(&nodeVal, val, found->m, found->n);

      *dest = val;
      break;
    }
    default:
      break;
  }

  if (nodeVal)
    ak_free(nodeVal);

  /* end element */
  _xml_endElement;

  return AK_OK;
}

static
int _assetkit_hide
valuePairCmp(const void * a, const void * b) {
  const ak_value_pair * _a = a;
  const ak_value_pair * _b = b;

  return strcmp(_a->key, _b->key);
}

static
int _assetkit_hide
valuePairCmp2(const void * a, const void * b) {
  const char * _a = a;
  const ak_value_pair * _b = b;

  return strcmp(_a, _b->key);
}