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
