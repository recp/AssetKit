/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_path_h
#define ak_path_h

#include <stdio.h>

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
FILE *
ak_path_tmpfile();

AK_EXPORT
char *
ak_path_tmpfilepath();

#endif /* ak_path_h */
