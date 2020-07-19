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

#include "common.h"

AK_HIDE
void
ak_dsSetAllocator(AkHeapAllocator * __restrict alc,
                  DsAllocator     * __restrict dsalc) {
  dsalc->calloc   = alc->calloc;
  dsalc->free     = alc->free;
  dsalc->malloc   = alc->malloc;
  dsalc->memalign = alc->memalign;
  dsalc->realloc  = alc->realloc;
  dsalc->size     = alc->size;
  dsalc->strdup   = alc->strdup;
}
