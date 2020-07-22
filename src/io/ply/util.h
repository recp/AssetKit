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

AK_INLINE
float
ply_flt(char * __restrict p, PLYProperty * __restrict prop, bool isLittleEndian) {
  if (prop->typeDesc->size == 4) {
    switch (prop->typeDesc->typeId) {
      case AKT_FLOAT: {
        float f;
        if (isLittleEndian) {
          le_32(f, p);
        } else {
          be_32(f, p);
        }
        return f;
      }
      case AKT_INT: {
        int32_t f;
        if (isLittleEndian) {
          le_32(f, p);
        } else {
          be_32(f, p);
        }
        return (float)f;
      }
      case AKT_UINT: {
        uint32_t f;
        if (isLittleEndian) {
          le_32(f, p);
        } else {
          be_32(f, p);
        }
        return (float)f;
      }
      default: return 0.0f;
    }
  } else if (prop->typeDesc->size == 8) {
    switch (prop->typeDesc->typeId) {
      case AKT_DOUBLE: {
        double f;
        if (isLittleEndian) {
          le_64(f, p);
        } else {
          be_64(f, p);
        }
        return (float)f;
      }
      case AKT_INT64: {
        int64_t f;
        if (isLittleEndian) {
          le_64(f, p);
        } else {
          be_64(f, p);
        }
        return (float)f;
      }
      case AKT_UINT64: {
        uint64_t f;
        if (isLittleEndian) {
          le_64(f, p);
        } else {
          be_64(f, p);
        }
        return (float)f;
      }
      default: return 0.0f;
    }
  } else if (prop->typeDesc->size == 2) {
    switch (prop->typeDesc->typeId) {
      case AKT_SHORT: {
        int16_t f;
        memcpy(&f, p, 2);
        p += 2;
        return (float)f;
      }
      case AKT_USHORT: {
        uint16_t f;
        memcpy(&f, p, 2);
        p += 2;
        return (float)f;
      }
      default: return 0.0f;
    }
  } else if (prop->typeDesc->size == 1) {
    switch (prop->typeDesc->typeId) {
      case AKT_BYTE: {
        int8_t f;
        memcpy(&f, p, 1);
        p += 2;
        return (float)f;
      }
      case AKT_UBYTE: {
        uint8_t f;
        memcpy(&f, p, 1);
        p += 2;
        return (float)f;
      }
      default: return 0.0f;
    }
  }
  
  return 0.0f;
}

AK_INLINE
uint32_t
ply_u32(char * __restrict p, PLYProperty * __restrict prop, bool isLittleEndian) {
  if (prop->typeDesc->size == 4) {
    switch (prop->typeDesc->typeId) {
      case AKT_INT: {
        int32_t f;
        if (isLittleEndian) {
          le_32(f, p);
        } else {
          be_32(f, p);
        }
        return (uint32_t)f;
      }
      case AKT_UINT: {
        uint32_t f;
        if (isLittleEndian) {
          le_32(f, p);
        } else {
          be_32(f, p);
        }
        return f;
      }
      case AKT_FLOAT: {
        float f;
        if (isLittleEndian) {
          le_32(f, p);
        } else {
          be_32(f, p);
        }
        return (uint32_t)f;
      }
      default: return 0;
    }
  } else if (prop->typeDesc->size == 8) {
    switch (prop->typeDesc->typeId) {
      case AKT_INT64: {
        int64_t f;
        if (isLittleEndian) {
          le_64(f, p);
        } else {
          be_64(f, p);
        }
        return (uint32_t)f;
      }
      case AKT_UINT64: {
        uint64_t f;
        if (isLittleEndian) {
          le_64(f, p);
        } else {
          be_64(f, p);
        }
        return (uint32_t)f;
      }
      case AKT_DOUBLE: {
        double f;
        if (isLittleEndian) {
          le_64(f, p);
        } else {
          be_64(f, p);
        }
        return (uint32_t)f;
      }
      default: return 0;
    }
  } else if (prop->typeDesc->size == 2) {
    switch (prop->typeDesc->typeId) {
      case AKT_SHORT: {
        int16_t f;
        memcpy(&f, p, 2);
        p += 2;
        return (uint32_t)f;
      }
      case AKT_USHORT: {
        uint16_t f;
        memcpy(&f, p, 2);
        p += 2;
        return (uint32_t)f;
      }
      default: return 0;
    }
  } else if (prop->typeDesc->size == 1) {
    switch (prop->typeDesc->typeId) {
      case AKT_BYTE: {
        int8_t f;
        memcpy(&f, p, 1);
        p += 2;
        return (uint32_t)f;
      }
      case AKT_UBYTE: {
        uint8_t f;
        memcpy(&f, p, 1);
        p += 2;
        return (uint32_t)f;
      }
      default: return 0;
    }
  }
  
  return 0;
}

#endif /* ply_util_h */
