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

/*
 * TODO: maybe there is better way to implement sid addressing?
 */

#ifndef assetkit_sid_h
#define assetkit_sid_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "context.h"

AK_EXPORT
const char *
ak_sid_get(void *memnode);

AK_EXPORT
const char *
ak_sid_geta(void *memnode,
            void *memptr);

AK_EXPORT
void
ak_sid_dup(void *newMemnode,
           void *oldMemnode);

AK_EXPORT
void
ak_sid_set(void       *memnode,
           const char * __restrict sid);

AK_EXPORT
void
ak_sid_seta(void       *memnode,
            void       *memptr,
            const char * __restrict sid);

AK_EXPORT
void *
ak_sid_resolve(AkContext   * __restrict ctx,
               const char  * __restrict target,
               const char ** __restrict attribString);

AK_EXPORT
void *
ak_sid_resolve_from(AkContext   * __restrict ctx,
                    const char  * __restrict id,
                    const char  * __restrict target,
                    const char ** __restrict attribString);
  
AK_EXPORT
void *
ak_sid_resolve_val(AkContext  * __restrict ctx,
                   const char * __restrict target);

AK_EXPORT
uint32_t
ak_sid_attr_offset(const char *attr);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_sid_h */
