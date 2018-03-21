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
jsn_i32_def(json_t *jsn, const char *key, int32_t def) {
  json_t *jval;

  jval = json_object_get(jsn, key);
  if (!jval)
    return def;

  return (int32_t)json_integer_value(jval);
}

int32_t
jsn_i32(json_t *jsn, const char *key) {
  return jsn_i32_def(jsn, key, 0);
}

float
jsn_flt(json_t *jsn, const char *key) {
  json_t *jval;

  jval = json_object_get(jsn, key);
  if (!jval)
    return 0.0f;

  return (float)json_number_value(jval);
}

float
jsn_flt_at(json_t *jsn, int32_t index) {
  return json_number_value(json_array_get(jsn, index));
}

void
jsn_flt_if(json_t *jsn, const char *key, float *dest) {
  json_t *jval;

  if ((jval = json_object_get(jsn, key)))
    *dest = (float)json_number_value(jval);
}

int64_t
jsn_i64(json_t *jsn, const char *key) {
  json_t *jval;

  jval = json_object_get(jsn, key);
  if (!jval)
    return 0;

  return json_integer_value(jval);
}
