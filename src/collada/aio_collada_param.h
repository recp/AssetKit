/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_param__h_
#define __libassetio__aio_collada_param__h_

#include "../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_newparam(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 aio_newparam ** __restrict dest);

int _assetio_hide
aio_dae_param(void * __restrict memParent,
              xmlTextReaderPtr reader,
              aio_param_type param_type,
              aio_param ** __restrict dest);

int _assetio_hide
aio_dae_setparam(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 aio_setparam ** __restrict dest);

#endif /* __libassetio__aio_collada_param__h_ */
