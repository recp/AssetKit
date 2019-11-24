/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../test_common.h"

const char *dae_dir = "./test/sample-models/collada/files";

TEST_IMPL(dae_load_folder) {
  DIR           *dir;
  struct dirent *ent;
  AkDoc         *doc;

  chdir(dae_dir);

  if ((dir = opendir ("./")) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
	  size_t namelen;

	  namelen = strlen(ent->d_name);

      if (*ent->d_name == '.') {
        if (namelen == 1
            || (namelen == 2 && ent->d_name[1] == '.')
            || strcmp(ent->d_name, ".DS_Store") == 0)
          continue;
      }

      ASSERT(ak_load(&doc, ent->d_name, NULL) == AK_OK);
      ak_free(doc);
    }
    
    closedir(dir);
  }

  TEST_SUCCESS
}
