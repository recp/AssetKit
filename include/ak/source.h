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

#include "common.h"

#include <stdint.h>
#include <stdbool.h>
#include "type.h"
#include "url.h"
#include "core-types.h"

/*
  Input -> Source -> TechniqueCommon (Accessor) -> Buffer
  Input -> Accessor -> Buffer
*/

struct AkTechnique;
struct AkBuffer;

/* for vectors: item count,
   for matrics: item count | matrix size
*/
typedef enum AkComponentSize {
  AK_COMPONENT_SIZE_UNKNOWN = 0,
  AK_COMPONENT_SIZE_SCALAR  = 1,
  AK_COMPONENT_SIZE_VEC2    = 2,
  AK_COMPONENT_SIZE_VEC3    = 3,
  AK_COMPONENT_SIZE_VEC4    = 4,
  AK_COMPONENT_SIZE_MAT2    = (4  << 3) | 2,
  AK_COMPONENT_SIZE_MAT3    = (9  << 3) | 3,
  AK_COMPONENT_SIZE_MAT4    = (16 << 3) | 4
} AkComponentSize;

typedef struct AkDataParam {
  /* const char * sid; */

  struct AkDataParam *next;
  const char         *name;
  const char         *semantic;
  AkTypeDesc          type;
} AkDataParam;

typedef struct AkBuffer {
  const char *name;
  void       *data;
  size_t      length;
} AkBuffer;

typedef struct AkAccessor {
  struct AkBuffer *buffer;
  const char      *name;
  void            *min;
  void            *max;
  size_t           byteOffset;           /* byte offset on the buffer        */
  size_t           byteStride;           /* stride in bytes                  */
  size_t           byteLength;           /* total bytes for this accessor    */
  uint32_t         count;                /* count to access buffer           */
  uint32_t         componentBytes;       /* component stride in bytes        */
  AkComponentSize  componentSize;        /* vec1 | vec2 | vec3 | vec4 ...    */
  AkTypeId         componentType;        /* single component type            */
  uint32_t         componentCount;
  size_t           fillByteSize;         /* filled size for single access    */
  int32_t          gpuTarget;            /* GPU buffer target to bound       */
  bool             normalized;
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
  AkUIntArray              *dupcsum;
  size_t                    startIndex;
  size_t                    endIndex;
} AkDuplicatorRange;

typedef struct AkDuplicator {
  AkDuplicatorRange *range;
  void              *buffstate;
  void              *vertices;
  size_t             dupCount;
  size_t             bufCount;
} AkDuplicator;

typedef struct AkSourceBuffState {
  AkDuplicator *duplicator;
  void         *buff;
  char         *url;
  size_t        count;
  uint32_t      stride;
} AkSourceBuffState;

typedef struct AkSourceEditHelper {
  struct AkSourceEditHelper *next;
  AkAccessor                *oldsource;
  AkAccessor                *source;
} AkSourceEditHelper;

AK_EXPORT
AkBuffer*
ak_sourceDetachArray(AkAccessor * __restrict acc);

#ifdef __cplusplus
}
#endif
#endif /* ak_source_h */
