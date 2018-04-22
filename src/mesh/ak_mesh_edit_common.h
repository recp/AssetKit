/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_mesh_edit_common_h
#define ak_mesh_edit_common_h

#include "../ak_common.h"

typedef struct AkPrimProxy {
  struct AkPrimProxy *next;
  AkMeshPrimitive    *orig;
  uint8_t            *flg;
  AkUIntArray        *ind;
  AkUIntArray        *newind;
  AkInput            *input;
  uint32_t           *inpi;
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
  int32_t             index;
} AkInputDesc;

void
ak_meshFreeRsvBuff(RBTree *tree, RBNode *node);

#endif /* ak_meh_edit_common_h */
