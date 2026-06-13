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

#ifndef dae_brep_semantic_h
#define dae_brep_semantic_h

#include "../../../string_fast.h"

#define DAE_BREP_SEM_CURVE                                                  \
  AK_STR_PACK8_CHARS('C', 'U', 'R', 'V', 'E', 0, 0, 0)
#define DAE_BREP_SEM_CURVE2D                                                \
  AK_STR_PACK8_CHARS('C', 'U', 'R', 'V', 'E', '2', 'D', 0)
#define DAE_BREP_SEM_SURFACE                                                \
  AK_STR_PACK8_CHARS('S', 'U', 'R', 'F', 'A', 'C', 'E', 0)
#define DAE_BREP_SEM_SHELL                                                  \
  AK_STR_PACK8_CHARS('S', 'H', 'E', 'L', 'L', 0, 0, 0)
#define DAE_BREP_SEM_EDGE  AK_STR_PACK4_CHARS('E', 'D', 'G', 'E')
#define DAE_BREP_SEM_EGDE  AK_STR_PACK4_CHARS('E', 'G', 'D', 'E')
#define DAE_BREP_SEM_WIRE  AK_STR_PACK4_CHARS('W', 'I', 'R', 'E')
#define DAE_BREP_SEM_FACE  AK_STR_PACK4_CHARS('F', 'A', 'C', 'E')
#define DAE_BREP_SEM_PARAM                                                  \
  AK_STR_PACK8_CHARS('P', 'A', 'R', 'A', 'M', 0, 0, 0)

#endif /* dae_brep_semantic_h */
