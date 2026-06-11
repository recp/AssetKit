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
#include "material.h"

AK_HIDE
void
dae_write_library_controllers(DAEExpState * __restrict st) {
  DAEExpWriter *w;
  DAEExpMorphRef *morphRef;
  AkSkin       *skin;
  AkMorph      *morph;
  uint32_t      skinIdx;
  uint32_t      morphIdx;
  bool          hasMorphs;

  if (!st)
    return;

  hasMorphs = false;
  for (morph = st->doc->lib.morphs.first; morph; morph = morph->next) {
    if (rb_find(st->morphGeometries, morph)) {
      hasMorphs = true;
      break;
    }
  }
  for (morphRef = st->extraMorphs; !hasMorphs && morphRef; morphRef = morphRef->next) {
    if (rb_find(st->morphGeometries, morphRef->morph))
      hasMorphs = true;
  }

  if (st->skinCount == 0 && !hasMorphs)
    return;

  w = &st->w;
  dae_w_lit(w, "<library_controllers>");
  for (morph = st->doc->lib.morphs.first; morph; morph = morph->next) {
    if (!rb_find(st->morphGeometries, morph))
      continue;
    morphIdx = dae_map_index(st->morphs, morph);
    if (morphIdx == UINT32_MAX)
      return;
    if (!dae_write_morph_controller(st, morph, morphIdx))
      return;
  }
  for (morphRef = st->extraMorphs; morphRef; morphRef = morphRef->next) {
    morph = morphRef->morph;
    if (!rb_find(st->morphGeometries, morph))
      continue;
    morphIdx = dae_map_index(st->morphs, morph);
    if (morphIdx == UINT32_MAX)
      return;
    if (!dae_write_morph_controller(st, morph, morphIdx))
      return;
  }
  skinIdx = 0;
  for (skin = st->doc->lib.skins.first; skin; skin = skin->next, skinIdx++) {
    if (!dae_write_skin_controller(st, skin, skinIdx))
      return;
  }
  dae_w_lit(w, "</library_controllers>\n");
}

AK_HIDE
void
dae_write_instance_controller(DAEExpState        * __restrict st,
                              AkInstanceGeometry * __restrict inst) {
  DAEExpWriter   *w;
  AkInstanceSkin *skinner;
  AkInstanceMorph *morpher;
  AkSkin         *skin;
  AkMorph        *morph;
  AkGeometry     *geom;
  uint32_t        controllerIdx;
  bool            isSkin;

  skinner = inst ? inst->skinner : NULL;
  morpher = inst ? inst->morpher : NULL;
  skin    = skinner ? skinner->skin : NULL;
  morph   = morpher ? morpher->morph : NULL;
  geom    = dae_instance_geometry_object(inst);
  isSkin  = skin != NULL;
  controllerIdx = isSkin
                  ? dae_map_index(st->skins, skin)
                  : dae_map_index(st->morphs, morph);
  if (controllerIdx == UINT32_MAX || !geom)
    return;

  w = &st->w;
  dae_w_lit(w, "<instance_controller url=\"#");
  if (isSkin)
    dae_w_skin_id(w, controllerIdx);
  else
    dae_w_morph_id(w, controllerIdx);
  dae_w_lit(w, "\">");

  if (isSkin && skin->skeleton) {
    dae_w_lit(w, "<skeleton>#");
    if (!dae_w_node_id_ref(st, skin->skeleton)) {
      if (w->result == AK_OK)
        w->result = AK_EINVAL;
      return;
    }
    dae_w_lit(w, "</skeleton>");
  }

  dae_write_instance_materials(st, geom, inst);
  dae_write_extra(w, inst ? inst->base.extra : NULL);
  dae_w_lit(w, "</instance_controller>");
}

AK_HIDE
bool
dae_instance_controller_exportable(DAEExpState        * __restrict st,
                                   AkInstanceGeometry * __restrict inst) {
  AkInstanceSkin  *skinner;
  AkInstanceMorph *morpher;
  AkSkin          *skin;
  AkMorph         *morph;

  skinner = inst ? inst->skinner : NULL;
  morpher = inst ? inst->morpher : NULL;
  skin    = skinner ? skinner->skin : NULL;
  morph   = morpher ? morpher->morph : NULL;

  if (skin && dae_map_index(st->skins, skin) != UINT32_MAX)
    return true;

  return morph
         && dae_map_index(st->morphs, morph) != UINT32_MAX
         && rb_find(st->morphGeometries, morph) != NULL;
}
