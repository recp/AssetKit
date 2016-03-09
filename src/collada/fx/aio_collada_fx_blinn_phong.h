/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_blinn_phong__h_
#define __libassetio__aio_collada_blinn_phong__h_

#include "../../../include/assetio.h"

typedef union {
  aio_blinn blinn;
  aio_phong phong;
} aio_blinn_phong;

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_blinn_phong(xmlTextReaderPtr __restrict reader,
                    const char * elm,
                    aio_blinn_phong ** __restrict dest);

#endif /* __libassetio__aio_collada_blinn_phong__h_ */
