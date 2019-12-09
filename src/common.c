/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "common.h"
#include <json/json.h>


int _assetkit_hide
ak_enumpair_cmp(const void * a, const void * b) {
  return strcmp(((const ak_enumpair *)a)->key,
                ((const ak_enumpair *)b)->key);
}

int _assetkit_hide
ak_enumpair_cmp2(const void * a, const void * b) {
  return strcmp(a, ((const ak_enumpair *)b)->key);
}

int _assetkit_hide
ak_enumpair_json_val_cmp(const void * a, const void * b) {
  const char *s;
  
  if (!(s = json_string(a)))
    return -1;

  return strncmp(s, ((const ak_enumpair *)b)->key, ((json_t *)a)->valsize);
}
