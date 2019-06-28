/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_path_h
#define ak_path_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include <stdio.h>

struct AkHeap;
struct AkDoc;

AK_EXPORT
const char *
ak_path_fragment(const char *path);

AK_EXPORT
size_t
ak_path_trim(const char *path,
             char *trimmed);

AK_EXPORT
int
ak_path_join(char   *fragments[],
             char   *buf,
             size_t *size);

AK_EXPORT
int
ak_path_isfile(const char *path);

AK_EXPORT
char*
ak_path_dir(struct AkHeap * __restrict heap,
            void          * __restrict memparent,
            const char    * __restrict path);

AK_EXPORT
const char*
ak_fullpath(struct AkDoc * __restrict doc,
            const char   * __restrict ref,
            char         * __restrict buf);

AK_EXPORT
FILE *
ak_path_tmpfile(void);

AK_EXPORT
char *
ak_path_tmpfilepath(void);

#ifdef __cplusplus
}
#endif
#endif /* ak_path_h */
