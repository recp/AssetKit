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

#include "resource.h"
#include "../../include/ak/path.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

static AkFetchFromURLHandler ak__urlhandler = NULL;

static AkHeap ak__resc_heap = {
  .flags = 0
};

#define resc_heap &ak__resc_heap

AK_EXPORT
void
ak_setFetchFromURLHandler(AkFetchFromURLHandler handler) {
  ak__urlhandler = handler;
}

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

  if (!ak__urlhandler) {
    ak_free(resc);
    return NULL;
  }

  resc->localurl = ak__urlhandler(resc->url);
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

AK_HIDE
void
ak_resc_init() {
  ak_heap_init(resc_heap, NULL, NULL, NULL);
}

AK_HIDE
void
ak_resc_deinit() {
  ak_heap_destroy(resc_heap);
}
