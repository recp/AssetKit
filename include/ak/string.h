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

#ifndef assetkit_string_h
#define assetkit_string_h

#include "common.h"
#include "core-types.h"

AK_EXPORT
const char*
ak_strltrim_fast(const char * __restrict str);

/*!
 * @brief util for count tokens before call strtok to save realloc calls
 *
 * @param[in]  buff string buffer
 * @param[in]  sep  separators use comma to use multiple e.g. " \t\r"
 * @param[out] len  non-separator char count
 *
 * @return returns word count
 */
AK_EXPORT
int
ak_strtok_count(char * __restrict buff,
                char * __restrict sep,
                size_t           *len);

AK_EXPORT
int
ak_strtok_count_fast(char * __restrict buff,
                     size_t            srclen,
                     size_t           *len);


AK_EXPORT
unsigned long
ak_strtof(char    * __restrict src,
          size_t               srclen,
          unsigned long        n,
          AkFloat * __restrict dest);

AK_EXPORT
unsigned long
ak_strtof_line(char    * __restrict src,
               size_t               srclen,
               unsigned long        n,
               AkFloat * __restrict dest);

AK_EXPORT
unsigned long
ak_strtod(char     * __restrict src,
          size_t                srclen,
          unsigned long         n,
          AkDouble * __restrict dest);

AK_EXPORT
unsigned long
ak_strtoui(char    * __restrict src,
           size_t               srclen,
           unsigned long        n,
           AkUInt  * __restrict dest);

AK_EXPORT
unsigned long
ak_strtoi(char    * __restrict src,
          size_t               srclen,
          unsigned long        n,
          AkInt   * __restrict dest);

AK_EXPORT
unsigned long
ak_strtoi_line(char    * __restrict src,
               size_t               srclen,
               unsigned long        n,
               AkInt   * __restrict dest);

AK_EXPORT
unsigned long
ak_strtob(char    * __restrict src,
          size_t               srclen,
          unsigned long        n,
          AkBool  * __restrict dest);

AK_EXPORT
char*
ak_tolower(char *str);

AK_EXPORT
char*
ak_toupper(char *str);

#endif /* assetkit_string_h */
