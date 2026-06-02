/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef gltf_commoh_h
#define gltf_commoh_h

#include "state.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../json.h"
#include "../../string_fast.h"

#include <string.h>
#include <stdlib.h>

/* JSON parser */
#include <json/json.h>

#define GLTF_JSON_OBJMAP_FN8(NAME, FUN, PARAM)                               \
  JSON_OBJMAP_FNP(_s_gltf_##NAME,                                            \
                  _s_gltf_##NAME##_len,                                      \
                  _s_gltf_##NAME##_u64_exact,                                \
                  FUN,                                                       \
                  PARAM)

#define GLTF_JSON_OBJMAP_OBJ8(NAME, USERDATA)                                \
  JSON_OBJMAP_OBJP(_s_gltf_##NAME,                                           \
                   _s_gltf_##NAME##_len,                                     \
                   _s_gltf_##NAME##_u64_exact,                               \
                   USERDATA)

#define GLTF_JSON_KEY_EQ4(OBJ, NAME)                                         \
  json_key_eq_packed4(OBJ, _s_gltf_##NAME##_u32_exact, _s_gltf_##NAME##_len)

#define GLTF_JSON_VAL_EQ4(OBJ, NAME)                                         \
  json_val_eq_packed4(OBJ, _s_gltf_##NAME##_u32_exact, _s_gltf_##NAME##_len)

#define GLTF_JSON_KEY_EQ8(OBJ, NAME)                                         \
  json_key_eq_packed(OBJ, _s_gltf_##NAME##_u64_exact, _s_gltf_##NAME##_len)

#define GLTF_JSON_VAL_EQ8(OBJ, NAME)                                         \
  json_val_eq_packed(OBJ, _s_gltf_##NAME##_u64_exact, _s_gltf_##NAME##_len)

#define GLTF_JSON_KEY_EQ(OBJ, NAME)                                          \
  json_key_eqsz(OBJ, _s_gltf_##NAME, _s_gltf_##NAME##_len)

#define GLTF_JSON_VAL_EQ(OBJ, NAME)                                          \
  json_val_eqsz(OBJ, _s_gltf_##NAME, _s_gltf_##NAME##_len)

#define GLTF_JSON_GET8(OBJECT, NAME)                                         \
  gltf_jsonGetPacked(OBJECT,                                                \
                     _s_gltf_##NAME##_u64_exact,                            \
                     _s_gltf_##NAME##_len)

#define GLTF_JSON_GET4(OBJECT, NAME)                                         \
  gltf_jsonGetPacked4(OBJECT,                                                \
                      _s_gltf_##NAME##_u32_exact,                            \
                      _s_gltf_##NAME##_len)

#define GLTF_JSON_GET(OBJECT, NAME)                                          \
  gltf_jsonGetLen(OBJECT, _s_gltf_##NAME, _s_gltf_##NAME##_len)

static inline
json_t*
gltf_jsonGetPacked4(const json_t * __restrict object,
                    uint32_t                 packed,
                    size_t                   len) {
  json_t *iter;

  if (!object
      || object->type != JSON_OBJECT
      || !(iter = (json_t *)object->value))
    return NULL;

  while (iter && !json_key_eq_packed4(iter, packed, len))
    iter = iter->next;

  return iter;
}

static inline
json_t*
gltf_jsonGetPacked(const json_t * __restrict object,
                   uint64_t                  packed,
                   size_t                    len) {
  json_t *iter;

  if (!object
      || object->type != JSON_OBJECT
      || !(iter = (json_t *)object->value))
    return NULL;

  while (iter && !json_key_eq_packed(iter, packed, len))
    iter = iter->next;

  return iter;
}

#define GETCHILD(INITIAL, ITEM, INDEX)                                        \
  do {                                                                        \
    int i;                                                                    \
    if (INITIAL && (i = INDEX) >= 0) {                                        \
      ITEM = (void *)INITIAL;                                                 \
      while (i > 0) {                                                         \
        if (!(ITEM = (void *)ITEM->base.next)) {                              \
          i     = -1;                                                         \
          ITEM  = NULL;                                                       \
          break;  /* not foud */                                              \
        }                                                                     \
        i--;                                                                  \
      }                                                                       \
    } else {                                                                  \
      ITEM = NULL;                                                            \
    }                                                                         \
  } while (0)

#endif /* gltf_commoh_h */
