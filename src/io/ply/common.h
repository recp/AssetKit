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

#ifndef ply_commoh_h
#define ply_commoh_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../json.h"
#include "../../data.h"

#include <string.h>
#include <stdlib.h>

typedef struct PLYProperty {
  struct PLYProperty *next;
  char               *name;
  char               *type;
  char               *listCountType;
  bool                islist;
} PLYProperty;

typedef enum PLYElementType {
  PLY_ELEM_UNKNOWN = 0,
  PLY_ELEM_VERTEX  = 1,
  PLY_ELEM_FACE    = 2
} PLYElementType;

typedef struct PLYElement {
  struct PLYElement *next;
  PLYProperty       *lastProperty;
  PLYProperty       *property;
  char              *name;
  size_t             count;
  PLYElementType     type;
} PLYElement;

typedef struct PLYState {
  AkHeap        *heap;
  AkDoc         *doc;
  void          *tmpParent;
  AkLibrary     *lib_geom;
  AkGeometry    *geom;
  AkDataContext *dc_ind, *dc_pos, *dc_nor, *dc_vcount;
  AkNode        *node;
  PLYElement    *element;
  PLYElement    *lastElement;
  uint32_t       maxVC;
  uint32_t       count;
} PLYState;

#define SKIP_SPACES                                                           \
  {                                                                           \
    while (c != '\0' && AK_ARRAY_SPACE_CHECK) c = *++p;                       \
    if (c == '\0')                                                            \
      break; /* to break loop */                                              \
  }

#define NEXT_LINE                                                             \
  do {                                                                        \
    c = *p;                                                                   \
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

#define EQ4(c1,c2,c3,c4) \
    (p[0] == c1 \
  && p[1] == c2 \
  && p[2] == c3 \
  && p[3] == c4 \
  && (p[4] == ' ' || p[4] == '\t'))

#define EQ5(c1,c2,c3,c4,c5) \
    (p[0] == c1 \
  && p[1] == c2 \
  && p[2] == c3 \
  && p[3] == c4 \
  && p[4] == c5 \
  && (p[5] == ' ' || p[5] == '\t'))

#define EQ6(c1,c2,c3,c4,c5,c6) \
    (p[0] == c1 \
  && p[1] == c2 \
  && p[2] == c3 \
  && p[3] == c4 \
  && p[4] == c5 \
  && p[5] == c6 \
  && (p[6] == ' ' || p[6] == '\t'))

#define EQ7(c1,c2,c3,c4,c5,c6,c7) \
    (p[0] == c1 \
  && p[1] == c2 \
  && p[2] == c3 \
  && p[3] == c4 \
  && p[4] == c5 \
  && p[5] == c6 \
  && p[6] == c7 \
  && (p[7] == ' ' || p[7] == '\t'))

#define EQ8(c1,c2,c3,c4,c5,c6,c7,c8) \
    (p[0] == c1 \
  && p[1] == c2 \
  && p[2] == c3 \
  && p[3] == c4 \
  && p[4] == c5 \
  && p[5] == c6 \
  && p[6] == c7 \
  && p[7] == c8 \
  && (p[8] == ' ' || p[8] == '\t'))

#define EQT7(c1,c2,c3,c4,c5,c6,c7) \
    (p[0] == c1 \
  && p[1] == c2 \
  && p[2] == c3 \
  && p[3] == c4 \
  && p[4] == c5 \
  && p[5] == c6 \
  && p[6] == c7)

#endif /* ply_commoh_h */