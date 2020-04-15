/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_array_h
#define ak_array_h

#include "../include/ak/assetkit.h"

AkResult _assetkit_hide
ak_strtostr_array(AkHeap         * __restrict heap,
                  void           * __restrict memParent,
                  char                       *content,
                  char                        separator,
                  AkStringArray ** __restrict array);

AkResult _assetkit_hide
ak_strtostr_arrayL(AkHeap          * __restrict heap,
                   void            * __restrict memParent,
                   char                        *content,
                   char                         separator,
                   AkStringArrayL ** __restrict array);

#endif /* ak_array_h */
