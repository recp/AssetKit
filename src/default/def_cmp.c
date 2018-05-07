/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../common.h"
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

AK_EXPORT
int
ak_cmp_i32(void *key1, void *key2) {
  int32_t a, b;

  a = *(int32_t *)key1;
  b = *(int32_t *)key2;

  return a - b;
}

AK_EXPORT
int
ak_cmp_vec3(void *key1, void *key2) {
  float *v1, *v2;

  v1  = key1;
  v2  = key2;

  if (v1[0] > v2[0])
    return 1;
  else if (v1[0] < v2[0])
    return -1;

  if (v1[1] > v2[1])
    return 1;
  else if (v1[1] < v2[1])
    return -1;

  if (v1[2] > v2[2])
    return 1;
  else if (v1[2] < v2[2])
    return -1;

  return 0;
}
