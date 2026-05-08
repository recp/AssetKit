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

#include "array.h"
#include <string.h>

static bool
ak_strarray_is_sep(char c, char separator) {
  return c == separator || c == '\n' || c == '\r' || c == '\t';
}

static size_t
ak_strarray_count(const char *content, char separator) {
  size_t count;
  bool   inTok;

  count = 0;
  inTok = false;

  if (!content)
    return 0;

  while (*content) {
    if (ak_strarray_is_sep(*content, separator)) {
      inTok = false;
    } else if (!inTok) {
      inTok = true;
      count++;
    }
    content++;
  }

  return count;
}

AK_HIDE
AkResult
ak_strtostr_array(AkHeap         * __restrict heap,
                  void           * __restrict memParent,
                  char                       *content,
                  char                        separator,
                  AkStringArray ** __restrict array) {
  AkStringArray  *stringArray;
  char           *pData;
  char           *tok;
  char            separatorStr[5];
  size_t          arrayIndex;
  size_t          itemCount;
  size_t          arraySize;
  size_t          arrayDataSize;

  arrayIndex = 0;

  if (!content || !array)
    return AK_EINVAL;

  itemCount  = ak_strarray_count(content, separator);

  separatorStr[0] = separator;
  separatorStr[1] = '\n';
  separatorStr[2] = '\r';
  separatorStr[3] = '\t';
  separatorStr[4] = '\0';

  /*
   |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|

   the last one is pointer to all data
   */
  arraySize = sizeof(char *) * (itemCount + 1);
  arrayDataSize = strlen(content) + itemCount + 1 /* NULL */;

  stringArray = ak_heap_alloc(heap,
                              memParent,
                              sizeof(*stringArray) + arraySize);
  if (!stringArray)
    return AK_ENOMEM;

  pData = ak_heap_alloc(heap,
                        stringArray,
                        arrayDataSize);
  if (!pData)
    return AK_ENOMEM;

  stringArray->count = itemCount;
  stringArray->items[itemCount] = pData;
  pData[0] = '\0';

  tok = strtok(content, separatorStr);
  while (tok) {
    strcpy(pData, tok);
    stringArray->items[arrayIndex++] = pData;

    pData += strlen(tok);
    *pData++ = '\0';

    tok = strtok(NULL, separatorStr);
  }

  *array = stringArray;

  return AK_OK;
}

AK_HIDE
AkResult
ak_strtostr_arrayL(AkHeap * __restrict heap,
                   void * __restrict memParent,
                   char * stringRep,
                   char separator,
                   AkStringArrayL ** __restrict array) {
  AkStringArrayL *stringArray;
  char           *pData;
  char           *tok;
  char            separatorStr[5];
  size_t          arrayIndex;
  size_t          itemCount;
  size_t          arraySize;
  size_t          arrayDataSize;

  arrayIndex = 0;

  if (!stringRep || !array)
    return AK_EINVAL;

  itemCount  = ak_strarray_count(stringRep, separator);

  separatorStr[0] = separator;
  separatorStr[1] = '\n';
  separatorStr[2] = '\r';
  separatorStr[3] = '\t';
  separatorStr[4] = '\0';

  /*
   |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|

   the last one is pointer to all data
   */
  arraySize = sizeof(char *) * (itemCount + 1);
  arrayDataSize = strlen(stringRep) + itemCount + 1 /* NULL */;

  stringArray = ak_heap_alloc(heap,
                              memParent,
                              sizeof(*stringArray) + arraySize);
  if (!stringArray)
    return AK_ENOMEM;

  pData = ak_heap_alloc(heap,
                        stringArray,
                        arrayDataSize);
  if (!pData)
    return AK_ENOMEM;

  stringArray->count = itemCount;
  stringArray->items[itemCount] = pData;
  pData[0] = '\0';

  tok = strtok(stringRep, separatorStr);
  while (tok) {
    strcpy(pData, tok);
    stringArray->items[arrayIndex++] = pData;

    pData += strlen(tok);
    *pData++ = '\0';

    tok = strtok(NULL, separatorStr);
  }

  *array = stringArray;

  return AK_OK;
}
