/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_technique_fx__h_
#define __libassetkit__ak_collada_technique_fx__h_

#include "../../../include/assetkit.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetkit_hide
ak_dae_techniqueFx(void * __restrict memParent,
                    xmlTextReaderPtr reader,
                    ak_technique_fx ** __restrict dest);

#endif /* __libassetkit__ak_collada_technique_fx__h_ */
