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

#include "include/common.h"
#include "tests.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>

#define TARGET_NAME "AssetKit"

#ifndef AK_BUILD_EXPORTERS
#  define AK_BUILD_EXPORTERS 1
#endif
#ifndef AK_BUILD_DAE_EXPORTER
#  define AK_BUILD_DAE_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_GLTF_EXPORTER
#  define AK_BUILD_GLTF_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_OBJ_EXPORTER
#  define AK_BUILD_OBJ_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_STL_EXPORTER
#  define AK_BUILD_STL_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_PLY_EXPORTER
#  define AK_BUILD_PLY_EXPORTER AK_BUILD_EXPORTERS
#endif
#ifndef AK_BUILD_3MF_EXPORTER
#  define AK_BUILD_3MF_EXPORTER AK_BUILD_EXPORTERS
#endif

#if AK_BUILD_DAE_EXPORTER && AK_BUILD_GLTF_EXPORTER && AK_BUILD_OBJ_EXPORTER \
    && AK_BUILD_STL_EXPORTER && AK_BUILD_PLY_EXPORTER && AK_BUILD_3MF_EXPORTER
#  define AK_TEST_ALL_EXPORTERS 1
#else
#  define AK_TEST_ALL_EXPORTERS 0
#endif

#if !AK_TEST_ALL_EXPORTERS
static int
test_requires_export(const char *name) {
  return strstr(name, "export") != NULL
         || strncmp(name, "three_mf_import_", 16) == 0
         || strncmp(name, "dae14_", 6) == 0
         || (strncmp(name, "dae_", 4) == 0
             && strcmp(name, "dae_instance_node_is_instance_node") != 0
             && strcmp(name, "dae_camera_light_extra_preserve_opt") != 0
             && strcmp(name, "dae_load_folder") != 0);
}
#endif

int
main(int argc, const char * argv[]) {
  test_entry_t *entry;
  test_status_t st;
  int32_t       i, count, passed, failed, skipped, maxlen;
  double        start, end, elapsed, total;

  passed = failed = skipped = maxlen = 0;
  total  = 0.0;
  count  = sizeof(tests) / sizeof(tests[0]);

  fprintf(stderr, CYAN "\nWelcome to %s tests\n\n" RESET, TARGET_NAME);

  for (i = 0; i < count; i++) {
    int32_t len;

    entry = tests + i;
    len   = (int32_t)strlen(entry->name);

    if (len > maxlen)
      maxlen = len;
  }

  maxlen += 5;

  fprintf(stderr,
          BOLDWHITE  "  %-*s    %-*s\n",
          maxlen, "Test Name", maxlen, "Elapsed Time");

  for (i = 0; i < count; i++) {
    entry   = tests + i;

#if !AK_TEST_ALL_EXPORTERS
    if (test_requires_export(entry->name)) {
      fprintf(stderr, YELLOW  "  skip: %-*s  export support disabled\n" RESET,
              maxlen,
              entry->name);
      skipped++;
      continue;
    }
#endif

    start   = clock();
    st      = entry->entry();
    end     = clock();
    elapsed = (end - start) / CLOCKS_PER_SEC;
    total  += elapsed;

    if (!st.status) {
      fprintf(stderr,
              BOLDRED  "  " FAIL_TEXT BOLDWHITE " %s " RESET, entry->name);
      if (st.msg) {
        fprintf(stderr,
                YELLOW "- %s" RESET,
                st.msg);
      }

      fprintf(stderr, "\n");

      failed++;
    } else {
      fprintf(stderr, GREEN  "  " OK_TEXT RESET " %-*s  ", maxlen, entry->name);

      if (elapsed > 0.01)
        fprintf(stderr, YELLOW "%.2fs", elapsed);
      else
        fprintf(stderr, "0");

      fprintf(stderr, "\n" RESET);
      passed++;
    }
  }

  if (failed == 0) {
    fprintf(stderr,
            BOLDGREEN "\n  All tests are passed " FINAL_TEXT "\n" RESET);
  }

  fprintf(stderr,
          CYAN "\n%s test results (%0.2fs):\n" RESET
          "--------------------------\n"

          MAGENTA "%d" RESET " tests are runned, "
          GREEN   "%d" RESET " %s passed, "
          RED     "%d" RESET " %s failed, "
          YELLOW  "%d" RESET " skipped\n\n" RESET,
          TARGET_NAME,
          total,
          count,
          passed,
          passed > 1 ? "are" : "is",
          failed,
          failed > 1 ? "are" : "is",
          skipped);

  return failed;
}
