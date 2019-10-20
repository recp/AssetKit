/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef tests_h
#define tests_h

#include "include/common.h"

/*
 * To register a test:
 *   1. use TEST_DECLARE() to forward declare test
 *   2. use TEST_ENTRY() to add test to list
 */

TEST_DECLARE(heap)
TEST_DECLARE(heap_multiple)
TEST_DECLARE(dae_load_folder)

/*****************************************************************************/

TEST_LIST {
  TEST_ENTRY(heap)
  TEST_ENTRY(heap_multiple)
  TEST_ENTRY(dae_load_folder)
};

#endif /* tests_h */
