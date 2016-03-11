/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetio__aio_collada_fx_image__h_
#define __libassetio__aio_collada_fx_image__h_

#include "../../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_fxImage(xmlTextReaderPtr __restrict reader,
                aio_image ** __restrict dest);

int _assetio_hide
aio_dae_fxImageInstance(xmlTextReaderPtr __restrict reader,
                        aio_image_instance ** __restrict dest);

#endif /* __libassetio__aio_collada_fx_image__h_ */
