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

#include "../../../test_common.h"

const char *dae_dir = "./test/sample-models/collada/files";

TEST_IMPL(dae_load_folder) {
  DIR           *dir;
  struct dirent *ent;
  AkDoc         *doc;
  char           cwd[4096];

  if (!getcwd(cwd, sizeof(cwd)))
    cwd[0] = '\0';

  if (chdir(dae_dir) != 0)
    TEST_SUCCESS

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

  if (cwd[0] != '\0')
    chdir(cwd);

  TEST_SUCCESS
}

TEST_IMPL(load_auto_uppercase_extension) {
  const char *path = "/tmp/assetkit_uppercase_auto.DAE";
  const char *xml =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" "
    "version=\"1.4.1\">"
    "<asset><up_axis>Y_UP</up_axis></asset>"
    "<library_visual_scenes><visual_scene id=\"Scene\"/></library_visual_scenes>"
    "<scene><instance_visual_scene url=\"#Scene\"/></scene>"
    "</COLLADA>";
  AkDoc *doc;
  FILE  *file;

  file = fopen(path, "wb");
  ASSERT(file != NULL);
  ASSERT(fwrite(xml, 1, strlen(xml), file) == strlen(xml));
  fclose(file);

  ASSERT(ak_load(&doc, path, AK_FILE_TYPE_AUTO) == AK_OK);
  ASSERT(doc != NULL);

  ak_free(doc);
  unlink(path);

  TEST_SUCCESS
}
