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

#ifndef assetkit_url_h
#define assetkit_url_h
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
#endif /* assetkit_url_h */
