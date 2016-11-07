/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_resource.h"
#include "../ak_memory_common.h"
#include "../../include/ak-path.h"
#include "ak_curl.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

extern AkHeapAllocator ak__allocator;

static AkHeap ak__resc_heap = {
  .allocator  = &ak__allocator,
  .srchctx    = NULL,
  .root       = NULL,
  .trash      = NULL,
  .flags      = 0
};

#define resc_heap &ak__resc_heap

AkDoc *
ak_resc_ins(const char *url) {
  void       *pFoundResc;
  AkResource *resc, *foundResc;
  char       *tmp, *trimmedURL;
  AkResult    ret;

  tmp = malloc(sizeof(*tmp) * strlen(url));

  ak_path_trim(url, tmp);
  trimmedURL = ak_heap_strdup(resc_heap, NULL, tmp);
  free(tmp);

  ret = ak_heap_getMemById(resc_heap,
                           trimmedURL,
                           &pFoundResc);

  if (ret != AK_EFOUND) {
    foundResc = pFoundResc;
    foundResc->refc++;
    return foundResc->doc;
  }

  resc = ak_heap_calloc(resc_heap,
                        NULL,
                        sizeof(*resc),
                        true);

  ak_heap_setId(resc_heap,
                ak__alignof(resc),
                trimmedURL);

  resc->refc++;
  resc->url = ak_heap_strdup(resc_heap, resc, url);

  if (ak_path_isfile(url)) {
    resc->isdwn    = true;
    resc->islocal  = true;
    resc->localurl = url;
    resc->result   = ak_load(&resc->doc,
                             url,
                             AK_FILE_TYPE_COLLADA);
    return resc->doc;
  }

  /* download the file, file must be downloadable, e.g not live stream
   * to do this the remote file must send file size
   */

  /* TODO: check preferences, user may want to download manually */
  resc->localurl = ak_curl_dwn(url);
  ak_heap_setpm(ak_heap_getheap((void *)resc->localurl),
                (void *)resc->localurl,
                resc);

  resc->result = ak_load(&resc->doc,
                         url,
                         AK_FILE_TYPE_COLLADA);
  return resc->doc;
}

void
ak_resc_unref(AkResource *resc) {
  resc->refc--;
  if (resc->refc <= 0) {
    if (resc->doc)
      ak_free(resc->doc);

    ak_free(resc);
  }
}

void
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

    if (ret != AK_EFOUND)
      ak_resc_unref(resc);

    free(trimmed);
  }
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
