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

#ifndef ak_io_lgsc_h
#define ak_io_lgsc_h

#include "../common/util.h"

/* Slots: position, rotation, scale, opacity, then RGB SH sets 0..15. */
typedef struct AkLGSCData {
  AkBuffer *buffer;
  size_t    offsets[20];
  size_t    strides[20];
  uint32_t  count;
  uint32_t  degree;
} AkLGSCData;

AK_HIDE
bool
lgsc_decode(AkHeap *heap, void *parent, const uint8_t *bytes, size_t size,
            uint32_t degree, uint32_t expectedCount, bool rdf, AkLGSCData *out);

AK_HIDE
void
lgsc_accessor(AkAccessor *acc, const AkLGSCData *data, uint32_t slot);

AK_HIDE
AkResult
lgsc_doc(AkDoc **dest, const char *filepath);

#endif /* ak_io_lgsc_h */
