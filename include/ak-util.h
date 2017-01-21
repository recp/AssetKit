/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_util_h
#define ak_util_h

#include "ak-common.h"

/* pre-defined compare funcs */

AK_EXPORT
int
ak_cmp_str(void *key1, void *key2);

AK_EXPORTs
int
ak_cmp_ptr(void *key1, void *key2);

#endif /* ak_util_h */
