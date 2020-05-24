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

#include "common.h"
#include "mem/rb.h"
#include "mem/lt.h"
#include "trash.h"
#include "sid.h"
#include "profile.h"
#include "type.h"
#include "resc/resource.h"

void
AK_CONSTRUCTOR
ak__init(void);

void
AK_DESTRUCTOR
ak__cleanup(void);

void
AK_CONSTRUCTOR
ak__init() {
  ak_mem_init();
  ak_trash_init();
  ak_resc_init();
  ak_type_init();
  ak_sid_init();
  ak_profile_init();
}

void
AK_DESTRUCTOR
ak__cleanup() {
  ak_profile_deinit();
  ak_sid_deinit();
  ak_type_deinit();
  ak_resc_deinit();
  ak_trash_deinit();
  ak_mem_deinit();
}
