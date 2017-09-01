/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__aioutils__h_
#define __libassetkit__aioutils__h_

#include "../include/assetkit.h"
#include "../include/ak-util.h"

AkResult
ak_readfile(const char * __restrict file,
            const char * __restrict modes,
            void      ** __restrict dest,
            size_t     * __restrict size);
time_t
ak_parse_date(const char * __restrict input,
              const char ** __restrict ret);

#endif /* __libassetkit__aioutils__h_ */
