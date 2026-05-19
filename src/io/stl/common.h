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

#ifndef stl_commoh_h
#define stl_commoh_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../json.h"
#include "../../data.h"
#include "../../string_fast.h"

#include <string.h>
#include <stdlib.h>

#define STL_DATA_NODE_ITEMS 4096

typedef struct STLState {
  AkHeap        *heap;
  AkDoc         *doc;
  void          *tmp;
  AkLibrary     *lib_geom;
  AkGeometry    *geom;
  AkDataContext *dc_ind, *dc_pos, *dc_nor, *dc_vcount;
  AkBuffer      *buff_pos, *buff_nor, *buff_col;
  AkIndexArray  *raw_index_array;
  uint32_t      *raw_indices;
  AkNode        *node;
  uint32_t       indexCount;
  uint32_t       indexMax;
  uint32_t       maxVC;
  uint32_t       count;
} STLState;

#ifdef SKIP_SPACES
# undef SKIP_SPACES
#endif

#define SKIP_SPACES                                                           \
  {                                                                           \
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

#define STL_EQ4(c1,c2,c3,c4) \
    (ak_str_pack4_ci_fast(p, 4) == AK_STR_PACK4_CHARS(c1, c2, c3, c4) \
  && (p[4] == ' ' || p[4] == '\t'))

#define STL_EQ5(c1,c2,c3,c4,c5) \
    (ak_str_pack4_ci_fast(p, 4) == AK_STR_PACK4_CHARS(c1, c2, c3, c4) \
  && ak_str_ascii_lower_fast(p[4]) == c5 \
  && (p[5] == ' ' || p[5] == '\t'))

#define STL_EQ6(c1,c2,c3,c4,c5,c6) \
    (ak_str_pack4_ci_fast(p, 4) == AK_STR_PACK4_CHARS(c1, c2, c3, c4) \
  && ak_str_ascii_lower_fast(p[4]) == c5 \
  && ak_str_ascii_lower_fast(p[5]) == c6 \
  && (p[6] == ' ' || p[6] == '\t'))

#define STL_EQ7(c1,c2,c3,c4,c5,c6,c7) \
    (ak_str_pack4_ci_fast(p, 4) == AK_STR_PACK4_CHARS(c1, c2, c3, c4) \
  && ak_str_ascii_lower_fast(p[4]) == c5 \
  && ak_str_ascii_lower_fast(p[5]) == c6 \
  && ak_str_ascii_lower_fast(p[6]) == c7 \
  && (p[7] == ' ' || p[7] == '\t'))

#define STL_EQT7(c1,c2,c3,c4,c5,c6,c7) \
    (ak_str_pack4_ci_fast(p, 4) == AK_STR_PACK4_CHARS(c1, c2, c3, c4) \
  && ak_str_ascii_lower_fast(p[4]) == c5 \
  && ak_str_ascii_lower_fast(p[5]) == c6 \
  && ak_str_ascii_lower_fast(p[6]) == c7)

#define STL_EQT8(c1,c2,c3,c4,c5,c6,c7,c8) \
    (ak_str_pack8_ci_fast(p, 8) == AK_STR_PACK8_CHARS(c1, c2, c3, c4, \
                                                      c5, c6, c7, c8))

#endif /* stl_commoh_h */
