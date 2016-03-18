/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_float_or_param___h_
#define __libassetkit__ak_collada_float_or_param___h_

#include "../../../include/assetkit.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetkit_hide
ak_dae_floatOrParam(void * __restrict memParent,
                     xmlTextReaderPtr reader,
                     const char * elm,
                     ak_fx_float_or_param ** __restrict dest);

#endif /* __libassetkit__ak_collada_float_or_param___h_ */
