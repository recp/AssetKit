/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */


#ifndef __libassetkit__ak_collada_fx_material__h_
#define __libassetkit__ak_collada_fx_material__h_

#include "../../../include/assetkit.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

AkResult _assetkit_hide
ak_dae_material(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 ak_material ** __restrict dest);

#endif /* __libassetkit__ak_collada_fx_material__h_ */
