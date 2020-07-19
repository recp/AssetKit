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

#include "transp.h"
#include <string.h>
#include <ctype.h>

void AK_HIDE
dae_bugfix_transp(AkTransparent * __restrict transp) {
  AkContributor *contr;
  char          *tool;

  if (!(contr = ak_getAssetInfo(transp, offsetof(AkAssetInf, contributor)))
      || !(tool = (char *)contr->authoringTool))
    return;

  tool = ak_tolower(strdup(tool));

  /* fix old SketchUp transparency bug */
  if (strstr(tool, _s_dae_sketchup)) {
    int major, minor, patch;
    if(sscanf(tool, "%*[^0123456789]%d%*[. ]%d%*[. ]%d",
              &major, &minor, &patch)) {

      /* don't flip >= 7.1.1 */
      if (major <= 7 && minor < 2 && patch < 1) {
        if (transp->amount->val) {
          *transp->amount->val = 1.0f - *transp->amount->val;
        }
      }
    }
  } /* _s_dae_sketchup */
  
  free(tool);
}
