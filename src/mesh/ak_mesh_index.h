/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_mesh_index_h
#define ak_mesh_index_h

#include "../ak_common.h"

typedef struct AkInputDesc {
  struct AkInputDesc *next;
  const char         *semantic;
  AkURL              *source;
  uint32_t            set;
} AkInputDesc;

typedef struct AkFakePrim {
  uint8_t         *flg;
  AkUIntArray     *ind;
  AkInput         *input;
  uint32_t         chk_start;
  uint32_t         chk_end;
  uint32_t         vo;     /* vertOffset */
  uint32_t         st;
  uint32_t         count;
  uint32_t         icount;
  uint32_t         ccount; /* checked count */
  struct AkFakePrim *next;
} AkFakePrim;


#endif /* ak_mesh_index_h */
