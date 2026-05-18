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

#include <assert.h>

extern const char* ak_mesh_edit_assert1;

AK_EXPORT
AkResult
ak_meshFillBuffers(AkMesh * __restrict mesh) {
  AkInput            *input;
  AkMeshPrimitive    *primi;
  AkAccessor         *acc, *newacc;
  AkIndexArray       *ind1, *ind2;
  AkBuffer           *oldbuff, *newbuff;
  AkSourceBuffState  *buffstate;
  AkSourceEditHelper *srch;
  char               *olditms, *newitms;
  size_t              icount, i;
  AkUInt              oldidx, newidx, oldByteSt, newByteSt, fillSize, indxSt,
                      inpOff;

  assert(mesh->edith && ak_mesh_edit_assert1);
  primi = mesh->primitive;
 
  /* per-primitive inputs */
  while (primi) {
    ind1 = primi->indices;
    ind2 = ak_meshIndicesArrayFor(mesh, primi, false);

    /* same index buff */
    if (!ind1 || ind1 == ind2 || !ind2) {
      primi = primi->next;
      continue;
    }

    input = primi->input;

    while (input) {
      if (input->semantic == AK_INPUT_POSITION
          || !(acc     = input->accessor)
          || !(oldbuff = acc->buffer))
        goto cont;

      /* copy buff to mesh */
      if ((buffstate = input->reserved)) {
        srch      = buffstate->sourceEdit;
        if (!srch)
          goto cont;
        newbuff   = buffstate->buff;
        newacc    = srch->source;
        oldByteSt = (AkUInt)acc->byteStride;
        newByteSt = (AkUInt)newacc->byteStride;
        fillSize  = (AkUInt)acc->fillByteSize;

        assert(newacc && "accessor is needed!");

        inpOff  = input->offset;
        indxSt  = primi->indexStride;
        icount  = ind1->count / indxSt;
        newitms = (char *)newbuff->data + newacc->byteOffset;
        olditms = (char *)oldbuff->data + acc->byteOffset;

#define AK_FILL_BUFFER_COPY_LOOP(DSTTYPE, TYPE, SRC, SIZE)                  \
        do {                                                                 \
          const DSTTYPE *dst_;                                               \
          const TYPE *src_;                                                  \
                                                                             \
          dst_ = (const DSTTYPE *)(const void *)ind2->items;                 \
          src_ = (const TYPE *)(const void *)(SRC);                          \
          for (i = 0; i < icount; i++) {                                     \
            oldidx = (AkUInt)src_[i * indxSt + inpOff];                      \
            newidx = (AkUInt)dst_[i];                                        \
                                                                             \
            memcpy(newitms + newByteSt * newidx,                             \
                   olditms + oldByteSt * oldidx,                             \
                   SIZE);                                                    \
          }                                                                  \
        } while (0)

#define AK_FILL_BUFFER_FOR_INDEX_TYPE(DSTTYPE, TYPE, SRC)                    \
        do {                                                                 \
          switch (fillSize) {                                                \
            case 4:                                                          \
              AK_FILL_BUFFER_COPY_LOOP(DSTTYPE, TYPE, SRC, 4);              \
              break;                                                         \
            case 8:                                                          \
              AK_FILL_BUFFER_COPY_LOOP(DSTTYPE, TYPE, SRC, 8);              \
              break;                                                         \
            case 12:                                                         \
              AK_FILL_BUFFER_COPY_LOOP(DSTTYPE, TYPE, SRC, 12);             \
              break;                                                         \
            case 16:                                                         \
              AK_FILL_BUFFER_COPY_LOOP(DSTTYPE, TYPE, SRC, 16);             \
              break;                                                         \
            default:                                                         \
              AK_FILL_BUFFER_COPY_LOOP(DSTTYPE, TYPE, SRC, fillSize);        \
              break;                                                         \
          }                                                                  \
        } while (0)

#define AK_FILL_BUFFER_FOR_OUTPUT_TYPE(DSTTYPE)                              \
        do {                                                                 \
          switch (ind1->componentType) {                                      \
            case AKT_UBYTE:                                                  \
              AK_FILL_BUFFER_FOR_INDEX_TYPE(DSTTYPE, uint8_t, ind1->items);  \
              break;                                                         \
            case AKT_USHORT:                                                 \
              AK_FILL_BUFFER_FOR_INDEX_TYPE(DSTTYPE, uint16_t, ind1->items); \
              break;                                                         \
            case AKT_UINT:                                                   \
              AK_FILL_BUFFER_FOR_INDEX_TYPE(DSTTYPE, AkUInt, ind1->items);   \
              break;                                                         \
            default:                                                         \
              break;                                                         \
          }                                                                  \
        } while (0)

        switch (ind2->componentType) {
          case AKT_UBYTE:
            AK_FILL_BUFFER_FOR_OUTPUT_TYPE(uint8_t);
            break;
          case AKT_USHORT:
            AK_FILL_BUFFER_FOR_OUTPUT_TYPE(uint16_t);
            break;
          case AKT_UINT:
            AK_FILL_BUFFER_FOR_OUTPUT_TYPE(AkUInt);
            break;
          default:
            break;
        }

#undef AK_FILL_BUFFER_FOR_OUTPUT_TYPE
#undef AK_FILL_BUFFER_FOR_INDEX_TYPE
#undef AK_FILL_BUFFER_COPY_LOOP

        /* to prevent duplication operation for next time */
        input->offset = 0;
      }

    cont:
      input = input->next;
    }

    primi = primi->next;
  }

  return AK_OK;
}
