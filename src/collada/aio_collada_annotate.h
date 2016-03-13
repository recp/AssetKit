/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_annotate__h_
#define __libassetio__aio_collada_annotate__h_

#include "../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_annotate(void * __restrict memParent,
                 xmlTextReaderPtr __restrict reader,
                 aio_annotate ** __restrict dest);

#endif /* __libassetio__aio_collada_annotate__h_ */
