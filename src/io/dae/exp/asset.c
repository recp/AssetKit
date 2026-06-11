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

#include "asset.h"
#include "../strpool.h"
#include "../../../../include/ak/options.h"
#include "../../../../include/ak/version.h"

static
DAEExpName
dae_up_axis(AkCoordSys * __restrict coordSys) {
  AkAxis up;

  if (!coordSys)
    return DAE_EXP_NAME_LIT("Y_UP");

  up = coordSys->axis.up;
  if (up == AK_AXIS_POSITIVE_X || up == AK_AXIS_NEGATIVE_X)
    return DAE_EXP_NAME_LIT("X_UP");
  if (up == AK_AXIS_POSITIVE_Z || up == AK_AXIS_NEGATIVE_Z)
    return DAE_EXP_NAME_LIT("Z_UP");

  return DAE_EXP_NAME_LIT("Y_UP");
}

AK_HIDE
void
dae_write_asset(DAEExpState * __restrict st) {
  DAEExpWriter *w;
  AkUnit       *unit;
  AkTree       *extra;
  const char   *unitName;
  const char   *authoringTool;
  double        meter;

  w        = &st->w;
  unit     = st->doc ? st->doc->unit : NULL;
  extra    = st->doc && st->doc->inf ? st->doc->inf->base.extra : NULL;
  unitName = unit && unit->name ? unit->name : _s_dae_meter;
  authoringTool = (const char *)ak_opt_get(AK_OPT_EXPORT_AUTHORING_TOOL);
  if (!authoringTool || !authoringTool[0])
    authoringTool = AK_AUTHORING_TOOL;
  meter    = unit && unit->dist > 0.0 ? unit->dist : 1.0;

  dae_w_lit(w, "<asset><contributor><authoring_tool>");
  dae_w_xml(w, authoringTool, false);
  dae_w_lit(w, "</authoring_tool></contributor><unit name=\"");
  dae_w_xml(w, unitName, true);
  dae_w_lit(w, "\" meter=\"");
  dae_w_double(w, meter);
  dae_w_lit(w, "\"/><up_axis>");
  dae_w_name(w, dae_up_axis(st->doc ? st->doc->coordSys : NULL));
  dae_w_lit(w, "</up_axis>");
  dae_write_extra(w, extra);
  dae_w_lit(w, "</asset>\n");
}
