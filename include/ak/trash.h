/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_trash_h
#define ak_trash_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

AK_EXPORT
void ak_trash_add(void *mem);

AK_EXPORT
void
ak_trash_empty(void);

#ifdef __cplusplus
}
#endif
#endif /* ak_trash_h */
