/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_value__h_
#define __libassetio__aio_collada_value__h_

#include "../../include/assetio.h"

typedef struct _xmlTextReader *xmlTextReaderPtr;

int _assetio_hide
aio_dae_value(void * __restrict memParent,
              xmlTextReaderPtr __restrict reader,
              void ** __restrict dest,
              aio_value_type * __restrict val_type);

long _assetio_hide
aio_dae_valueType(const char * typeName);

#endif /* __libassetio__aio_collada_value__h_ */
