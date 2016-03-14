/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_fx_lambert__h_
#define __libassetio__aio_collada_fx_lambert__h_

#include "../../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_fxLambert(void * __restrict memParent,
                  xmlTextReaderPtr reader,
                  aio_lambert ** __restrict dest);

#endif /* __libassetio__aio_collada_fx_lambert__h_ */
