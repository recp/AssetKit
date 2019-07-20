/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_json_h
#define ak_json_h

#include <string.h>
#include <stdlib.h>

/* JSON parser */
#include <json/json.h>

const char*
json_cstr(json_t *jsn, const char *key);

int32_t
jsn_i32_def(json_t     *jsn,
               const char *key,
               int32_t     def);

int32_t
jsn_i32(json_t *jsn, const char *key);

int64_t
jsn_i64(json_t *jsn, const char *key);

float
jsn_flt(json_t *jsn, const char *key);

float
jsn_flt_at(json_t *jsn, int32_t index);

void
jsn_flt_if(json_t *jsn, const char *key, float *dest);

#endif /* ak_json_h */
