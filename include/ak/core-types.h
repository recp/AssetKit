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

#ifndef assetkit_core_types_h
#define assetkit_core_types_h

#include "common.h"

typedef const char           *AkString;
typedef char                 *AkMutString;
typedef bool                  AkBool;
typedef uint8_t               AkUInt8;
typedef int16_t               AkInt16;
typedef uint16_t              AkUInt16;
typedef int32_t               AkInt;
typedef uint32_t              AkUInt;
typedef int64_t               AkInt64;
typedef uint64_t              AkUInt64;
typedef float                 AkFloat;
typedef double                AkDouble;

typedef AkBool                AkBool4[4];
typedef AkInt                 AkInt2[2];
typedef AkInt                 AkInt4[4];
typedef AkFloat               AkFloat2[2];
typedef AkDouble              AkDouble2[2];

typedef              AkFloat  AkFloat3[3];
typedef              AkDouble AkDouble3[3];
typedef AK_ALIGN(16) AkFloat  AkFloat4[4];
typedef AK_ALIGN(16) AkDouble AkDouble4[4];
typedef AK_ALIGN(32) AkDouble AkDouble4x4[4];
typedef AK_ALIGN(32) AkFloat4 AkFloat4x4[4];

typedef struct AkColorRGBA {
  AkFloat R;
  AkFloat G;
  AkFloat B;
  AkFloat A;
} AkColorRGBA;

typedef union AkColor {
  AK_ALIGN(16) AkColorRGBA rgba;
  AK_ALIGN(16) AkFloat4    vec;
} AkColor;

AK_INLINE
bool
ak_colorLessThanOne(AkColor color) {
  return color.rgba.R < 0.999
      || color.rgba.G < 0.999
      || color.rgba.B < 0.999
      || color.rgba.A < 0.999
  ;
}

AK_INLINE
float
ak_sRGB_linearf(float channel) {
  if (channel <= 0.04045) {
    return channel / 12.92;
  } else {
    return powf((channel + 0.055) / 1.055, 2.4);
  }
}

AK_INLINE
float
ak_linear_sRGBf(float channel) {
  if (channel <= 0.0031308f) {
    return channel * 12.92f;
  } else {
    return 1.055f * powf(channel, 1.0f / 2.4f) - 0.055f;
  }
}

AK_INLINE
void
ak_sRGB_linear(AkColor * __restrict color) {
  color->rgba.R = ak_sRGB_linearf(color->rgba.R);
  color->rgba.G = ak_sRGB_linearf(color->rgba.G);
  color->rgba.B = ak_sRGB_linearf(color->rgba.B);
}

AK_INLINE
void
ak_linear_sRGB(AkColor * __restrict color) {
  color->rgba.R = ak_linear_sRGBf(color->rgba.R);
  color->rgba.G = ak_linear_sRGBf(color->rgba.G);
  color->rgba.B = ak_linear_sRGBf(color->rgba.B);
}

#undef AK__DEF_ARRAY

#define AK__DEF_ARRAY(TYPE)                                                   \
  typedef struct TYPE##Array {                                                \
    size_t count;                                                             \
    TYPE   items[];                                                           \
  } TYPE##Array;                                                              \
                                                                              \
  typedef struct TYPE##ArrayL {                                               \
    struct TYPE##ArrayL * next;                                               \
    size_t count;                                                             \
    TYPE   items[];                                                           \
  } TYPE##ArrayL

AK__DEF_ARRAY(AkBool);
AK__DEF_ARRAY(AkUInt8);
AK__DEF_ARRAY(AkUInt16);
AK__DEF_ARRAY(AkInt);
AK__DEF_ARRAY(AkUInt);
AK__DEF_ARRAY(AkFloat);
AK__DEF_ARRAY(AkDouble);
AK__DEF_ARRAY(AkString);

#undef AK__DEF_ARRAY

#endif /* assetkit_core_types_h */
