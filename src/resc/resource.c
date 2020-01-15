/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "resource.h"
#include "../mem_common.h"
#include "../../include/ak/path.h"
#include "curl.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

static AkHeap ak__resc_heap = {
  .flags = 0
};

#define resc_heap &ak__resc_heap

AkResource *
ak_resc_ins(const char *url) {
  void       *pFoundResc;
  AkResource *resc, *foundResc;
  char       *tmp, *trimmedURL;
  const char *fragment;
  size_t      trimmedURLSize;
  AkResult    ret;

  tmp = malloc(sizeof(*tmp) * strlen(url));

  ak_path_trim(url, tmp);

  fragment = ak_path_fragment(tmp);
  if (strlen(fragment) > 0)
    trimmedURLSize = fragment - tmp;
  else
    trimmedURLSize = strlen(tmp);

  trimmedURL = ak_heap_strndup(resc_heap,
                               NULL,
                               tmp,
                               trimmedURLSize);
  free(tmp);

  ret = ak_heap_getMemById(resc_heap,
                           trimmedURL,
                           &pFoundResc);

  if (ret != AK_EFOUND) {
    foundResc = pFoundResc;
    foundResc->refc++;

    ak_free(trimmedURL);
    return foundResc;
  }

  resc = ak_heap_calloc(resc_heap,
                        NULL,
                        sizeof(*resc));

  resc->url = trimmedURL;
  ak_heap_setpm(trimmedURL, resc);
  ak_heap_setId(resc_heap,
                ak__alignof(resc),
                trimmedURL);

  resc->refc++;

  if (ak_path_isfile(url)) {
    resc->isdwn    = true;
    resc->islocal  = true;
    resc->localurl = resc->url;
    resc->result   = ak_load(&resc->doc,
                             resc->url,
                             AK_FILE_TYPE_COLLADA);
    return resc;
  }

  /* download the file, file must be downloadable, e.g not live stream
   * to do this the remote file must send file size
   */

  /* TODO: check preferences, user may want to download manually */
  resc->localurl = ak_curl_dwn(resc->url);
  ak_heap_setpm((void *)resc->localurl, resc);

  resc->result = ak_load(&resc->doc,
                         resc->url,
                         AK_FILE_TYPE_COLLADA);
  return resc;
}

void
ak_resc_ref(AkResource *resc) {
  resc->refc++;
}

int
ak_resc_unref(AkResource *resc) {
  resc->refc--;
  if (resc->refc <= 0) {
    if (resc->doc)
      ak_free(resc->doc);

    ak_free(resc);

    return -1;
  }

  return 0;
}

int
ak_resc_unref_url(const char *url) {
  void    *resc;
  char    *trimmed;
  AkResult ret;

  trimmed = NULL;
  ak_path_trim(url, trimmed);

  if (trimmed) {
    ret = ak_heap_getMemById(resc_heap,
                             trimmed,
                             &resc);

    free(trimmed);

    if (ret != AK_EFOUND)
      return ak_resc_unref(resc);
  }

  return -1;
}

void
ak_resc_print() {
  ak_heap_printKeys(resc_heap);
}

void _assetkit_hide
ak_resc_init() {
  ak_heap_init(resc_heap, NULL, NULL, NULL);
}

void _assetkit_hide
ak_resc_deinit() {
  ak_heap_destroy(resc_heap);
}
