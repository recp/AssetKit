/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__assetio_string__h_
#define __libassetio__assetio_string__h_

int
aio_strtof(char ** __restrict src,
           aio_float * __restrict dest,
           unsigned long n);

int
aio_strtof_nxn(char ** __restrict src,
               aio_float * __restrict dest,
               unsigned long m,
               unsigned long n);

int
aio_strtof_s(const char * __restrict src,
             aio_float * __restrict dest,
             unsigned long n);

extern
int
aio_strtof4(char ** __restrict src,
            aio_float4 * __restrict dest);

extern
int
aio_strtof4_s(const char * __restrict src,
              aio_float4 * __restrict dest);

#endif /* __libassetio__assetio_string__h_ */
