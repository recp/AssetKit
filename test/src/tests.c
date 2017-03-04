/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "test-common.h"
#include "test_memory.h"
#include "collada/test_collada_load.h"

int
main(int argc, const char * argv[]) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_heap),
    cmocka_unit_test(test_heap_multiple),
    cmocka_unit_test(test_collada_load_folder),
  };

  return cmocka_run_group_tests(tests,
                                NULL,
                                NULL);
}
