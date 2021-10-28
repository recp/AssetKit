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

typedef const char           *AkString;
typedef char                 *AkMutString;
typedef bool                  AkBool;
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
AK__DEF_ARRAY(AkInt);
AK__DEF_ARRAY(AkUInt);
AK__DEF_ARRAY(AkFloat);
AK__DEF_ARRAY(AkDouble);
AK__DEF_ARRAY(AkString);

#undef AK__DEF_ARRAY

#endif /* assetkit_core_types_h */
