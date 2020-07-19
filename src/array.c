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

AkResult AK_HIDE
ak_strtostr_array(AkHeap         * __restrict heap,
                  void           * __restrict memParent,
                  char                       *content,
                  char                        separator,
                  AkStringArray ** __restrict array) {
  AkStringArray  *stringArray;
  char           *pData;
  char           *tok;
  char            separatorStr[4];
  size_t          arrayIndex;
  size_t          itemCount;
  size_t          arraySize;
  size_t          arrayDataSize;

  arrayIndex = 0;
  itemCount  = 0;

  separatorStr[0] = separator;
  separatorStr[1] = '\n';
  separatorStr[2] = '\r';
  separatorStr[3] = '\t';

  pData = content;
  while (*pData != '\0'
         && *++pData == separator)
    itemCount++;

  /*
   |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|

   the last one is pointer to all data
   */
  arraySize = sizeof(char *) * (itemCount + 1);
  arrayDataSize = strlen(content) + itemCount /* NULL */;

  stringArray = ak_heap_alloc(heap,
                              memParent,
                              sizeof(*stringArray) + arraySize);

  pData = ak_heap_alloc(heap,
                        stringArray,
                        arrayDataSize);

  stringArray->count = itemCount;
  stringArray->items[itemCount] = pData;

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

AkResult AK_HIDE
ak_strtostr_arrayL(AkHeap * __restrict heap,
                   void * __restrict memParent,
                   char * stringRep,
                   char separator,
                   AkStringArrayL ** __restrict array) {
  AkStringArrayL *stringArray;
  char           *pData;
  char           *tok;
  char            separatorStr[4];
  size_t          arrayIndex;
  size_t          itemCount;
  size_t          arraySize;
  size_t          arrayDataSize;

  arrayIndex = 0;
  itemCount  = 0;

  separatorStr[0] = separator;
  separatorStr[1] = '\n';
  separatorStr[2] = '\r';
  separatorStr[3] = '\t';

  pData = stringRep;
  while (*pData != '\0'
         && *++pData == separator)
    itemCount++;

  /*
   |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|

   the last one is pointer to all data
   */
  arraySize = sizeof(char *) * (itemCount + 1);
  arrayDataSize = strlen(stringRep) + itemCount /* NULL */;

  stringArray = ak_heap_alloc(heap,
                              memParent,
                              sizeof(*stringArray) + arraySize);

  pData = ak_heap_alloc(heap,
                        stringArray,
                        arrayDataSize);

  stringArray->count = itemCount;
  stringArray->items[itemCount] = pData;

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
