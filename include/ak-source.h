/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_source_h
#define ak_source_h
#ifdef __cplusplus
extern "C" {
#endif

struct AkTechnique;
struct AkAssetInf;

typedef enum AkSourceArrayType {
  AK_SOURCE_ARRAY_TYPE_BOOL   = 1,
  AK_SOURCE_ARRAY_TYPE_FLOAT  = 2,
  AK_SOURCE_ARRAY_TYPE_INT    = 3,
  AK_SOURCE_ARRAY_TYPE_IDREF  = 4,
  AK_SOURCE_ARRAY_TYPE_NAME   = 5,
  AK_SOURCE_ARRAY_TYPE_SIDREF = 6,
  AK_SOURCE_ARRAY_TYPE_TOKEN  = 7
} AkSourceArrayType;

typedef struct AkDataType {
  const char *typeName;
  AkValueType type;
  int         size;
} AkDataType;

typedef struct AkDataParam {
  /* const char * sid; */

  struct AkDataParam *next;
  const char         *name;
  const char         *semantic;
  AkDataType          type;
  uint32_t            offset;
} AkDataParam;

typedef struct AkAccessor {
  AkURL    source;
  size_t   count;
  size_t   offset;
  uint32_t firstBound;
  uint32_t stride;
  uint32_t bound;
  struct AkDataParam *param;
} AkAccessor;

typedef struct AkSource {
  /* const char * id; */
  struct AkAssetInf  *inf;
  const char         *name;
  AkObject           *data; /* arrays inside source */
  AkAccessor         *tcommon;
  struct AkTechnique *technique;
  struct AkSource    *next;
} AkSource;

typedef struct AkSourceArrayNew {
  void     *array;
  char     *url;
  size_t    count;
  uint32_t  offset;
  uint32_t  stride;
} AkSourceArrayNew;

typedef struct AkSourceBoolArray {
  AkSourceArrayNew *newArray;
  /* const char * id; */
  const char *name;
  size_t      count;
  AkBool      items[];
} AkSourceBoolArray;

typedef struct AkSourceFloatArray {
  /* const char * id; */
  AkSourceArrayNew *newArray;
  const char *name;
  size_t      count;
  AkUInt      digits;
  AkUInt      magnitude;
  AkFloat     items[];
} AkSourceFloatArray;

typedef struct AkSourceIntArray {
  /* const char * id; */
  AkSourceArrayNew *newArray;
  const char *name;
  size_t      count;
  AkInt       minInclusive;
  AkInt       maxInclusive;
  AkInt       items[];
} AkSourceIntArray;

typedef struct AkSourceStringArray {
  AkSourceArrayNew *newArray;
  /* const char  * id; */
  const char  *name;
  size_t       count;
  AkString     items[];
} AkSourceStringArray;

static
AK_INLINE
size_t
ak_sourceArraySize(AkSourceArrayType type) {
  switch (type) {
    case AK_SOURCE_ARRAY_TYPE_FLOAT : return sizeof(AkSourceFloatArray);
    case AK_SOURCE_ARRAY_TYPE_INT   : return sizeof(AkSourceIntArray);
    case AK_SOURCE_ARRAY_TYPE_SIDREF:
    case AK_SOURCE_ARRAY_TYPE_IDREF :
    case AK_SOURCE_ARRAY_TYPE_NAME  :
    case AK_SOURCE_ARRAY_TYPE_TOKEN : return sizeof(AkSourceStringArray);
    case AK_SOURCE_ARRAY_TYPE_BOOL  : return sizeof(AkSourceBoolArray);
    default                         : return 0;
  }
}

#ifdef __cplusplus
}
#endif
#endif /* ak_source_h */
