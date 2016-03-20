/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_blinn_phong__h_
#define __libassetkit__ak_collada_blinn_phong__h_

#include "../../../include/assetkit.h"
#include "../ak_collada_common.h"

typedef union {
  ak_blinn blinn;
  ak_phong phong;
} ak_blinn_phong;

AkResult _assetkit_hide
ak_dae_blinn_phong(void * __restrict memParent,
                    xmlTextReaderPtr reader,
                    const char * elm,
                    ak_blinn_phong ** __restrict dest);

#endif /* __libassetkit__ak_collada_blinn_phong__h_ */
