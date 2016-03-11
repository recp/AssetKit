/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_fx_program_h_
#define __libassetio__aio_collada_fx_program_h_

#include "../../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_fxProg(xmlTextReaderPtr __restrict reader,
               aio_program ** __restrict dest);

#endif /* __libassetio__aio_collada_fx_program_h_ */
