/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef aio_types_h
#define aio_types_h

typedef const char * aio_str;
typedef char       * aio_mutstr;

#define aio_true  0x1
#define aio_false 0x0

#define AIO_IS_EQ_CASE(s1, s2) strcasecmp(s1, s2) == 0

#endif /* aio_types_h */
