/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_array_h
#define ak_array_h

#include "../include/assetkit.h"

AkResult _assetkit_hide
ak_strtod_array(void * __restrict memParent,
                char * __restrict stringRep,
                AkDoubleArray ** __restrict array);

AkResult _assetkit_hide
ak_strtod_arrayL(void * __restrict memParent,
                 char * __restrict stringRep,
                 AkDoubleArrayL ** __restrict array);

AkResult _assetkit_hide
ak_strtoi_array(void * __restrict memParent,
                char * stringRep,
                AkIntArray ** __restrict array);

AkResult _assetkit_hide
ak_strtostr_array(void * __restrict memParent,
                  char * stringRep,
                  char separator,
                  AkStringArray ** __restrict array);

AkResult _assetkit_hide
ak_strtostr_arrayL(void * __restrict memParent,
                   char * stringRep,
                   char separator,
                   AkStringArrayL ** __restrict array);

#endif /* ak_array_h */
