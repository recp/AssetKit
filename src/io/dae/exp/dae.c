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

#include "common.h"
#include "dae.h"
#include "document.h"
#include "image.h"
#include "scene.h"
#include "state.h"

#include <stdio.h>

#define DAE_EXP_FILE_BUFFER_SIZE (1024u * 1024u)
AkResult
dae_export(AkDoc * __restrict doc, const char * __restrict filepath) {
  DAEExpState          st;
  FILE                *file;
  AkResult             result;
  AkDaeExportIndexMode indexMode;
  AkDaeExportVersion   versionMode;

  if (!doc || !filepath)
    return AK_ERR;

  indexMode = (AkDaeExportIndexMode)ak_opt_get(AK_OPT_DAE_EXPORT_INDEX_MODE);
  if (indexMode != AK_DAE_EXPORT_INDEX_MULTI
      && indexMode != AK_DAE_EXPORT_INDEX_SINGLE
      && indexMode != AK_DAE_EXPORT_INDEX_AUTO)
    return AK_EINVAL;

  versionMode = (AkDaeExportVersion)ak_opt_get(AK_OPT_DAE_EXPORT_VERSION);
  if (versionMode != AK_DAE_EXPORT_VERSION_AUTO
      && versionMode != AK_DAE_EXPORT_VERSION_1_4
      && versionMode != AK_DAE_EXPORT_VERSION_1_5)
    return AK_EINVAL;

  if (dae_doc_has_unsupported_features(doc))
    return AK_EINVAL;

  file = fopen(filepath, "wb");
  if (!file)
    return AK_EBADF;
  (void)setvbuf(file, NULL, _IOFBF, DAE_EXP_FILE_BUFFER_SIZE);

  if (!dae_state_init(&st, doc, file, filepath, indexMode, versionMode)) {
    dae_state_destroy(&st);
    fclose(file);
    return AK_ERR;
  }

  if (!dae_prepare_maps(&st)) {
    dae_state_destroy(&st);
    fclose(file);
    remove(filepath);
    return AK_ERR;
  }

  if (!dae_select_collada_version(&st)) {
    dae_state_destroy(&st);
    fclose(file);
    remove(filepath);
    return AK_EINVAL;
  }

  if (!dae_prepare_image_export_uris(&st)) {
    dae_state_destroy(&st);
    fclose(file);
    remove(filepath);
    return AK_ERR;
  }

  dae_write_doc(&st);
  dae_w_flush(&st.w);
  if (fclose(file) != 0 && st.w.result == AK_OK)
    st.w.result = AK_ERR;

  result = st.w.result;
  dae_state_destroy(&st);
  if (result != AK_OK)
    remove(filepath);

  return result;
}
