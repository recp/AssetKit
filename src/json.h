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

#ifndef ak_json_h
#define ak_json_h

#include <string.h>
#include <stdlib.h>

/* JSON parser */
#include <json/json.h>

AK_INLINE
char *
json_strdup(const json_t * __restrict jsonObject,
            AkHeap       * __restrict heap,
            void         * __restrict parent) {
  return ak_heap_strndup(heap,
                         parent,
                         json_string(jsonObject),
                         jsonObject->valsize);
}

AK_INLINE
void
json_array_set(void         * __restrict p,
               AkTypeId                  typeId,
               int                       index,
               const json_t * __restrict json) {
  switch (typeId) {
    case AKT_FLOAT:
      ((float *)p)[index] = json_float(json, 0.0f);
      break;
    case AKT_INT:
      ((int32_t *)p)[index] = json_int32(json, 0);
      break;
    case AKT_UINT:
      ((int32_t *)p)[index] = json_uint32(json, 0);
      break;
    case AKT_SHORT:
      ((int16_t *)p)[index] = json_int32(json, 0);
      break;
    case AKT_USHORT:
      ((uint16_t *)p)[index] = json_uint32(json, 0);
      break;
    case AKT_BYTE:
      ((char *)p)[index] = json_int32(json, 0);
      break;
    case AKT_UBYTE:
      ((unsigned char *)p)[index] = json_uint32(json, 0);
      break;
    default:
      break;
  }
}

#endif /* ak_json_h */
