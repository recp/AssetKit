/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_test_collada.h"
#include "ak_test_collada_load.h"

const struct CMUnitTest ak_collada_tests[] = {
  cmocka_unit_test(ak_collada_load_folder)
};

int
ak_test_collada() {
  return cmocka_run_group_tests(ak_collada_tests, NULL, NULL);
}
