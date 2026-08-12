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

#include "controller.h"
#include "mesh.h"

AK_HIDE
void
dae_w_morph_id(DAEExpWriter * __restrict w, uint32_t morphIdx) {
  dae_w_id(w, DAE_EXP_NAME(morph), morphIdx);
}

static
void
dae_w_morph_source_id(DAEExpWriter     * __restrict w,
                      uint32_t                      morphIdx,
                      DAEExpName                    suffix) {
  dae_w_morph_id(w, morphIdx);
  dae_w_ch(w, '_');
  dae_w_name(w, suffix);
}

static
bool
dae_write_morph_target_source(DAEExpState * __restrict st,
                              AkMorph     * __restrict morph,
                              uint32_t                 morphIdx) {
  DAEExpWriter *w;
  AkMorphTarget *target;
  uint32_t       targetIdx;

  if (!morph || morph->targetCount == 0)
    return false;

  w = &st->w;
  dae_w_lit(w, "<source id=\"");
  dae_w_morph_source_id(w, morphIdx, DAE_EXP_NAME(targets));
  dae_w_lit(w, "\"><IDREF_array id=\"");
  dae_w_morph_source_id(w, morphIdx, DAE_EXP_NAME_LIT("targets_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint_fast(w, morph->targetCount);
  dae_w_lit(w, "\">");

  targetIdx = 0;
  for (target = morph->target; target; target = target->next, targetIdx++) {
    AkGeometry *targetGeom;
    uint32_t    geomIdx;

    if (targetIdx > 0)
      dae_w_ch(w, ' ');

    if (target->target
        && (target->target->type == AK_MORPHABLE_MORPHABLE
            || dae_morph_target_vertices_only(target))) {
      dae_w_morph_target_geom_id(w, morphIdx, targetIdx);
    } else {
      targetGeom = dae_morph_target_geometry(target);
      geomIdx    = targetGeom ? dae_map_index(st->geometries, targetGeom)
                              : UINT32_MAX;
      if (geomIdx == UINT32_MAX)
        return false;
      dae_w_geom_id(w, geomIdx);
    }
  }

  if (targetIdx != morph->targetCount)
    return false;

  dae_w_lit(w, "</IDREF_array><technique_common><accessor source=\"#");
  dae_w_morph_source_id(w, morphIdx, DAE_EXP_NAME_LIT("targets_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint_fast(w, morph->targetCount);
  dae_w_lit(w, "\" stride=\"1\"><param name=\"MORPH_TARGET\" type=\"IDREF\"/>"
               "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

static
bool
dae_write_morph_weight_source(DAEExpState * __restrict st,
                              AkMorph     * __restrict morph,
                              uint32_t                 morphIdx) {
  DAEExpWriter    *w;
  AkInstanceMorph *morpher;
  AkFloatArray    *weights;
  uint32_t         i;

  if (!morph || morph->targetCount == 0)
    return false;

  morpher = rb_find(st->morphInstances, morph);
  weights = morpher && morpher->overrideWeights
            ? morpher->overrideWeights
            : morph->defaultWeights;

  w = &st->w;
  dae_w_lit(w, "<source id=\"");
  dae_w_morph_source_id(w, morphIdx, DAE_EXP_NAME_LIT("weights"));
  dae_w_lit(w, "\"><float_array id=\"");
  dae_w_morph_source_id(w, morphIdx, DAE_EXP_NAME_LIT("weights_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint_fast(w, morph->targetCount);
  dae_w_lit(w, "\">");

  for (i = 0; i < morph->targetCount; i++) {
    float weight;

    weight = 0.0f;
    if (weights && i < weights->count)
      weight = weights->items[i];

    if (i > 0)
      dae_w_ch(w, ' ');
    dae_w_float_fast(w, weight);
  }

  dae_w_lit(w, "</float_array><technique_common><accessor source=\"#");
  dae_w_morph_source_id(w, morphIdx, DAE_EXP_NAME_LIT("weights_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint_fast(w, morph->targetCount);
  dae_w_lit(w, "\" stride=\"1\"><param name=\"MORPH_WEIGHT\" type=\"float\"/>"
               "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

AK_HIDE
bool
dae_write_morph_controller(DAEExpState * __restrict st,
                           AkMorph     * __restrict morph,
                           uint32_t                 morphIdx) {
  DAEExpWriter *w;
  AkGeometry   *geom;
  uint32_t      geomIdx;

  geom    = rb_find(st->morphGeometries, morph);
  geomIdx = geom ? dae_map_index(st->geometries, geom) : UINT32_MAX;
  if (!morph || geomIdx == UINT32_MAX)
    return false;

  w = &st->w;
  dae_w_lit(w, "<controller id=\"");
  dae_w_morph_id(w, morphIdx);
  dae_w_lit(w, "\"><morph");
  if (morph->method == AK_MORPH_METHOD_RELATIVE)
    dae_w_lit(w, " method=\"RELATIVE\"");
  dae_w_lit(w, " source=\"#");
  dae_w_geom_id(w, geomIdx);
  dae_w_lit(w, "\">");

  if (!dae_write_morph_target_source(st, morph, morphIdx)
      || !dae_write_morph_weight_source(st, morph, morphIdx)) {
    if (w->result == AK_OK)
      w->result = AK_EINVAL;
    return false;
  }

  dae_w_lit(w, "<targets><input semantic=\"MORPH_TARGET\" source=\"#");
  dae_w_morph_source_id(w, morphIdx, DAE_EXP_NAME(targets));
  dae_w_lit(w, "\"/><input semantic=\"MORPH_WEIGHT\" source=\"#");
  dae_w_morph_source_id(w, morphIdx, DAE_EXP_NAME_LIT("weights"));
  dae_w_lit(w, "\"/></targets></morph></controller>");

  return w->result == AK_OK;
}
