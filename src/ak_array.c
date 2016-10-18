/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_array.h"
#include <string.h>

#define AK__TMP_ARRAY_INCREMENT 32

AkResult _assetkit_hide
ak_strtod_array(AkHeap * __restrict heap,
                void * __restrict memParent,
                char * __restrict stringRep,
                AkDoubleArray ** __restrict array) {
  AkDoubleArray  *doubleArray;
  AkDouble       *tmpArray;
  char           *tok;
  AkUInt64        tmpCount;
  AkUInt64        arrayIndex;
  size_t          arraySize;

  tmpCount  = AK__TMP_ARRAY_INCREMENT;
  arrayIndex = 0;

  tmpArray = ak_heap_alloc(heap,
                           memParent,
                           sizeof(AkDouble) * tmpCount,
                           false);

  tok = strtok(stringRep, " \t\r\n");
  while (tok) {
    *(tmpArray + arrayIndex++) = strtod(tok, NULL);

    tok = strtok(NULL, " \t\r\n");

    if (tok && arrayIndex == tmpCount) {
      tmpCount += AK__TMP_ARRAY_INCREMENT;
      tmpArray = ak_realloc(memParent,
                            tmpArray,
                            sizeof(AkDouble) * tmpCount);
    }
  }

  arraySize = sizeof(AkDouble) * arrayIndex;
  doubleArray = ak_heap_alloc(heap,
                              memParent,
                              sizeof(*doubleArray) + arraySize,
                              false);

  doubleArray->count = arrayIndex;
  memmove(doubleArray->items, tmpArray, arraySize);

  ak_free(tmpArray);

  *array = doubleArray;

  return AK_OK;
}

AkResult _assetkit_hide
ak_strtod_arrayL(AkHeap * __restrict heap,
                 void * __restrict memParent,
                 char * __restrict stringRep,
                 AkDoubleArrayL ** __restrict array) {
  AkDoubleArrayL *doubleArray;
  AkDouble       *tmpArray;
  char           *tok;
  AkUInt64        tmpCount;
  AkUInt64        arrayIndex;
  size_t          arraySize;

  tmpCount  = AK__TMP_ARRAY_INCREMENT;
  arrayIndex = 0;

  tmpArray = ak_heap_alloc(heap,
                           memParent,
                           sizeof(AkDouble) * tmpCount,
                           false);

  tok = strtok(stringRep, " \t\r\n");
  while (tok) {
    *(tmpArray + arrayIndex++) = strtod(tok, NULL);

    tok = strtok(NULL, " \t\r\n");

    if (tok && arrayIndex == tmpCount) {
      tmpCount += AK__TMP_ARRAY_INCREMENT;
      tmpArray = ak_realloc(memParent,
                            tmpArray,
                            sizeof(AkDouble) * tmpCount);
    }
  }

  arraySize = sizeof(AkDouble) * arrayIndex;
  doubleArray = ak_heap_alloc(heap,
                              memParent,
                              sizeof(*doubleArray) + arraySize,
                              false);

  doubleArray->count = arrayIndex;
  memmove(doubleArray->items, tmpArray, arraySize);

  ak_free(tmpArray);

  *array = doubleArray;

  return AK_OK;
}

AkResult _assetkit_hide
ak_strtoi_array(AkHeap * __restrict heap,
                void * __restrict memParent,
                char * stringRep,
                AkIntArray ** __restrict array) {
  AkIntArray     *intArray;
  AkInt          *tmpArray;
  char           *tok;
  AkUInt64        tmpCount;
  AkUInt64        arrayIndex;
  size_t          arraySize;

  tmpCount  = AK__TMP_ARRAY_INCREMENT;
  arrayIndex = 0;

  tmpArray = ak_heap_alloc(heap,
                           memParent,
                           sizeof(AkInt) * tmpCount,
                           false);

  tok = strtok(stringRep, " \t\r\n");
  while (tok) {
    tmpArray[arrayIndex++] = (AkInt)strtol(tok, NULL, 10);

    tok = strtok(NULL, " \t\r\n");

    if (tok && arrayIndex == tmpCount) {
      tmpCount += AK__TMP_ARRAY_INCREMENT;
      tmpArray = ak_realloc(memParent,
                            tmpArray,
                            sizeof(AkInt) * tmpCount);
    }
  }

  arraySize = sizeof(AkInt) * arrayIndex;
  intArray = ak_heap_alloc(heap,
                           memParent,
                           sizeof(*intArray) + arraySize,
                           false);

  intArray->count = arrayIndex;
  memmove(intArray->items, tmpArray, arraySize);

  ak_free(tmpArray);

  *array = intArray;

  return AK_OK;
}

AkResult _assetkit_hide
ak_strtoui_array(AkHeap * __restrict heap,
                 void * __restrict memParent,
                 char * stringRep,
                 AkUIntArray ** __restrict array) {
  AkUIntArray    *intArray;
  AkUInt         *tmpArray;
  char           *tok;
  AkUInt64        tmpCount;
  AkUInt64        arrayIndex;
  size_t          arraySize;

  tmpCount  = AK__TMP_ARRAY_INCREMENT;
  arrayIndex = 0;

  tmpArray = ak_heap_alloc(heap,
                           memParent,
                           sizeof(AkUInt) * tmpCount,
                           false);

  tok = strtok(stringRep, " \t\r\n");
  while (tok) {
    tmpArray[arrayIndex++] = (AkUInt)strtoul(tok, NULL, 10);

    tok = strtok(NULL, " \t\r\n");

    if (tok && arrayIndex == tmpCount) {
      tmpCount += AK__TMP_ARRAY_INCREMENT;
      tmpArray = ak_realloc(memParent,
                            tmpArray,
                            sizeof(AkUInt) * tmpCount);
    }
  }

  arraySize = sizeof(AkUInt) * arrayIndex;
  intArray = ak_heap_alloc(heap,
                           memParent,
                           sizeof(*intArray) + arraySize,
                           false);

  intArray->count = arrayIndex;
  memmove(intArray->items, tmpArray, arraySize);

  ak_free(tmpArray);

  *array = intArray;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_strtostr_array(AkHeap * __restrict heap,
                  void * __restrict memParent,
                  char * stringRep,
                  char separator,
                  AkStringArray ** __restrict array) {
  AkStringArray  *stringArray;
  char           *pData;
  char           *tok;
  char            separatorStr[4];
  AkUInt64        arrayIndex;
  AkUInt64        itemCount;
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
                              sizeof(*stringArray) + arraySize,
                              false);

  pData = ak_heap_alloc(heap,
                        stringArray,
                        arrayDataSize,
                        false);

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

AkResult _assetkit_hide
ak_strtostr_arrayL(AkHeap * __restrict heap,
                   void * __restrict memParent,
                   char * stringRep,
                   char separator,
                   AkStringArrayL ** __restrict array) {
  AkStringArrayL  *stringArray;
  char           *pData;
  char           *tok;
  char            separatorStr[4];
  AkUInt64        arrayIndex;
  AkUInt64        itemCount;
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
                              sizeof(*stringArray) + arraySize,
                              false);

  pData = ak_heap_alloc(heap,
                        stringArray,
                        arrayDataSize,
                        false);

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
