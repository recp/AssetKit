/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__memory__h_
#define __libassetio__memory__h_

#include <stddef.h>
#include <sys/types.h>

void   aio_init();
void   aio_cleanup();
void * aio_malloc(size_t);
void * aio_calloc(size_t, size_t);
char * aio_strdup(const char * __restrict);
void * aio_realloc(void * __restrict, size_t);
void	 aio_free(void * __restrict);

#endif /* __libassetio__memory__h_ */
