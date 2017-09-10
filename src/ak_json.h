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
#include <jansson.h>

const char*
json_cstr(json_t *jsn, const char *key);

int32_t
json_int32_def(json_t     *jsn,
               const char *key,
               int32_t     def);

int32_t
json_int32(json_t *jsn, const char *key);

int64_t
json_int64(json_t *jsn, const char *key);

float
json_float(json_t *jsn, const char *key);

#endif /* ak_json_h */
