/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "test_dae_load.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

const char *ak_dae_dir = "./test/sample-models/collada/files";

void
test_dae_load_folder(void **state) {
  DIR           *dir;
  struct dirent *ent;
  AkDoc         *doc;

  (void)state;
  chdir(ak_dae_dir);

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

      assert(ak_load(&doc, ent->d_name, NULL) == AK_OK);
      ak_free(doc);
    }
    
    closedir(dir);
  }
}
