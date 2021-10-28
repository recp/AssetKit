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

AK_HIDE
void
ak_resc_init(void);

AK_HIDE
void
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
