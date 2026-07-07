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

#include "mesh.h"
#include "source.h"

#include <stdlib.h>

AK_HIDE
AkGeometry*
dae_morph_target_geometry(AkMorphTarget * __restrict target) {
  AkGeometry *geom;

  if (!target || !target->target || target->target->size != sizeof(void *))
    return NULL;

  geom = ak_objGetTarget(target->target);
  return geom && geom->gdata ? geom : NULL;
}

static
AkMorphable*
dae_morphable_at(AkMorphTarget * __restrict target, uint32_t primIdx) {
  AkMorphable *morphable;
  uint32_t     i;

  if (!target
      || !target->target
      || target->target->type != AK_MORPHABLE_MORPHABLE)
    return NULL;

  morphable = ak_objGet(target->target);
  for (i = 0; morphable && i < primIdx; i++)
    morphable = morphable->next;

  return morphable;
}

static
bool
dae_morph_input_supported(AkInput * __restrict input) {
  if (!input || !input->accessor)
    return false;

  switch (input->semantic) {
    case AK_INPUT_POSITION:
    case AK_INPUT_NORMAL:
    case AK_INPUT_TANGENT:
      return input->accessor->componentCount == 3;
    default:
      return false;
  }
}

static
bool
dae_morph_input_seen_before(AkInput * __restrict head,
                            AkInput * __restrict input) {
  AkInput *it;

  for (it = head; it && it != input; it = it->next) {
    if (it->semantic == input->semantic)
      return true;
  }

  return false;
}

static
bool
dae_morphable_primitive_supported(AkMorphable      * __restrict morphable,
                                  AkMeshPrimitive * __restrict prim) {
  AkInput *input;
  uint32_t baseVertexCount;
  bool     hasPosition;

  if (!morphable
      || !prim
      || !prim->pos
      || !prim->pos->accessor
      || prim->pos->accessor->count == 0)
    return false;

  baseVertexCount = prim->pos->accessor->count;
  hasPosition     = false;

  for (input = morphable->input; input; input = input->next) {
    if (!dae_morph_input_supported(input)
        || dae_morph_input_seen_before(morphable->input, input)
        || input->accessor->count < baseVertexCount)
      continue;

    if (input->semantic == AK_INPUT_POSITION)
      hasPosition = true;
  }

  return hasPosition;
}

static
bool
dae_morph_target_supported(AkMorphTarget * __restrict target,
                           AkMesh        * __restrict baseMesh) {
  AkMeshPrimitive *prim;
  uint32_t         primIdx;

  if (!target || !target->target || !baseMesh)
    return false;

  if (target->target->type == AK_MORPHABLE_GEOMETRY) {
    AkGeometry *targetGeom;

    targetGeom = dae_morph_target_geometry(target);
    return targetGeom
           && targetGeom->gdata
           && targetGeom->gdata->type == AK_GEOMETRY_MESH;
  }

  if (target->target->type != AK_MORPHABLE_MORPHABLE)
    return false;

  primIdx = 0;
  for (prim = baseMesh->primitive; prim; prim = prim->next, primIdx++) {
    if (!dae_morphable_primitive_supported(dae_morphable_at(target, primIdx),
                                           prim))
      return false;
  }

  return primIdx > 0
         && (target->primitiveCount == 0 || target->primitiveCount == primIdx);
}

AK_HIDE
void
dae_mark_morph_vertex_geometry(DAEExpState * __restrict st,
                               AkGeometry  * __restrict geom) {
  if (st && geom && st->morphVertexGeometries
      && !rb_find(st->morphVertexGeometries, geom))
    rb_insert(st->morphVertexGeometries, geom, (void *)(uintptr_t)1);
}

AK_HIDE
bool
dae_prepare_morph_target_geometries(DAEExpState * __restrict st,
                                    AkMorph     * __restrict morph) {
  AkMorphTarget *target;

  if (!st || !morph)
    return false;

  for (target = morph->target; target; target = target->next) {
    AkGeometry *geom;

    geom = dae_morph_target_geometry(target);
    if (geom) {
      if (!dae_prepare_extra_geometry(st, geom))
        return false;
      dae_mark_morph_vertex_geometry(st, geom);
    }
  }

  return true;
}

AK_HIDE
bool
dae_prepare_morph(DAEExpState * __restrict st,
                  AkMorph     * __restrict morph) {
  DAEExpMorphRef *ref;

  if (!st || !morph)
    return false;

  if (dae_map_index(st->morphs, morph) != UINT32_MAX)
    return true;

  if (st->morphCount == UINT32_MAX)
    return false;

  ref = malloc(sizeof(*ref));
  if (!ref)
    return false;

  rb_insert(st->morphs, morph, (void *)(uintptr_t)(++st->morphCount));

  ref->next  = NULL;
  ref->morph = morph;

  if (st->lastExtraMorph)
    st->lastExtraMorph->next = ref;
  else
    st->extraMorphs = ref;
  st->lastExtraMorph = ref;

  return true;
}

AK_HIDE
bool
dae_instance_morph_supported(AkInstanceGeometry * __restrict inst) {
  AkInstanceMorph *morpher;
  AkMorph         *morph;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMorphTarget   *target;
  uint32_t         count;

  morpher = inst ? inst->morpher : NULL;
  morph   = morpher ? morpher->morph : NULL;
  geom    = dae_instance_geometry_object(inst);
  if (!morph
      || !geom
      || !geom->gdata
      || geom->gdata->type != AK_GEOMETRY_MESH
      || morph->targetCount == 0)
    return false;

  mesh = ak_objGet(geom->gdata);
  if (!mesh || !mesh->primitive)
    return false;

  if (morph->method != AK_MORPH_METHOD_NORMALIZED
      && morph->method != AK_MORPH_METHOD_RELATIVE)
    return false;

  count = 0;
  for (target = morph->target; target; target = target->next) {
    if (!dae_morph_target_supported(target, mesh))
      return false;
    count++;
  }

  return count == morph->targetCount;
}

AK_HIDE
void
dae_w_morph_target_geom_id(DAEExpWriter * __restrict w,
                           uint32_t                  morphIdx,
                           uint32_t                  targetIdx) {
  dae_w_lit(w, "morph_");
  dae_w_uint_fast(w, morphIdx);
  dae_w_lit(w, "_target_");
  dae_w_uint_fast(w, targetIdx);
}

static
void
dae_w_morph_target_prim_id(DAEExpWriter * __restrict w,
                           uint32_t                  morphIdx,
                           uint32_t                  targetIdx,
                           uint32_t                  primIdx,
                           DAEExpName                suffix) {
  dae_w_morph_target_geom_id(w, morphIdx, targetIdx);
  dae_w_lit(w, "_prim_");
  dae_w_uint_fast(w, primIdx);
  if (suffix.ptr && suffix.len > 0) {
    dae_w_ch(w, '_');
    dae_w_name(w, suffix);
  }
}

static
void
dae_w_morph_target_source_id(DAEExpWriter * __restrict w,
                             uint32_t                  morphIdx,
                             uint32_t                  targetIdx,
                             uint32_t                  primIdx,
                             DAEExpName                semantic,
                             uint32_t                  inputIdx) {
  dae_w_morph_target_prim_id(w, morphIdx, targetIdx, primIdx, semantic);
  dae_w_ch(w, '_');
  dae_w_uint_fast(w, inputIdx);
}

static
bool
dae_write_morphable_source(DAEExpState * __restrict st,
                           AkInput     * __restrict input,
                           uint32_t                 morphIdx,
                           uint32_t                 targetIdx,
                           uint32_t                 primIdx,
                           uint32_t                 inputIdx) {
  DAEExpWriter *w;
  AkAccessor   *acc;
  DAEExpName    semanticName;
  const char   *semantic;
  float        *scratch;
  uint32_t      i;
  uint32_t      c;
  uint32_t      componentCount;
  bool          direct;

  w        = &st->w;
  acc      = input ? input->accessor : NULL;
  semantic = dae_semantic_name(input);
  if (!semantic || !*semantic || !acc || acc->count == 0 || acc->componentCount == 0)
    return false;
  semanticName = DAE_EXP_NAME_CSTR(semantic);

  componentCount = acc->componentCount;
  direct         = io_accessor_float_direct(acc);
  scratch        = NULL;

  if (!direct) {
    size_t floatCount;

    if ((size_t)acc->count > (size_t)-1 / componentCount)
      return false;
    floatCount = (size_t)acc->count * componentCount;
    scratch = dae_scratch(st, sizeof(float) * floatCount);
    if (!scratch)
      return false;
    if (ak_accessorAsFloat(acc, scratch, floatCount) != floatCount)
      return false;
  }

  dae_w_lit(w, "<source id=\"");
  dae_w_morph_target_source_id(w,
                               morphIdx,
                               targetIdx,
                               primIdx,
                               semanticName,
                               inputIdx);
  dae_w_lit(w, "\"><float_array id=\"");
  dae_w_morph_target_source_id(w,
                               morphIdx,
                               targetIdx,
                               primIdx,
                               semanticName,
                               inputIdx);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, (size_t)acc->count * componentCount);
  dae_w_lit(w, "\">");

  for (i = 0; i < acc->count; i++) {
    const float *row;

    row = direct
          ? io_accessor_float_row(acc, i)
          : scratch + (size_t)i * componentCount;

    for (c = 0; c < componentCount; c++) {
      if (i > 0 || c > 0)
        dae_w_ch(w, ' ');
      dae_w_float_fast(w, row[c]);
    }
  }

  dae_w_lit(w, "</float_array><technique_common><accessor source=\"#");
  dae_w_morph_target_source_id(w,
                               morphIdx,
                               targetIdx,
                               primIdx,
                               semanticName,
                               inputIdx);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint_fast(w, acc->count);
  dae_w_lit(w, "\" stride=\"");
  dae_w_uint_fast(w, componentCount);
  dae_w_lit(w, "\">");

  for (c = 0; c < componentCount; c++) {
    dae_w_lit(w, "<param name=\"");
    dae_w_name(w, dae_input_param_exp_name(input, c));
    dae_w_lit(w, "\" type=\"float\"/>");
  }

  dae_w_lit(w, "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

static
void
dae_write_morphable_vertices(DAEExpState * __restrict st,
                             uint32_t                 morphIdx,
                             uint32_t                 targetIdx,
                             uint32_t                 primIdx,
                             AkInput    ** __restrict inputs,
                             uint32_t                 inputCount) {
  DAEExpWriter *w;
  uint32_t      i;

  w = &st->w;
  dae_w_lit(w, "<vertices id=\"");
  dae_w_morph_target_prim_id(w,
                             morphIdx,
                             targetIdx,
                             primIdx,
                             DAE_EXP_NAME(vertices));
  dae_w_lit(w, "\">");

  for (i = 0; i < inputCount; i++) {
    AkInput    *input;
    DAEExpName  semanticName;
    const char *semantic;

    input    = inputs[i];
    semantic = dae_semantic_name(input);
    if (!semantic || !*semantic)
      continue;
    semanticName = DAE_EXP_NAME_CSTR(semantic);

    dae_w_lit(w, "<input semantic=\"");
    if (input->semantic == AK_INPUT_POSITION)
      dae_w_name(w, DAE_EXP_NAME(POSITION));
    else
      dae_w_name(w, semanticName);
    dae_w_lit(w, "\" source=\"#");
    dae_w_morph_target_source_id(w,
                                 morphIdx,
                                 targetIdx,
                                 primIdx,
                                 semanticName,
                                 i);
    dae_w_lit(w, "\"/>");
  }

  dae_w_lit(w, "</vertices>");
}

static
bool
dae_write_morphable_primitive(DAEExpState     * __restrict st,
                              AkMeshPrimitive * __restrict prim,
                              AkMorphable     * __restrict morphable,
                              uint32_t                     morphIdx,
                              uint32_t                     targetIdx,
                              uint32_t                     primIdx) {
  DAEExpWriter *w;
  AkInput      *input;
  AkInput      *inputs[32];
  uint32_t      inputCount;
  uint32_t      posInputIdx;
  uint32_t      i;
  uint32_t      vertexCount;
  DAEExpName    tag;

  if (!dae_primitive_supported(prim)
      || !dae_morphable_primitive_supported(morphable, prim))
    return false;

  w           = &st->w;
  inputCount  = 0;
  posInputIdx = UINT32_MAX;

  for (input = morphable->input; input; input = input->next) {
    bool isPosition;

    if (!dae_morph_input_supported(input)
        || dae_morph_input_seen_before(morphable->input, input))
      continue;
    if (inputCount >= AK_ARRAY_LEN(inputs))
      return false;

    isPosition = input->semantic == AK_INPUT_POSITION;
    if (!dae_write_morphable_source(st,
                                    input,
                                    morphIdx,
                                    targetIdx,
                                    primIdx,
                                    inputCount)) {
      if (isPosition)
        return false;
      continue;
    }
    if (isPosition)
      posInputIdx = inputCount;

    inputs[inputCount++] = input;
  }

  if (inputCount == 0 || posInputIdx == UINT32_MAX)
    return false;

  dae_write_morphable_vertices(st,
                               morphIdx,
                               targetIdx,
                               primIdx,
                               inputs,
                               inputCount);

  if (!dae_primitive_tag(prim, &tag))
    return false;

  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_attr_uint(w, DAE_EXP_NAME(count), dae_primitive_count(prim));
  dae_w_ch(w, '>');

  dae_w_lit(w, "<input semantic=\"");
  dae_w_name(w, DAE_EXP_NAME(VERTEX));
  dae_w_lit(w, "\" source=\"#");
  dae_w_morph_target_prim_id(w,
                             morphIdx,
                             targetIdx,
                             primIdx,
                             DAE_EXP_NAME(vertices));
  dae_w_lit(w, "\" offset=\"0\"/>");

  if (prim->type == AK_PRIMITIVE_POLYGONS) {
    AkPolygon *poly;
    size_t     vc;

    poly = (AkPolygon *)prim;
    dae_w_lit(w, "<vcount>");
    for (vc = 0; poly->vcount && vc < poly->vcount->count; vc++) {
      if (vc > 0)
        dae_w_ch(w, ' ');
      dae_w_uint_fast(w, poly->vcount->items[vc]);
    }
    dae_w_lit(w, "</vcount>");
  }

  vertexCount = io_primitive_vertex_count(prim);
  dae_w_lit(w, "<p>");
  if (prim->type == AK_PRIMITIVE_LINES
      && ((AkLines *)prim)->mode == AK_LINE_LOOP) {
    bool firstIndex;

    firstIndex = true;
    for (i = 0; i < vertexCount; i++) {
      uint32_t edgeVerts[2];
      uint32_t edgeVertIdx;

      edgeVerts[0] = i;
      edgeVerts[1] = (i + 1u) == vertexCount ? 0u : i + 1u;

      for (edgeVertIdx = 0; edgeVertIdx < 2u; edgeVertIdx++) {
        if (!firstIndex)
          dae_w_ch(w, ' ');
        firstIndex = false;
        dae_w_uint_fast(w,
                   io_primitive_input_index(prim,
                                             prim->pos,
                                             edgeVerts[edgeVertIdx]));
      }
    }
  } else {
    for (i = 0; i < vertexCount; i++) {
      if (i > 0)
        dae_w_ch(w, ' ');
      dae_w_uint_fast(w, io_primitive_input_index(prim, prim->pos, i));
    }
  }
  dae_w_lit(w, "</p>");
  dae_write_extra(w, prim->extra);
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');

  return w->result == AK_OK;
}

static
bool
dae_write_morphable_target_geometry(DAEExpState  * __restrict st,
                                    AkMorphTarget * __restrict target,
                                    AkGeometry    * __restrict baseGeom,
                                    uint32_t                   morphIdx,
                                    uint32_t                   targetIdx) {
  DAEExpWriter   *w;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  uint32_t        primIdx;

  if (!target
      || !baseGeom
      || !baseGeom->gdata
      || baseGeom->gdata->type != AK_GEOMETRY_MESH)
    return false;

  mesh = ak_objGet(baseGeom->gdata);
  if (!mesh)
    return false;

  w = &st->w;
  dae_w_lit(w, "<geometry id=\"");
  dae_w_morph_target_geom_id(w, morphIdx, targetIdx);
  dae_w_lit(w, "\"><mesh>");

  primIdx = 0;
  for (prim = mesh->primitive; prim; prim = prim->next, primIdx++) {
    if (!dae_write_morphable_primitive(st,
                                       prim,
                                       dae_morphable_at(target, primIdx),
                                       morphIdx,
                                       targetIdx,
                                       primIdx))
      return false;
  }

  dae_write_extra(w, mesh->extra);
  dae_w_lit(w, "</mesh></geometry>");
  return w->result == AK_OK;
}

AK_HIDE
bool
dae_write_morphable_target_geometries(DAEExpState * __restrict st,
                                      AkMorph     * __restrict morph,
                                      AkGeometry  * __restrict baseGeom,
                                      uint32_t                 morphIdx) {
  AkMorphTarget *target;
  uint32_t       targetIdx;

  if (!morph)
    return false;

  targetIdx = 0;
  for (target = morph->target; target; target = target->next, targetIdx++) {
    if (!target->target || target->target->type != AK_MORPHABLE_MORPHABLE)
      continue;
    if (!dae_write_morphable_target_geometry(st,
                                             target,
                                             baseGeom,
                                             morphIdx,
                                             targetIdx))
      return false;
  }

  return targetIdx == morph->targetCount;
}
