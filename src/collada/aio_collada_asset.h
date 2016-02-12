/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_asset__h_
#define __libassetio__aio_collada_asset__h_

#include "../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_assetInf(xmlTextReaderPtr __restrict reader,
                 aio_assetinf ** __restrict dest);

#define _AIO_ASSET_LOAD_TO(NODE, ASSET_INF)

#endif /* __libassetio__aio_collada_asset__h_ */
