/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_path_h
#define ak_path_h

AK_EXPORT
size_t
ak_path_trim(char *path,
             char *trimmed);

AK_EXPORT
int
ak_path_join(char   *fragments[],
             char   *buf,
             size_t *size);

#endif /* ak_path_h */
