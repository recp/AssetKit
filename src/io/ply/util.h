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

#ifndef ply_util_h
#define ply_util_h

#include "common.h"
#include "../../endian.h"

#define ply_val(p, typeDesc, leEndian, T, DEST, DEFAULT)                      \
  do {                                                                        \
    uint64_t buf;                                                             \
                                                                              \
    buf = 0;                                                                  \
    switch (typeDesc->typeId) {                                               \
      case AKT_FLOAT:                                                         \
      case AKT_INT:                                                           \
      case AKT_UINT:   memcpy_endian32(leEndian, buf, p); break;              \
      case AKT_DOUBLE:                                                        \
      case AKT_INT64:                                                         \
      case AKT_UINT64: memcpy_endian64(leEndian, buf, p); break;              \
      case AKT_SHORT:                                                         \
      case AKT_USHORT: memcpy_endian16(leEndian, buf, p); break;              \
      case AKT_BYTE:                                                          \
      case AKT_UBYTE:  memcpy(&buf, p++, 1);              break;              \
      default:         DEST = DEFAULT;                    break;              \
    }                                                                         \
                                                                              \
    switch (typeDesc->typeId) {                                               \
      case AKT_FLOAT:  DEST = (T)(*(float    *)&buf);     break;              \
      case AKT_INT:    DEST = (T)(*(int32_t  *)&buf);     break;              \
      case AKT_UINT:   DEST = (T)(*(uint32_t *)&buf);     break;              \
      case AKT_DOUBLE: DEST = (T)(*(double   *)&buf);     break;              \
      case AKT_INT64:  DEST = (T)(*(int64_t  *)&buf);     break;              \
      case AKT_UINT64: DEST = (T)(*(uint64_t *)&buf);     break;              \
      case AKT_SHORT:  DEST = (T)(*(int16_t  *)&buf);     break;              \
      case AKT_USHORT: DEST = (T)(*(uint16_t *)&buf);     break;              \
      case AKT_BYTE:   DEST = (T)(*(int8_t   *)&buf);     break;              \
      case AKT_UBYTE:  DEST = (T)(*(uint8_t  *)&buf);     break;              \
      default:         DEST = DEFAULT;                    break;              \
    }                                                                         \
  } while (0)

#endif /* ply_util_h */
