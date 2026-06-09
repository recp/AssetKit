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

#ifndef ak_json_h
#define ak_json_h

#include <string.h>
#include <stdlib.h>

/* JSON parser */
#include <json/json.h>

AK_INLINE
long double
ak_json_pow10l(int exp) {
  static const long double pow10[] = {
    1.0e1L, 1.0e2L, 1.0e4L, 1.0e8L, 1.0e16L,
    1.0e32L, 1.0e64L, 1.0e128L, 1.0e256L
  };
  long double scale;
  unsigned int e;
  unsigned int i;
  bool neg;

  if (exp == 0)
    return 1.0L;

  neg   = exp < 0;
  e     = neg ? (unsigned int)-exp : (unsigned int)exp;
  scale = 1.0L;
  for (i = 0; e && i < AK_ARRAY_LEN(pow10); i++, e >>= 1) {
    if (e & 1u)
      scale *= pow10[i];
  }

  return neg ? 1.0L / scale : scale;
}

AK_INLINE
float
ak_json_parse_float(const char * __restrict p,
                    const char * __restrict end,
                    float                    defaultValue) {
  uint64_t sig;
  int      exp10;
  int      explicitExp;
  bool     neg;
  bool     expNeg;
  bool     afterDecimal;
  bool     hasDigits;
  bool     seenSig;
  uint32_t sigDigits;

  if (!p || p >= end)
    return defaultValue;

  neg = *p == '-';
  if (neg || *p == '+')
    p++;

  sig          = 0;
  exp10        = 0;
  afterDecimal = false;
  hasDigits    = false;
  seenSig      = false;
  sigDigits    = 0;

  while (p < end) {
    unsigned int digit;

    if (*p == '.' && !afterDecimal) {
      afterDecimal = true;
      p++;
      continue;
    }

    if (*p < '0' || *p > '9')
      break;

    digit     = (unsigned int)(*p++ - '0');
    hasDigits = true;

    if (!seenSig && digit == 0) {
      if (afterDecimal)
        exp10--;
      continue;
    }

    seenSig = true;
    if (sigDigits < 19) {
      sig = sig * 10u + digit;
      sigDigits++;
      if (afterDecimal)
        exp10--;
    } else if (!afterDecimal) {
      exp10++;
    }
  }

  if (!hasDigits)
    return defaultValue;

  if (p < end && (*p == 'e' || *p == 'E')) {
    p++;
    expNeg = p < end && *p == '-';
    if (expNeg || (p < end && *p == '+'))
      p++;

    explicitExp = 0;
    while (p < end && *p >= '0' && *p <= '9') {
      if (explicitExp < 10000)
        explicitExp = explicitExp * 10 + (*p - '0');
      p++;
    }

    exp10 += expNeg ? -explicitExp : explicitExp;
  }

  if (sig == 0)
    return neg ? -0.0f : 0.0f;

  return (float)((neg ? -1.0L : 1.0L)
                 * (long double)sig
                 * ak_json_pow10l(exp10));
}

AK_INLINE
float
ak_json_float(const json_t * __restrict object, float defaultValue) {
  const char *end;

  return ak_json_parse_float(json__num_begin(object, &end), end, defaultValue);
}

#define json_float ak_json_float

AK_INLINE
void
ak_json_array_float(float        * __restrict dest,
                    const json_t * __restrict object,
                    float                     defaultValue,
                    int                       desiredCount,
                    bool                      sourceIsReversed) {
  json_array_t *arr;
  json_t       *item;
  int           count;
  int           i;

  if (!(arr = json_array(object))) {
    for (i = 0; i < desiredCount; i++)
      dest[i] = defaultValue;
    return;
  }

  count = arr->count;
  item  = arr->base.value;

  if (desiredCount > 0 && count > desiredCount)
    count = desiredCount;

  if (sourceIsReversed) {
    if (desiredCount > count) {
      for (i = desiredCount - 1; i >= 0; i--)
        dest[i] = defaultValue;
    }

    while (item) {
      if (count <= 0)
        break;

      dest[--count] = ak_json_float(item, defaultValue);
      item          = item->next;
    }

    while (count)
      dest[--count] = defaultValue;
  } else {
    i = 0;
    while (item) {
      if (i >= count)
        break;

      dest[i++] = ak_json_float(item, defaultValue);
      item      = item->next;
    }

    if (i != desiredCount) {
      for (; i < desiredCount; i++)
        dest[i] = defaultValue;
    }
  }
}

#define json_array_float ak_json_array_float

AK_INLINE
char *
json_strdup(const json_t * __restrict jsonObject,
            AkHeap       * __restrict heap,
            void         * __restrict parent) {
  return ak_heap_strndup(heap,
                         parent,
                         json_string(jsonObject),
                         jsonObject->valsize);
}

AK_INLINE
void
json_array_set(void         * __restrict p,
               AkTypeId                  typeId,
               int                       index,
               const json_t * __restrict json) {
  switch (typeId) {
    case AKT_FLOAT:
      ((float *)p)[index] = json_float(json, 0.0f);
      break;
    case AKT_INT:
      ((int32_t *)p)[index] = json_int32(json, 0);
      break;
    case AKT_UINT:
      ((int32_t *)p)[index] = json_uint32(json, 0);
      break;
    case AKT_SHORT:
      ((int16_t *)p)[index] = json_int32(json, 0);
      break;
    case AKT_USHORT:
      ((uint16_t *)p)[index] = json_uint32(json, 0);
      break;
    case AKT_BYTE:
      ((char *)p)[index] = json_int32(json, 0);
      break;
    case AKT_UBYTE:
      ((unsigned char *)p)[index] = json_uint32(json, 0);
      break;
    default:
      break;
  }
}

#endif /* ak_json_h */
