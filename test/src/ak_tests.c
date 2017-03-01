/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak-test-common.h"
#include "collada/ak_test_collada.h"

int
main(int argc, const char * argv[]) {
  int ret;

  ret = 0;
  ret += ak_test_collada();

  return ret;
}
