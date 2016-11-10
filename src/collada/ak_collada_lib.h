/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_collada_lib_h
#define ak_collada_lib_h

#include "ak_collada_common.h"

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
} AkLibChldDesc;

void _assetkit_hide
ak_dae_lib(AkXmlState    * __restrict xst,
           AkLibChldDesc * __restrict lc);

#endif /* ak_collada_lib_h */
