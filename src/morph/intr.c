/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
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
  size_t          targetByteStride, *inpSizes;
  uint32_t        i, count, foundInpCount;
  
  if (!(target = morph->target))
    return;
  
  inpSizes       = alloca(desiredInputsCount * sizeof(uint32_t));
  foundInpCount    = 0;
  targetByteStride = 0;
  count            = 0;
  
  /* collect stride  for each each input */
  do {
    if (!(inp = target->input))
      continue;

    do {
      for (i = 0; i < desiredInputsCount; i++) {
        if (desiredInputs[i] == inp->semantic
            && (acc = inp->accessor)) {

          inpSizes[i]       = acc->fillByteSize;
          targetByteStride += acc->fillByteSize;

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

  if (byteStride)
    *byteStride = targetByteStride;

  if (bufferSize)
    *bufferSize = targetByteStride * count;

  return;
}

AK_EXPORT
void
ak_morphInterleave(void    * __restrict buff,
                   AkMorph * __restrict morph,
                   AkInputSemantic      desiredInputs[],
                   uint32_t             desiredInputsCount) {
  AkMorphTarget  *target;
  AkInput        *inp;
  AkAccessor     *acc;
  AkBuffer       *buf;
  char           *pi;
  size_t          inputByteStride, targetByteStride, *inpOffsets;
  size_t          inpOffset, compSize;
  uint32_t        i, count, foundInpCount;
  bool            found;
  
  if (!(target = morph->target))
    return;

  inpOffsets       = alloca(desiredInputsCount * sizeof(*inpOffsets));
  inpOffset        = 0;
  foundInpCount    = 0;
  targetByteStride = 0;
  
  memset(inpOffsets, 0, desiredInputsCount * sizeof(*inpOffsets));
  
  /* collect stride  for each each input */
  do {
    if (!(inp = target->input))
      continue;

    do {
      for (i = 0; i < desiredInputsCount; i++) {
        if (desiredInputs[i] == inp->semantic
            && (acc = inp->accessor)) {

          inpOffsets[i]     = acc->fillByteSize;
          targetByteStride += acc->fillByteSize;

          if (++foundInpCount >= desiredInputsCount)
            goto calc_off;
          
          break;
        }
      }
    } while ((inp = inp->next));
    
    if (foundInpCount >= desiredInputsCount)
      goto calc_off;
  } while ((target = target->next));
  
calc_off:
  /* calc offsets */
  for (i = 1; i < desiredInputsCount; i++)
    inpOffsets[i] += inpOffsets[i - 1];

  /* fill buffer */

  target = morph->target;

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
      
      for (i = 0; i < count; i++) {
        memcpy((char *)buff + targetByteStride * i + inpOffset,
               pi   + inputByteStride * i,
               compSize);
      }
    } while ((inp = inp->next));
  } while ((target = target->next));
}
