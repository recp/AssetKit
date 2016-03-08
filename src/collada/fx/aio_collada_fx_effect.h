/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetio__aio_collada_fx_effect__h_
#define __libassetio__aio_collada_fx_effect__h_

#include "../../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_effect(xmlTextReaderPtr __restrict reader,
               aio_effect ** __restrict  dest);

#endif /* __libassetio__aio_collada_fx_effect__h_ */
