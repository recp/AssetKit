/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_core_types_h
#define ak_core_types_h

typedef const char           *AkString;
typedef char                 *AkMutString;
typedef bool                  AkBool;
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

typedef AK_ALIGN(8)  AkFloat  AkFloat3[3];
typedef AK_ALIGN(8)  AkDouble AkDouble3[3];
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

#endif /* ak_core_types_h */
