/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_fx_lambert__h_
#define __libassetkit__ak_collada_fx_lambert__h_

#include "../../../include/assetkit.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

AkResult _assetkit_hide
ak_dae_fxLambert(void * __restrict memParent,
                  xmlTextReaderPtr reader,
                  ak_lambert ** __restrict dest);

#endif /* __libassetkit__ak_collada_fx_lambert__h_ */
