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

#ifndef wobj_commoh_h
#define wobj_commoh_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../json.h"
#include "../../data.h"

#include <string.h>
#include <stdlib.h>

typedef struct WOMtl {
  char *name;
  char *map_Ka;
  char *map_Kd;
  char *map_Ks;
  char *map_Ke;
  vec3  Ka;
  vec3  Kd;
  vec3  Ks;
  vec3  Ke;
  float Ni;
  float Ns; /* exponent */
  float d;  /* dissolve */
  float Tr; /* Transparent (1 - d) */
  int   illum;
} WOMtl;

typedef struct WOMtlLib {
  char   *name;
  RBTree *materials;
} WOMtlLib;

typedef struct WOPrim {
  struct WOPrim *next;
  const char    *mtlname;
  AkDataContext *dc_face;
  AkDataContext *dc_vcount;
  uint32_t       maxVC;
  bool           isdefault;
  bool           hasTexture;
  bool           hasNormal;
} WOPrim;

typedef struct WOObject {
  AkGeometry    *geom;
  WOPrim        *prim;
  AkDataContext *dc_pos, *dc_tex, *dc_nor;
  AkAccessor    *ac_pos, *ac_tex, *ac_nor;
  bool           isdefault;
} WOObject;

typedef struct WOState {
  AkHeap        *heap;
  AkDoc         *doc;
  void          *tmp;
  AkLibrary     *lib_geom;
  AkNode        *node;
  WOMtlLib      *mtlib;
  WOObject       obj;
} WOState;

#define SKIP_SPACES                                                           \
  {                                                                           \
    while (c != '\0' && AK_ARRAY_SPACE_CHECK) c = *++p;                       \
    if (c == '\0')                                                            \
      break; /* to break loop */                                              \
  }

#define NEXT_LINE                                                             \
  do {                                                                        \
    c = p ? *p : '\0';                                                        \
    while (p                                                                  \
           && p[0] != '\0'                                                    \
           && !AK_ARRAY_NLINE_CHECK                                           \
           && (c = *++p) != '\0'                                              \
           && !AK_ARRAY_NLINE_CHECK);                                         \
                                                                              \
    while (p                                                                  \
           && p[0] != '\0'                                                    \
           && AK_ARRAY_NLINE_CHECK                                            \
           && (c = *++p) != '\0'                                              \
           && AK_ARRAY_NLINE_CHECK);                                          \
  } while(0);

#endif /* wobj_commoh_h */
