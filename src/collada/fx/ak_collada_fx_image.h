/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__ak_collada_fx_image__h_
#define __libassetkit__ak_collada_fx_image__h_

#include "../../../include/assetkit.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

AkResult _assetkit_hide
ak_dae_fxImage(void * __restrict memParent,
                xmlTextReaderPtr reader,
                ak_image ** __restrict dest);

AkResult _assetkit_hide
ak_dae_fxImageInstance(void * __restrict memParent,
                        xmlTextReaderPtr reader,
                        ak_image_instance ** __restrict dest);

#endif /* __libassetkit__ak_collada_fx_image__h_ */
