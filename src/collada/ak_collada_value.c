/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_value.h"
#include "ak_collada_common.h"

typedef struct {
  const char * key;
  ak_value_type val;
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
  {_s_dae_string,   ak_VALUE_TYPE_STRING,   1, 1},
  {_s_dae_bool,     ak_VALUE_TYPE_BOOL,     1, 1},
  {_s_dae_bool2,    ak_VALUE_TYPE_BOOL2,    1, 2},
  {_s_dae_bool3,    ak_VALUE_TYPE_BOOL3,    1, 3},
  {_s_dae_bool4,    ak_VALUE_TYPE_BOOL4,    1, 4},
  {_s_dae_int,      ak_VALUE_TYPE_INT,      1, 1},
  {_s_dae_int2,     ak_VALUE_TYPE_INT2,     1, 2},
  {_s_dae_int3,     ak_VALUE_TYPE_INT3,     1, 3},
  {_s_dae_int4,     ak_VALUE_TYPE_INT4,     1, 4},
  {_s_dae_float,    ak_VALUE_TYPE_FLOAT,    1, 1},
  {_s_dae_float2,   ak_VALUE_TYPE_FLOAT2,   1, 2},
  {_s_dae_float3,   ak_VALUE_TYPE_FLOAT3,   1, 3},
  {_s_dae_float4,   ak_VALUE_TYPE_FLOAT4,   1, 4},
  {_s_dae_float2x2, ak_VALUE_TYPE_FLOAT2x2, 2, 2},
  {_s_dae_float3x3, ak_VALUE_TYPE_FLOAT3x3, 3, 3},
  {_s_dae_float4x4, ak_VALUE_TYPE_FLOAT4x4, 4, 4}
};

static size_t valueMapLen = 0;

long _assetkit_hide
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
    return ak_VALUE_TYPE_UNKNOWN;

  return found->val;
}

int _assetkit_hide
ak_dae_value(void * __restrict memParent,
              xmlTextReaderPtr reader,
              void ** __restrict dest,
              ak_value_type * __restrict val_type) {
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
    *val_type = ak_VALUE_TYPE_UNKNOWN;
    return -1;
  }
  
  *val_type = found->val;

  _xml_readText(NULL, nodeVal);

  switch (found->val) {
    case ak_VALUE_TYPE_STRING:
      
      *dest = nodeVal;
      break;
    case ak_VALUE_TYPE_BOOL:
    case ak_VALUE_TYPE_BOOL2:
    case ak_VALUE_TYPE_BOOL3:
    case ak_VALUE_TYPE_BOOL4:{
      ak_bool * val;

      val = ak_calloc(memParent, sizeof(*val) * found->m * found->n, 1);
      ak_strtomb(&nodeVal, val, found->m, found->n);

      *dest = val;
      break;
    }
    case ak_VALUE_TYPE_INT:
    case ak_VALUE_TYPE_INT2:
    case ak_VALUE_TYPE_INT3:
    case ak_VALUE_TYPE_INT4:{
      ak_int * val;

      val = ak_calloc(memParent, sizeof(*val) * found->m * found->n, 1);
      ak_strtomi(&nodeVal, val, found->m, found->n);

      *dest = val;
      break;
    }
    case ak_VALUE_TYPE_FLOAT:
    case ak_VALUE_TYPE_FLOAT2:
    case ak_VALUE_TYPE_FLOAT3:
    case ak_VALUE_TYPE_FLOAT4:
    case ak_VALUE_TYPE_FLOAT2x2:
    case ak_VALUE_TYPE_FLOAT3x3:
    case ak_VALUE_TYPE_FLOAT4x4:{
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

  return 0;
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
