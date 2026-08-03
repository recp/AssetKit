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

#include <time.h>

static
bool
dae_asset_time(char out[21], time_t value) {
  struct tm tmv;

#if defined(_WIN32)
  if (gmtime_s(&tmv, &value) != 0)
    return false;
#else
  if (!gmtime_r(&value, &tmv))
    return false;
#endif

  return strftime(out, 21, "%Y-%m-%dT%H:%M:%SZ", &tmv) == 20u;
}

static
void
dae_write_asset_time(DAEExpWriter * __restrict w,
                     const char   * __restrict tag,
                     size_t                    tagLen,
                     time_t                    value) {
  char text[21];

  if (!dae_asset_time(text, value))
    memcpy(text, "1970-01-01T00:00:00Z", sizeof(text));

  dae_w_ch(w, '<');
  dae_w_raw(w, tag, tagLen);
  dae_w_ch(w, '>');
  dae_w_raw(w, text, 20u);
  dae_w_lit(w, "</");
  dae_w_raw(w, tag, tagLen);
  dae_w_ch(w, '>');
}

static
void
dae_write_asset_text(DAEExpWriter * __restrict w,
                     const char   * __restrict tag,
                     size_t                    tagLen,
                     const char   * __restrict value) {
  if (!value)
    return;

  dae_w_ch(w, '<');
  dae_w_raw(w, tag, tagLen);
  dae_w_ch(w, '>');
  dae_w_xml(w, value, false);
  dae_w_lit(w, "</");
  dae_w_raw(w, tag, tagLen);
  dae_w_ch(w, '>');
}

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
  AkAssetInf   *inf;
  AkUnit       *unit;
  AkTree       *extra;
  const char   *unitName;
  const char   *authoringTool;
  double        meter;
  time_t        now;
  time_t        created;
  time_t        modified;

  w        = &st->w;
  inf      = st->doc && st->doc->inf ? &st->doc->inf->base : NULL;
  unit     = st->doc ? st->doc->unit : NULL;
  extra    = inf ? inf->extra : NULL;
  unitName = unit && unit->name ? unit->name : _s_dae_meter;
  authoringTool = (const char *)ak_opt_get(AK_OPT_EXPORT_AUTHORING_TOOL);
  if (!authoringTool || !authoringTool[0])
    authoringTool = AK_AUTHORING_TOOL;
  meter    = unit && unit->dist > 0.0 ? unit->dist : 1.0;
  now      = time(NULL);
  created  = inf && inf->created > 0 ? inf->created : now;
  modified = inf && inf->modified > 0 ? inf->modified : now;

  dae_w_lit(w, "<asset><contributor><authoring_tool>");
  dae_w_xml(w, authoringTool, false);
  dae_w_lit(w, "</authoring_tool></contributor>");
  dae_write_asset_time(w, "created", sizeof("created") - 1u, created);
  dae_write_asset_text(w,
                       "keywords",
                       sizeof("keywords") - 1u,
                       inf ? inf->keywords : NULL);
  dae_write_asset_time(w, "modified", sizeof("modified") - 1u, modified);
  dae_write_asset_text(w,
                       "revision",
                       sizeof("revision") - 1u,
                       inf ? inf->revision : NULL);
  dae_write_asset_text(w,
                       "subject",
                       sizeof("subject") - 1u,
                       inf ? inf->subject : NULL);
  dae_write_asset_text(w,
                       "title",
                       sizeof("title") - 1u,
                       inf ? inf->title : NULL);
  dae_w_lit(w, "<unit name=\"");
  dae_w_xml(w, unitName, true);
  dae_w_lit(w, "\" meter=\"");
  dae_w_double(w, meter);
  dae_w_lit(w, "\"/><up_axis>");
  dae_w_name(w, dae_up_axis(st->doc ? st->doc->coordSys : NULL));
  dae_w_lit(w, "</up_axis>");
  dae_write_extra(w, extra);
  dae_w_lit(w, "</asset>\n");
}
