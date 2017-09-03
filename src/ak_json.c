/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_json.h"

const char*
json_cstr(json_t *jsn, const char *key) {
  json_t *jval;

  jval = json_object_get(jsn, key);
  if (!jval)
    return NULL;

  return json_string_value(jval);
}

int32_t
json_int32(json_t *jsn, const char *key) {
  json_t *jval;

  jval = json_object_get(jsn, key);
  if (!jval)
    return 0;

  return (int32_t)json_integer_value(jval);
}

int64_t
json_int64(json_t *jsn, const char *key) {
  json_t *jval;

  jval = json_object_get(jsn, key);
  if (!jval)
    return 0;

  return json_integer_value(jval);
}

