/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_fx_sampler_h_
#define __libassetio__aio_collada_fx_sampler_h_

#include "../../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_fxSampler(xmlTextReaderPtr __restrict reader,
                  const char *elm,
                  aio_fx_sampler_common ** __restrict dest);

#endif /* __libassetio__aio_collada_fx_sampler_h_ */
