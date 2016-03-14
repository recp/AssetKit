/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/assetio.h"

#include "aio_utils.h"
#include "collada/aio_collada.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <libxml/tree.h>

typedef struct {
  const char * fext;
  int (*floader_fn)(aio_doc ** __restrict,
                    const char * __restrict);
} floader_t;

int
_assetio_export
aio_load(aio_doc ** __restrict dest,
         const char * __restrict file, ...) {
  floader_t * floader;
  int         file_type;
  int         _err_no;

  va_list pref_args;
  va_start(pref_args, file);
  file_type = va_arg(pref_args, int);
  va_end(pref_args);

  floader_t floaders[] = {
    {"dae", aio_dae_doc}
  };

  floader = NULL;

  if (file_type == AIO_FILE_TYPE_AUTO) {
    char * file_ext;
    file_ext = strrchr(file, '.');
    if (file_ext) {
      int floader_len;
      int i;

      ++file_ext;
      floader_len = AIO_ARRAY_LEN(floaders);
      for (i = 0; i < floader_len; i++) {
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
    _err_no = floader->floader_fn(dest, file);
  else
    goto err;

  return _err_no;
err:
  *dest = NULL;
  return -1;
}
