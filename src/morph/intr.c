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

#include "../common.h"

AK_EXPORT
void
ak_morphInterleaveInspect(size_t  * __restrict bufferSize,
                          size_t  * __restrict byteStride,
                          AkMorph * __restrict morph,
                          AkInputSemantic      desiredInputs[],
                          uint8_t              desiredInputsCount) {
  AkMorphTarget  *target;
  AkInput        *inp;
  AkAccessor     *acc;
  RBTree         *foundInputs;
  size_t          targetStride;
  uint32_t        i, count, foundInpCount;

  if (!(target = morph->target))
    return;

  foundInputs   = rb_newtree_ptr();
  foundInpCount = 0;
  targetStride  = 0;
  count         = 0;
  
  /* collect stride  for each each input */
  do {
    if (!(inp = target->input))
      continue;

    do {
      for (i = 0; i < desiredInputsCount; i++) {
        if (!rb_find(foundInputs, (void *)(uintptr_t)inp->semantic)
            && desiredInputs[i] == inp->semantic
            && (acc = inp->accessor)) {

          targetStride += acc->fillByteSize;

          rb_insert(foundInputs, (void *)(uintptr_t)inp->semantic, inp);

          if (acc->count > count)
            count = acc->count;

          if (++foundInpCount >= desiredInputsCount)
            goto calc;

          break;
        }
      }
    } while ((inp = inp->next));

    if (foundInpCount >= desiredInputsCount)
      goto calc;
  } while ((target = target->next));

calc:

  rb_destroy(foundInputs);

  targetStride *= morph->targetCount;

  if (byteStride)
    *byteStride = targetStride;

  if (bufferSize)
    *bufferSize = targetStride * count;

  return;
}

AK_EXPORT
void
ak_morphInterleave(void    * __restrict buff,
                   AkMorph * __restrict morph,
                   AkInputSemantic      desiredInputs[],
                   uint32_t             desiredInputsCount) {
  AkMorphTarget *target;
  AkInput       *inp;
  AkAccessor    *acc;
  AkBuffer      *buf;
  RBTree        *foundInputs;
  char          *pi, *pi_dest;
  size_t         inputByteStride, targetStride, *inpOffsets, byteOffset;
  size_t         inpOffset, targetOffset, compSize;
  uint32_t       i, count, foundInpCount, targetIndex, nTargets;
  bool           found;
  
  if (desiredInputsCount < 1 || !(target = morph->target))
    return;

  inpOffsets    = alloca(desiredInputsCount * sizeof(*inpOffsets));
  foundInputs   = rb_newtree_ptr();
  foundInpCount = 0;
  targetStride  = 0;
  targetIndex   = 0;
  nTargets      = morph->targetCount;
  
  memset(inpOffsets, 0, desiredInputsCount * sizeof(*inpOffsets));
  
  /* collect stride  for each each input */
  do {
    if (!(inp = target->input))
      continue;

    do {
      for (i = 0; i < desiredInputsCount; i++) {
        if (!rb_find(foundInputs, (void *)(uintptr_t)inp->semantic)
            && desiredInputs[i] == inp->semantic
            && (acc = inp->accessor)) {

          inpOffsets[i] = acc->fillByteSize;
          targetStride += acc->fillByteSize;

          rb_insert(foundInputs, (void *)(uintptr_t)inp->semantic, inp);

          if (++foundInpCount >= desiredInputsCount)
            goto calc_off;

          break;
        }
      }
    } while ((inp = inp->next));

    if (foundInpCount >= desiredInputsCount)
      goto calc_off;
  } while ((target = target->next));

  rb_destroy(foundInputs);

calc_off:

  inpOffset = 0;

  /* calc offsets */
  for (i = 0; i < desiredInputsCount; i++) {
    size_t tmp;
    tmp           = inpOffsets[i];
    inpOffsets[i] = inpOffset;
    inpOffset    += tmp * nTargets;
  }

  /* fill buffer */

  targetStride *= nTargets;
  target        = morph->target;

  do {
    if (!(inp = target->input))
      continue;

    do {
      found = false;

      for (i = 0; i < desiredInputsCount; i++) {
        if (desiredInputs[i] == inp->semantic) {
          found     = true;
          inpOffset = inpOffsets[i];
          break;
        }
      }

      if (!found
          || !(acc = inp->accessor)
          || !(buf = acc->buffer)
          || !(pi  = buf->data))
        continue;

      count           = acc->count;
      inputByteStride = acc->byteStride;
      compSize        = acc->fillByteSize;
      byteOffset      = acc->byteOffset;
      targetOffset    = targetIndex * compSize;

      pi     += byteOffset;
      pi_dest = (char *)buff;

      for (i = 0; i < count; i++) {
        memcpy(pi_dest + targetStride * i + inpOffset + targetOffset,
               pi + inputByteStride * i,
               compSize);
      }
    } while ((inp = inp->next));

    targetIndex++;
  } while ((target = target->next));
}
