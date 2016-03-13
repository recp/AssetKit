/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_technique__h_
#define __libassetio__aio_collada_technique__h_

#include "../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_technique(void * __restrict memParent,
                  xmlTextReaderPtr __restrict reader,
                  aio_technique ** __restrict dest);

int _assetio_hide
aio_dae_techniquec(void * __restrict memParent,
                   xmlTextReaderPtr __restrict reader,
                   aio_technique_common ** __restrict dest);

#endif /* __libassetio__aio_collada_technique__h_ */
