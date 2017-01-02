/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_mesh_index_h
#define ak_mesh_index_h

#include "../ak_common.h"

typedef struct AkArrayList {
  struct AkArrayList *next;
  void               *data;
  void               *newdata;
} AkArrayList;

typedef struct AkPrimProxy {
  struct AkPrimProxy *next;
  AkMeshPrimitive    *orig;
  uint8_t            *flg;
  AkUIntArray        *ind;
  AkUIntArray        *newind;
  AkInput            *input;
  uint32_t            chk_start;
  uint32_t            chk_end;
  uint32_t            vo;     /* vertOffset */
  uint32_t            st;
  uint32_t            count;
  uint32_t            icount;
  uint32_t            ccount; /* checked count */
} AkPrimProxy;


typedef struct AkInputDesc {
  struct AkInputDesc *next;
  const char         *semantic;
  AkURL              *source;
  AkInput            *input;
  AkPrimProxy        *pp;
  uint32_t            set;
  uint32_t            tag;
} AkInputDesc;

_assetkit_hide
AkResult
ak_mesh_fix_indices(AkHeap *heap, AkMesh *mesh);

#endif /* ak_mesh_index_h */
