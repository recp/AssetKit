/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../../include/ak-path.h"
#include "ak_resource.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

void
ak_url_init(void  *parent,
            char  *urlstring,
            AkURL *dest) {

  assert(parent && "parent must be malloced by ak_heap_alloc/calloc");

  if (*urlstring == '#') {
    AkHeap *heap;

    dest->reserved = NULL;
    heap           = ak_heap_getheap(parent);
    dest->doc      = ak_heap_data(heap);
    dest->url      = ak_heap_strdup(heap,
                                    parent,
                                    urlstring);
    return;
  }

  dest->reserved = ak_resc_ins(urlstring);
  dest->url      = ak_path_fragment(urlstring);
  dest->doc      = ((AkResource *)dest->reserved)->doc;
}

void
ak_url_init_with_id(AkHeapAllocator *alc,
                    void            *parent,
                    char            *idstirng,
                    AkURL           *dest) {
  char *urlstring;
  urlstring = ak_url_string(alc, idstirng);
  ak_url_init(parent, urlstring, dest);
  alc->free(urlstring);
}

char *
ak_url_string(AkHeapAllocator *alc, char *id) {
  char *urlstring;

  urlstring  = alc->malloc(strlen(id) + 2);
  *urlstring = '#';
  strcpy(urlstring + 1, id);

  return urlstring;
}

void
ak_url_ref(AkURL *url) {
  if (!url->reserved)
    return;

  ak_resc_ref(url->reserved);
}

void
ak_url_unref(AkURL *url) {
  if (!url->reserved)
    return;

  if (ak_resc_unref(url->reserved) < 0) {
    url->reserved = NULL;
    url->url      = NULL;
  }
}

AK_EXPORT
void *
ak_getObjectByUrl(AkURL * __restrict url) {
  if (url->doc)
    return ak_getObjectById(url->doc, url->url + 1);

  return NULL;
}
