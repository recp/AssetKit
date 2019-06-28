/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_url_h
#define ak_url_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef struct AkURL {
  const char   *url;      /* only fragment */
  struct AkDoc *doc;      /* document      */
  void         *ptr;      /* direct link   */
  void         *reserved; /* private       */
} AkURL;

void
ak_url_init(void  *parent,
            char  *urlstring,
            AkURL *dest);

void
ak_url_dup(AkURL *src,
           void  *parent,
           AkURL *dest);

void
ak_url_init_with_id(AkHeapAllocator *alc,
                    void            *parent,
                    char            *idstirng,
                    AkURL           *dest);

char *
ak_url_string(AkHeapAllocator *alc, char *id);

void
ak_url_ref(AkURL *url);

void
ak_url_unref(AkURL *url);

#ifdef __cplusplus
}
#endif
#endif /* ak_url_h */
