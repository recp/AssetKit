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
  char *map_Ns;
  char *map_d;
  char *decal;
  char *disp;
  char *bump;
  vec3  Ka;
  vec3  Kd;
  vec3  Ks;
  vec3  Ke;
  vec3  Tf;
  float Ni;
  float Ns;     /* exponent */
  float d;      /* dissolve */
  float dHalo;  /* dissolve halo */
  float Tr;     /* Transparent (1 - d) */
  float sharpness; /* The default is 60   */
  int   illum;
  bool  map_aat;
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
  bool           hasTexture;
  bool           hasNormal;
} WOPrim;

typedef struct WOObject {
  struct WOObject *next;
  AkGeometry      *geom;
  WOPrim          *prim;
} WOObject;

typedef struct WOState {
  AkHeap        *heap;
  AkDoc         *doc;
  void          *tmp;
  AkLibrary     *lib_geom;
  AkNode        *node;
  WOMtlLib      *mtlib;
  AkDataContext *dc_pos, *dc_tex, *dc_nor;
  AkAccessor    *ac_pos, *ac_tex, *ac_nor;
  WOObject      *obj;
} WOState;

#ifdef SKIP_SPACES
# undef SKIP_SPACES
#endif

#define SKIP_SPACES                                                           \
  {                                                                           \
    c = p ? *p : '\0';                                                        \
    while (c != '\0' && AK_ARRAY_SPACE_CHECK) c = *++p;                       \
    if (c == '\0')                                                            \
      break; /* to break loop */                                              \
  }

#ifdef NEXT_LINE
# undef NEXT_LINE
#endif

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
