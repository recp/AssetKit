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

#include <stdint.h>
#include <stdbool.h>
#include "ak-rb.h"

struct AkTechnique;

typedef enum AkSourceArrayType {
  AK_SOURCE_ARRAY_TYPE_UNKNOWN = 0,
  AK_SOURCE_ARRAY_TYPE_FLOAT   = 1 << 0,
  AK_SOURCE_ARRAY_TYPE_INT     = 1 << 1,
  AK_SOURCE_ARRAY_TYPE_BOOL    = 1 << 2,
  AK_SOURCE_ARRAY_TYPE_IDREF   = 1 << 3,
  AK_SOURCE_ARRAY_TYPE_NAME    = 1 << 4,
  AK_SOURCE_ARRAY_TYPE_SIDREF  = 1 << 5,
  AK_SOURCE_ARRAY_TYPE_TOKEN   = 1 << 6,
  AK_SOURCE_ARRAY_TYPE_STRING  = AK_SOURCE_ARRAY_TYPE_IDREF
                               | AK_SOURCE_ARRAY_TYPE_NAME
                               | AK_SOURCE_ARRAY_TYPE_SIDREF
                               | AK_SOURCE_ARRAY_TYPE_TOKEN
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
  const char         *name;
  AkObject           *data; /* arrays inside source */
  AkAccessor         *tcommon;
  struct AkTechnique *technique;
  struct AkSource    *next;
} AkSource;

typedef struct AkDuplicatorRange {
  struct AkDuplicatorRange *next;
  AkUIntArray              *dupc;
  size_t                    startIndex;
  size_t                    endIndex;
} AkDuplicatorRange;

typedef struct AkDuplicator {
  void              *arrstate;
  void              *vertices;
  AkDuplicatorRange *range;
  size_t             dupCount;
} AkDuplicator;

typedef struct AkSourceArrayState {
  AkDuplicator *duplicator;
  void         *array;
  char         *url;
  size_t        count;
  uint32_t      stride;
  uint32_t      lastoffset;
} AkSourceArrayState;

typedef struct AkSourceArrayBase {
  /* const char * id; */
  const char       *name;
  size_t            count;
  AkSourceArrayType type;
  void             *items;
} AkSourceArrayBase;

typedef struct AkSourceBoolArray {
  AkSourceArrayBase base;
  AkBool            items[];
} AkSourceBoolArray;

typedef struct AkSourceFloatArray {
  AkSourceArrayBase base;
  AkUInt            digits;
  AkUInt            magnitude;
  AkFloat           items[];
} AkSourceFloatArray;

typedef struct AkSourceIntArray {
  AkSourceArrayBase base;
  AkInt             minInclusive;
  AkInt             maxInclusive;
  AkInt             items[];
} AkSourceIntArray;

typedef struct AkSourceStringArray {
  AkSourceArrayBase base;
  AkMutString       items[];
} AkSourceStringArray;

typedef struct AkSourceEditHelper {
  struct AkSourceEditHelper *next;
  AkSource *oldsource;
  AkSource *source;
  void     *url;
  void     *arrayid;
  bool      isnew;
} AkSourceEditHelper;

static
AK_INLINE
size_t
ak_sourceArraySize(AkSourceArrayType type) {
  switch (type) {
    case AK_SOURCE_ARRAY_TYPE_FLOAT  : return sizeof(AkSourceFloatArray);
    case AK_SOURCE_ARRAY_TYPE_INT    : return sizeof(AkSourceIntArray);
    case AK_SOURCE_ARRAY_TYPE_STRING : return sizeof(AkSourceStringArray);
    case AK_SOURCE_ARRAY_TYPE_BOOL   : return sizeof(AkSourceBoolArray);
    default                          : return 0;
  }
}

static
AK_INLINE
size_t
ak_sourceArrayItemSize(AkSourceArrayType type) {
  switch (type) {
    case AK_SOURCE_ARRAY_TYPE_FLOAT : return sizeof(AkFloat);
    case AK_SOURCE_ARRAY_TYPE_INT   : return sizeof(AkInt);
    default                         : return 1;
  }
}

static
AK_INLINE
void*
ak_sourceArrayItems(AkSourceArrayBase *array) {
  switch (array->type) {
    case AK_SOURCE_ARRAY_TYPE_FLOAT:
      return ((AkSourceFloatArray *)array)->items;
    case AK_SOURCE_ARRAY_TYPE_INT:
      return ((AkSourceIntArray *)array)->items;
    case AK_SOURCE_ARRAY_TYPE_STRING:
      return ((AkSourceStringArray *)array)->items;
    case AK_SOURCE_ARRAY_TYPE_BOOL:
      return ((AkSourceBoolArray *)array)->items;
    default: return NULL;
  }
}

AK_EXPORT
AkObject*
ak_sourceDetachArray(AkAccessor * __restrict acc);

#ifdef __cplusplus
}
#endif
#endif /* ak_source_h */
