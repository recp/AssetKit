/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__assetkit_string__h_
#define __libassetkit__assetkit_string__h_

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
size_t
ak_strtok_count(char * __restrict buff,
                char * __restrict sep,
                size_t           *len);

AK_EXPORT
int
ak_strtod(char ** __restrict src,
          AkDouble * __restrict dest,
          unsigned long n);

AK_EXPORT
int
ak_strtof(char ** __restrict src,
           AkFloat * __restrict dest,
           unsigned long n);

AK_EXPORT
int
ak_strtomb(char ** __restrict src,
            AkBool * __restrict dest,
            unsigned long m,
            unsigned long n);

AK_EXPORT
int
ak_strtomi(char ** __restrict src,
            AkInt * __restrict dest,
            unsigned long m,
            unsigned long n);

AK_EXPORT
int
ak_strtomf(char ** __restrict src,
            AkFloat * __restrict dest,
            unsigned long m,
            unsigned long n);

AK_EXPORT
int
ak_strtof_s(const char * __restrict src,
             AkFloat * __restrict dest,
             unsigned long n);

AK_EXPORT
extern
int
ak_strtof4(char ** __restrict src,
            AkFloat4 * __restrict dest);

AK_EXPORT
extern
int
ak_strtof4_s(const char * __restrict src,
              AkFloat4 * __restrict dest);

#endif /* __libassetkit__assetkit_string__h_ */
