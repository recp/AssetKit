/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_lib_h
#define dae_lib_h

#include "dae_common.h"

typedef AkResult (*libChldFn)(AkXmlState * __restrict xst,
                              void * __restrict memParent,
                              void ** __restrict  dest);

typedef struct AkLibChldDesc {
  AkLibItem  * __restrict lastItem;
  const char * __restrict libname;
  const char * __restrict name;
  libChldFn   chldFn;
  int         libOffset;
  int         nextOfset;
  int         prevOfset;
} AkLibChldDesc;

void _assetkit_hide
dae_lib(AkXmlState    * __restrict xst,
        AkLibChldDesc * __restrict lc);

#endif /* dae_lib_h */
