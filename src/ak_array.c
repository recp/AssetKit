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
ak_strtod_array(void * __restrict memParent,
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

  tmpArray = ak_malloc(memParent,
                        sizeof(AkDouble) * tmpCount);

  tok = strtok(stringRep, " ");
  while (tok) {
    *(tmpArray + arrayIndex++) = strtod(tok, NULL);

    tok = strtok(NULL, " ");

    if (tok && arrayIndex == tmpCount) {
      tmpCount += AK__TMP_ARRAY_INCREMENT;
      tmpArray = ak_realloc(memParent,
                             tmpArray,
                             sizeof(AkDouble) * tmpCount);
    }
  }

  arraySize = sizeof(AkDouble) * arrayIndex;
  doubleArray = ak_malloc(memParent,
                          sizeof(*doubleArray) + arraySize);

  doubleArray->count = arrayIndex;
  memmove(doubleArray->items, tmpArray, arraySize);

  ak_free(tmpArray);

  *array = doubleArray;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_strtod_arrayL(void * __restrict memParent,
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

  tmpArray = ak_malloc(memParent,
                       sizeof(AkDouble) * tmpCount);

  tok = strtok(stringRep, " ");
  while (tok) {
    *(tmpArray + arrayIndex++) = strtod(tok, NULL);

    tok = strtok(NULL, " ");

    if (tok && arrayIndex == tmpCount) {
      tmpCount += AK__TMP_ARRAY_INCREMENT;
      tmpArray = ak_realloc(memParent,
                            tmpArray,
                            sizeof(AkDouble) * tmpCount);
    }
  }

  arraySize = sizeof(AkDouble) * arrayIndex;
  doubleArray = ak_malloc(memParent,
                          sizeof(*doubleArray) + arraySize);

  doubleArray->count = arrayIndex;
  memmove(doubleArray->items, tmpArray, arraySize);

  ak_free(tmpArray);

  *array = doubleArray;

  return AK_OK;
}

AkResult _assetkit_hide
ak_strtoi_array(void * __restrict memParent,
                char * stringRep,
                AkIntArray ** __restrict array) {
  AkIntArray     *intArray;
  AkDouble       *tmpArray;
  char           *tok;
  AkUInt64        tmpCount;
  AkUInt64        arrayIndex;
  size_t          arraySize;

  tmpCount  = AK__TMP_ARRAY_INCREMENT;
  arrayIndex = 0;

  tmpArray = ak_malloc(memParent,
                        sizeof(AkInt) * tmpCount);

  tok = strtok(stringRep, " ");
  while (tok) {
    *(tmpArray + arrayIndex++) = strtod(tok, NULL);

    tok = strtok(NULL, " ");

    if (tok && arrayIndex == tmpCount) {
      tmpCount += AK__TMP_ARRAY_INCREMENT;
      tmpArray = ak_realloc(memParent,
                             tmpArray,
                             sizeof(AkInt) * tmpCount);
    }
  }

  arraySize = sizeof(AkInt) * arrayIndex;
  intArray = ak_malloc(memParent,
                       sizeof(*intArray) + arraySize);

  intArray->count = arrayIndex;
  memmove(intArray->items, tmpArray, arraySize);

  ak_free(tmpArray);

  *array = intArray;
  
  return AK_OK;
}
