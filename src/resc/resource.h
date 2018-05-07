/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_resource_h
#define ak_resource_h

#include "../common.h"
#include <time.h>

typedef struct AkResource {
  AkDoc      *doc;
  const char *url;
  const char *localurl;
  int64_t     refc;
  time_t      dwntime;
  bool        isdwn;
  bool        islocal;
  AkResult    result;
} AkResource;

void _assetkit_hide
ak_resc_init(void);

void _assetkit_hide
ak_resc_deinit(void);

AkResource *
ak_resc_ins(const char *url);

void
ak_resc_ref(AkResource *resc);

int
ak_resc_unref(AkResource *resc);

int
ak_resc_unref_url(const char *url);

void
ak_resc_print(void);

#endif /* ak_resource_h */
