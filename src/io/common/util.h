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

#ifndef io_common_util_h
#define io_common_util_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../json.h"
#include "../../data.h"

#include <string.h>
#include <stdlib.h>

AK_HIDE
AkMesh*
ak_allocMesh(AkHeap      * __restrict heap,
             AkLibrary   * __restrict memp,
             AkGeometry ** __restrict geomLink);

AK_HIDE
AkInput*
io_addInput(AkHeap          * __restrict heap,
            AkDataContext   * __restrict dctx,
            AkMeshPrimitive * __restrict prim,
            AkInputSemantic              sem,
            const char      * __restrict semRaw,
            AkComponentSize              compSize,
            AkTypeId                     type,
            uint32_t                     offset);

AK_HIDE
AkAccessor*
io_acc(AkHeap          * __restrict heap,
       AkDoc           * __restrict doc,
       AkComponentSize              compSize,
       AkTypeId                     type,
       uint32_t                     count,
       AkBuffer        * __restrict buff);

AK_HIDE
AkInput*
io_input(AkHeap          * __restrict heap,
         AkMeshPrimitive * __restrict prim,
         AkAccessor      * __restrict acc,
         AkInputSemantic              sem,
         const char      * __restrict semRaw,
         uint32_t                     offset);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define le_uint32(X, DATA) \
  do {                                                                        \
    memcpy(&X, DATA, sizeof(uint32_t));                                       \
    DATA = (char *)DATA + sizeof(uint32_t);                                   \
  } while (0)
#else
# include <arpa/inet.h>
AK_INLINE
uint32_t
bswapu32(uint32_t val) {
  
  /*
   val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
   return (val << 16) | (val >> 16);
   */

  #if defined(__llvm__)
    return __builtin_bswap32(val);
  #else
    __asm__ ("bswap   %0" : "+r" (val));
    return val;
  #endif
}
# define le_uint32(X, DATA)                                                  \
  do {                                                                        \
    memcpy(&X, DATA, sizeof(uint32_t));                                       \
    X    = bswapu32(magic);                                                   \
    X    = ntohl(magic);                                                      \
    DATA = (char *)DATA + sizeof(uint32_t);                                   \
  } while (0)
#endif

#endif /* io_common_util_h */
