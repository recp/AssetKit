/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "assetio.h"
#include "aio_utils.h"

#include "collada/aio_collada.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <regex.h>
#include <libxml/tree.h>

int
aio_load(aio_doc ** __restrict dest,
         const char * __restrict file, ...) {
  int _err_no = 0;

  typedef struct {
    const char * fext;
    int (*floader_fn)(aio_doc ** __restrict,
                      const char * __restrict);
  } floader_t;

  va_list pref_args;
  va_start(pref_args, file);
  int file_type = va_arg(pref_args, int);
  va_end(pref_args);

  char * fcontents;
  floader_t * floader;

  if (aio_readfile(file, "rb", &fcontents) != 0)
    goto err;

  floader_t floaders[] = {
    {"dae", aio_load_collada}
  };

  if (file_type == AIO_FILE_TYPE_AUTO) {
    char * file_ext = strrchr(file, '.');
    if (file_ext) {
      ++file_ext;

      int i = 0;
      int floader_len = AIO_ARRAY_LEN(floaders);
      for (; i < floader_len; i++) {
        if (strcmp(file_ext, floaders[i].fext) == 0) {
          floader = &floaders[i];
          break;
        }
      }
    } else {
      /* TODO */
    }
  } else {
    switch (file_type) {
      case AIO_FILE_TYPE_COLLADA: {
        floader = &floaders[0];
        break;
      }
      case AIO_FILE_TYPE_WAVEFRONT:
        break;
      case AIO_FILE_TYPE_FBX:
        break;
      default:
        *dest = NULL;
        break;
    }
  }

  if (floader)
    _err_no = floader->floader_fn(dest, fcontents);
  else
    *dest = NULL;

  free(fcontents);
  
  return _err_no;
err:
  return -1;
}
