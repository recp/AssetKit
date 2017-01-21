/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include <string.h>

AK_EXPORT
int
ak_cmp_str(void * key1, void *key2) {
  return strcmp((char *)key1, (char *)key2);
}

AK_EXPORT
int
ak_cmp_ptr(void *key1, void *key2) {
  if (key1 > key2)
    return 1;
  else if (key1 < key2)
    return -1;
  return 0;
}
