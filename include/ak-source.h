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
#include "ak-type.h"

/*
  Input -> Source -> TechniqueCommon -> Accessor -> Buffer
*/

struct AkTechnique;

typedef struct AkDataParam {
  /* const char * sid; */

  struct AkDataParam *next;
  const char         *name;
  const char         *semantic;
  AkTypeDesc          type;
  uint32_t            offset;
} AkDataParam;

typedef struct AkBuffer {
  const char *name;
  void       *data;
  size_t      length;
  size_t      reserved;
} AkBuffer;

typedef struct AkAccessor {
  AkURL               source;
  AkTypeDesc         *type;
  struct AkDataParam *param;
  size_t              count;
  size_t              offset;
  uint32_t            firstBound;
  uint32_t            stride;
  uint32_t            bound;
  AkTypeId            itemTypeId;

  size_t              byteOffset;
  size_t              byteLength;
  uint32_t            byteStride;
} AkAccessor;

typedef struct AkSource {
  /* const char * id; */
  const char         *name;
  AkBuffer           *buffer;
  AkAccessor         *tcommon;
  struct AkTechnique *technique;
  struct AkSource    *next;
  int32_t             target;
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

typedef struct AkSourceEditHelper {
  struct AkSourceEditHelper *next;
  AkSource *oldsource;
  AkSource *source;
  void     *url;
  void     *arrayid;
  bool      isnew;
} AkSourceEditHelper;

AK_EXPORT
AkBuffer*
ak_sourceDetachArray(AkAccessor * __restrict acc);

#ifdef __cplusplus
}
#endif
#endif /* ak_source_h */
