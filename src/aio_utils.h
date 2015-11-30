/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aioutils__h_
#define __libassetio__aioutils__h_

#include <assetio.h>

int
aio_readfile(const char * __restrict file,
             const char * __restrict modes,
             char ** __restrict dest);
time_t
aio_parse_date(const char * __restrict input,
               const char ** __restrict ret);

#endif /* __libassetio__aioutils__h_ */
