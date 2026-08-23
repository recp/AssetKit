/*
 * Copyright (C) 2026 Recep Aslantas
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

#include "sketchup.h"

#include <ctype.h>
#include <stddef.h>

static
bool
dae_sketchup_tool(const char * __restrict tool) {
  const char *needle;
  const char *s;

  if (!tool)
    return false;

  needle = "sketchup";
  for (; *tool; tool++) {
    for (s = tool;
         *s && *needle
         && tolower((unsigned char)*s) == tolower((unsigned char)*needle);
         s++, needle++) {
    }

    if (!*needle)
      return true;

    needle = "sketchup";
  }

  return false;
}

AK_HIDE
void
dae_bugfix_sketchup_material_profile(
  AkTechniqueFxCommon * __restrict common) {
  AkContributor *contributor;

  if (!common
      || (common->type != AK_MATERIAL_TYPE_PHONG
          && common->type != AK_MATERIAL_TYPE_BLINN))
    return;

  contributor = ak_getAssetInfo(common, offsetof(AkAssetInf, contributor));
  for (; contributor; contributor = contributor->next) {
    if (!dae_sketchup_tool(contributor->authoringTool))
      continue;

    /* SketchUp has no user-facing specular or shininess controls. Its
       COLLADA exporter nevertheless labels every lit material Phong and
       writes a fixed specular lobe. That lobe is exporter boilerplate, not
       authored material intent, so expose the profile as diffuse Lambert.
       Keep the parsed classic inputs intact for source inspection while the
       surface type tells renderers which inputs are semantically active. */
    common->type = AK_MATERIAL_TYPE_LAMBERT;
    return;
  }
}
