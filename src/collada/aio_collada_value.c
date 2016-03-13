/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_value.h"
#include "aio_collada_common.h"

typedef struct {
  const char * key;
  aio_value_type val;
  int m;
  int n;
} aio_value_pair;

static
int _assetio_hide
valuePairCmp(const void *, const void *);

static
int _assetio_hide
valuePairCmp2(const void *, const void *);

static aio_value_pair valueMap[] = {
  {_s_dae_string,   AIO_VALUE_TYPE_STRING,   1, 1},
  {_s_dae_bool,     AIO_VALUE_TYPE_BOOL,     1, 1},
  {_s_dae_bool2,    AIO_VALUE_TYPE_BOOL2,    1, 2},
  {_s_dae_bool3,    AIO_VALUE_TYPE_BOOL3,    1, 3},
  {_s_dae_bool4,    AIO_VALUE_TYPE_BOOL4,    1, 4},
  {_s_dae_int,      AIO_VALUE_TYPE_INT,      1, 1},
  {_s_dae_int2,     AIO_VALUE_TYPE_INT2,     1, 2},
  {_s_dae_int3,     AIO_VALUE_TYPE_INT3,     1, 3},
  {_s_dae_int4,     AIO_VALUE_TYPE_INT4,     1, 4},
  {_s_dae_float,    AIO_VALUE_TYPE_FLOAT,    1, 1},
  {_s_dae_float2,   AIO_VALUE_TYPE_FLOAT2,   1, 2},
  {_s_dae_float3,   AIO_VALUE_TYPE_FLOAT3,   1, 3},
  {_s_dae_float4,   AIO_VALUE_TYPE_FLOAT4,   1, 4},
  {_s_dae_float2x2, AIO_VALUE_TYPE_FLOAT2x2, 2, 2},
  {_s_dae_float3x3, AIO_VALUE_TYPE_FLOAT3x3, 3, 3},
  {_s_dae_float4x4, AIO_VALUE_TYPE_FLOAT4x4, 4, 4}
};

static size_t valueMapLen = 0;

long _assetio_hide
aio_dae_valueType(const char * typeName) {
  aio_value_pair *found;

  if (valueMapLen == 0) {
    valueMapLen = AIO_ARRAY_LEN(valueMap);
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
    return AIO_VALUE_TYPE_UNKNOWN;

  return found->val;
}

int _assetio_hide
aio_dae_value(xmlTextReaderPtr __restrict reader,
              void ** __restrict dest,
              aio_value_type * __restrict val_type) {
  aio_value_pair *found;
  char           *nodeVal;
  const xmlChar  *nodeName;
  int nodeType;
  int nodeRet;

  nodeName = xmlTextReaderConstName(reader);

  if (valueMapLen == 0) {
    valueMapLen = AIO_ARRAY_LEN(valueMap);
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
    *val_type = AIO_VALUE_TYPE_UNKNOWN;
    return -1;
  }
  
  *val_type = found->val;

  _xml_readText(nodeVal);

  switch (found->val) {
    case AIO_VALUE_TYPE_STRING:
      
      *dest = nodeVal;
      break;
    case AIO_VALUE_TYPE_BOOL:
    case AIO_VALUE_TYPE_BOOL2:
    case AIO_VALUE_TYPE_BOOL3:
    case AIO_VALUE_TYPE_BOOL4:{
      aio_bool * val;

      val = aio_calloc(sizeof(*val) * found->m * found->n, 1);
      aio_strtomb(&nodeVal, val, found->m, found->n);

      *dest = val;
      break;
    }
    case AIO_VALUE_TYPE_INT:
    case AIO_VALUE_TYPE_INT2:
    case AIO_VALUE_TYPE_INT3:
    case AIO_VALUE_TYPE_INT4:{
      aio_int * val;

      val = aio_calloc(sizeof(*val) * found->m * found->n, 1);
      aio_strtomi(&nodeVal, val, found->m, found->n);

      *dest = val;
      break;
    }
    case AIO_VALUE_TYPE_FLOAT:
    case AIO_VALUE_TYPE_FLOAT2:
    case AIO_VALUE_TYPE_FLOAT3:
    case AIO_VALUE_TYPE_FLOAT4:
    case AIO_VALUE_TYPE_FLOAT2x2:
    case AIO_VALUE_TYPE_FLOAT3x3:
    case AIO_VALUE_TYPE_FLOAT4x4:{
      aio_float * val;

      val = aio_calloc(sizeof(*val) * found->m * found->n, 1);
      aio_strtomf(&nodeVal, val, found->m, found->n);

      *dest = val;
      break;
    }
    default:
      break;
  }

  if (nodeVal)
    aio_free(nodeVal);

  /* end element */
  _xml_endElement;

  return 0;
}

static
int _assetio_hide
valuePairCmp(const void * a, const void * b) {
  const aio_value_pair * _a = a;
  const aio_value_pair * _b = b;

  return strcmp(_a->key, _b->key);
}

static
int _assetio_hide
valuePairCmp2(const void * a, const void * b) {
  const char * _a = a;
  const aio_value_pair * _b = b;

  return strcmp(_a, _b->key);
}
