/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../include/ak/assetkit.h"

#include "utils.h"
#include "collada/dae.h"
#include "gltf/gltf.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <libxml/tree.h>

typedef struct {
  const char * fext;
  AkResult (*floader_fn)(AkDoc ** __restrict,
                    const char * __restrict);
} floader_t;

AK_EXPORT
AkResult
ak_load(AkDoc ** __restrict dest,
         const char * __restrict url, ...) {
  floader_t  *floader;
  const char *localurl;
  int         file_type;
  int         _err_no;

  va_list pref_args;
  va_start(pref_args, url);
  file_type = va_arg(pref_args, int);
  va_end(pref_args);

  localurl = ak_getFile(url);
  if (!localurl)
    return AK_EBADF;

  floader_t floaders[] = {
    {"dae",  dae_doc},
    {"gltf", gltf_doc}
  };

  floader = NULL;

  if (file_type == AK_FILE_TYPE_AUTO) {
    char * file_ext;
    file_ext = strrchr(localurl, '.');
    if (file_ext) {
      int floader_len;
      int i;

      ++file_ext;
      floader_len = AK_ARRAY_LEN(floaders);
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
      case AK_FILE_TYPE_COLLADA: {
        floader = &floaders[0];
        break;
      }
      case AK_FILE_TYPE_GLTF: {
        floader = &floaders[1];
        break;
      }
      case AK_FILE_TYPE_WAVEFRONT:
        break;
      case AK_FILE_TYPE_FBX:
        break;
      default:
        *dest = NULL;
        break;
    }
  }

  if (floader)
    _err_no = floader->floader_fn(dest, localurl);
  else
    goto err;

  return _err_no;
err:
  *dest = NULL;
  return AK_ERR;
}
