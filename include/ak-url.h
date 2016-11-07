/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_url_h
#define ak_url_h

typedef struct AkURL {
  const char   *url;      /* only fragment */
  struct AkDoc *doc;      /* document      */
  void         *reserved; /* private       */
} AkURL;

void
ak_url_init(void  *parent,
            char  *urlstring,
            AkURL *dest);

void
ak_url_ref(AkURL *url);

void
ak_url_unref(AkURL *url);

#endif /* ak_url_h */
