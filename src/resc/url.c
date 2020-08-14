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

#include "../common.h"
#include "../../include/ak/path.h"
#include "resource.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#ifdef _MSC_VER
#  ifndef PATH_MAX
#    define PATH_MAX MAX_PATH
#  endif
#endif

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

  /* TODO: */
  dest->reserved = ak_resc_ins(urlstring);
  dest->url      = ak_path_fragment(urlstring);
  dest->doc      = ((AkResource *)dest->reserved)->doc;
}

void
ak_url_dup(AkURL *src,
           void  *parent,
           AkURL *dest) {

  assert(parent && "parent must be malloced by ak_heap_alloc/calloc");

  memcpy(dest, src, sizeof(AkURL));
  if (!src->reserved) {
    AkHeap *heap;
    heap      = ak_heap_getheap(parent);
    dest->url = ak_heap_strdup(heap, parent, dest->url);
    return;
  }

  /* TODO: */
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
  if (url->ptr)
    return url->ptr;

  if (url->doc)
    return ak_getObjectById(url->doc, url->url + 1);

  return NULL;
}

const char*
ak_getFile(const char *url) {
  if (ak_path_isfile(url))
    return url;

  /* download the file, file must be downloadable, e.g not live stream
   * to do this the remote file must send file size
   */

  /* return local URL
   
     TODO: option for cache time,
           option for how to store this file,
           option for when to remove it
   */
  return NULL;
}

char*
ak_getFileFrom(AkDoc *doc, const char *url) {
  char        pathbuf[PATH_MAX];
  const char *path;

  path = ak_fullpath(doc, url, pathbuf);
  return ak_strdup(NULL, path);
}

AK_EXPORT
void
ak_retainURL(void * __restrict obj, AkURL * __restrict url) {
  AkHeap     *heap;
  AkHeapNode *hnode;
  AkUrlNode  *urlNode;
  int        *refc;
  void       *found, *last, **emptyslot;
  size_t      len;
  AkResult    ret;

  heap  = ak_heap_getheap(obj);
  hnode = ak__alignof(obj);

  /* check if object is available */
  ret = ak_heap_getNodeByURL(heap, url, &hnode);
  if (ret != AK_OK || !hnode)
    return;

  urlNode   = ak_heap_ext_add(heap, hnode, AK_HEAP_NODE_FLAGS_URL);
  refc      = ak_heap_ext_add(heap, hnode, AK_HEAP_NODE_FLAGS_REFC);
  len       = urlNode->len;
  found     = urlNode->urls[0];
  last      = urlNode->urls[len];
  emptyslot = NULL;

  while (found != last) {
    /* already retained */
    if (found == url)
      return;

    if (found == NULL && !emptyslot)
      emptyslot = found;
  }

  /* retain */
  (*refc)++;

  if (emptyslot) {
    *emptyslot = url;
    return;
  }

  /* append url to retained url lists */
  urlNode->len  = len + 1;
  urlNode->urls = heap->allocator->realloc(urlNode->urls,
                                           sizeof(void *) * urlNode->len);

  urlNode->urls[len] = url;
}

AK_EXPORT
void
ak_releaseURL(void * __restrict obj, AkURL * __restrict url) {
  AkHeapNode *hnode;
  AkUrlNode  *urlNode;
  void       *urlobj;
  void       **found, **it, *last;
  size_t      len;

  hnode = ak__alignof(obj);

  /* check if object is available */
  if (!(urlobj = ak_getObjectByUrl(url)))
    return;

  urlNode = ak_heap_ext_get(hnode, AK_HEAP_NODE_FLAGS_URL);
  len     = urlNode->len;
  it      = urlNode->urls[0];
  last    = urlNode->urls[len];
  found   = NULL;

  while (it != last) {
    /* already retained */
    if (*it == url) {
      found = it;
      break;
    }

    it++;
  }

  if (!found)
    return;

  /* empty slot */
  *found = NULL;

  ak_release(urlobj);
}
