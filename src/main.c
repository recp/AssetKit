/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "common.h"
#include "mem_common.h"
#include "mem_rb.h"
#include "mem_lt.h"
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
