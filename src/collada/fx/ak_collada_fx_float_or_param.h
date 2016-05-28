/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_float_or_param___h_
#define __libassetkit__ak_collada_float_or_param___h_

#include "../../../include/assetkit.h"
#include "../ak_collada_common.h"

AkResult _assetkit_hide
ak_dae_floatOrParam(AkHeap * __restrict heap,
                    void * __restrict memParent,
                    xmlTextReaderPtr reader,
                    const char * elm,
                    AkFxFloatOrParam ** __restrict dest);

#endif /* __libassetkit__ak_collada_float_or_param___h_ */
