/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

/*
 * TODO: maybe there is better way to implement sid addressing?
 */

#ifndef ak_sid_h
#define ak_sid_h

AK_EXPORT
const char *
ak_sid_get(void *memnode);

AK_EXPORT
const char *
ak_sid_geta(void *memnode,
            void *memptr);

AK_EXPORT
void
ak_sid_set(void       *memnode,
           const char * __restrict sid);

AK_EXPORT
void
ak_sid_seta(void       *memnode,
            void       *memptr,
            const char * __restrict sid);

#endif /* ak_sid_h */