/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_common.h"
#include "ak_memory_common.h"
#include "ak_memory_rb.h"
#include "ak_memory_lt.h"
#include "ak_trash.h"
#include "resc/ak_resource.h"

void
AK_CONSTRUCTOR
ak__init();

void
AK_DESTRUCTOR
ak__cleanup();

void
AK_CONSTRUCTOR
ak__init() {
  ak_mem_init();
  ak_trash_init();
  ak_resc_init();
}

void
AK_DESTRUCTOR
ak__cleanup() {
  ak_resc_deinit();
  ak_trash_deinit();
  ak_mem_deinit();
}
