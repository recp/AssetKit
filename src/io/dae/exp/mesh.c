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
#include "brep.h"
#include "source.h"

static
bool
dae_input_morph_vertex_domain(AkInput * __restrict input) {
  if (!input)
    return false;

  /* keep this list aligned with dae_morph_input_supported(). TEXCOORD/COLOR
     can be vertex attributes in COLLADA, but AssetKit does not currently
     expose them as morphable target data, so they stay as primitive inputs. */
  switch (input->semantic) {
    case AK_INPUT_POSITION:
    case AK_INPUT_NORMAL:
    case AK_INPUT_TANGENT:
      return true;
    default:
      break;
  }

  return false;
}

static
bool
dae_input_in_vertices(AkMeshPrimitive * __restrict prim,
                      AkInput         * __restrict input,
                      bool                         morphVertexGeometry) {
  if (!prim || !input)
    return false;

  if (input == prim->pos || input->semantic == AK_INPUT_POSITION)
    return true;

  if (!morphVertexGeometry
      || !prim->pos
      || !dae_input_morph_vertex_domain(input))
    return false;

  return input->indexOffset == prim->pos->indexOffset;
}

static
bool
dae_write_p_single_input_fast(DAEExpWriter    * __restrict w,
                              AkMeshPrimitive * __restrict prim,
                              AkInput         * __restrict input,
                              uint32_t                     vertexCount) {
  uint32_t i;

  if (!w || !prim || !input)
    return false;

  if (prim->indices) {
    const AkIndexArray *indices;
    uint32_t            stride;

    indices = prim->indices;
    stride  = prim->indexStride ? prim->indexStride : 1u;
    if (stride != 1u || input->indexOffset != 0u || indices->count < vertexCount)
      return false;

    switch (indices->componentType) {
      case AKT_UBYTE: {
        const uint8_t *items;

        items = (const uint8_t *)indices->items;
        for (i = 0; i < vertexCount; i++) {
          if (i > 0)
            dae_w_ch(w, ' ');
          dae_w_uint_fast(w, items[i]);
        }
        return true;
      }
      case AKT_USHORT: {
        const uint16_t *items;

        items = (const uint16_t *)indices->items;
        for (i = 0; i < vertexCount; i++) {
          if (i > 0)
            dae_w_ch(w, ' ');
          dae_w_uint_fast(w, items[i]);
        }
        return true;
      }
      case AKT_UINT: {
        const uint32_t *items;

        items = (const uint32_t *)indices->items;
        for (i = 0; i < vertexCount; i++) {
          if (i > 0)
            dae_w_ch(w, ' ');
          dae_w_uint_fast(w, items[i]);
        }
        return true;
      }
      default:
        return false;
    }
  }

  if (prim->indexAccessor) {
    IOIndexRows rows;

    if (!io_index_rows_init(&rows, prim->indexAccessor))
      return false;

    for (i = 0; i < vertexCount; i++) {
      if (i > 0)
        dae_w_ch(w, ' ');
      dae_w_uint_fast(w, io_index_rows_get_unchecked(&rows, i));
    }
    return true;
  }

  for (i = 0; i < vertexCount; i++) {
    if (i > 0)
      dae_w_ch(w, ' ');
    dae_w_uint_fast(w, i);
  }

  return true;
}

static
void
dae_write_vertices(DAEExpState * __restrict st,
                   AkMeshPrimitive * __restrict prim,
                   AkInput        ** __restrict inputs,
                   uint32_t                    inputCount,
                   uint32_t                 geomIdx,
                   uint32_t                 primIdx,
                   bool                     morphVertexGeometry) {
  DAEExpWriter *w;
  uint32_t      i;

  w = &st->w;
  dae_w_lit(w, "<vertices id=\"");
  dae_w_geom_prim_id(w, geomIdx, primIdx, DAE_EXP_NAME(vertices));
  dae_w_lit(w, "\">");

  for (i = 0; i < inputCount; i++) {
    AkInput    *input;
    DAEExpName  semanticName;
    const char *semantic;

    input = inputs[i];
    if (!dae_input_in_vertices(prim, input, morphVertexGeometry))
      continue;

    semantic = dae_semantic_name(input);
    if (!semantic || !*semantic)
      continue;
    semanticName = DAE_EXP_NAME_CSTR(semantic);

    dae_w_lit(w, "<input semantic=\"");
    if (input == prim->pos || input->semantic == AK_INPUT_POSITION)
      dae_w_name(w, DAE_EXP_NAME(POSITION));
    else
      dae_w_name(w, semanticName);
    dae_w_lit(w, "\" source=\"#");
    dae_w_geom_prim_id(w, geomIdx, primIdx, semanticName);
    dae_w_ch(w, '_');
    dae_w_uint_fast(w, i);
    dae_w_lit(w, "\"/>");
  }

  dae_w_lit(w, "</vertices>");
}

static
bool
dae_write_primitive(DAEExpState      * __restrict st,
                    AkMeshPrimitive  * __restrict prim,
                    uint32_t                      geomIdx,
                    uint32_t                      primIdx,
                    bool                          morphVertexGeometry) {
  DAEExpWriter *w;
  AkInput      *input;
  AkInput      *inputs[32];
  AkInput      *pInputs[32];
  uint32_t      inputCount;
  uint32_t      pInputCount;
  uint32_t      posInputIdx;
  uint32_t      i;
  uint32_t      vertexCount;
  DAEExpName    tag;
  bool          shareSingleIndex;

  if (!dae_primitive_supported(prim))
    return false;

  w           = &st->w;
  inputCount  = 0;
  pInputCount = 0;
  posInputIdx = UINT32_MAX;

  for (input = prim->input; input; input = input->next) {
    const char *semantic;
    bool        isPosition;

    if (!input->accessor)
      continue;

    semantic = dae_semantic_name(input);
    if (!semantic || !*semantic)
      continue;
    if (inputCount >= AK_ARRAY_LEN(inputs))
      return false;

    isPosition = input == prim->pos || input->semantic == AK_INPUT_POSITION;
    if (!dae_write_source(st, input, geomIdx, primIdx, inputCount)) {
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

  shareSingleIndex = st->indexMode == AK_DAE_EXPORT_INDEX_SINGLE
                     && prim->indexStride <= 1;
  dae_write_vertices(st,
                     prim,
                     inputs,
                     inputCount,
                     geomIdx,
                     primIdx,
                     morphVertexGeometry);

  if (!dae_primitive_tag(prim, &tag))
    return false;

  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_attr_uint(w, DAE_EXP_NAME(count), dae_primitive_count(prim));
  if (prim->material || st->materialCount > 0) {
    dae_w_lit(w, " material=\"");
    dae_w_prim_material_symbol(w, primIdx);
    dae_w_ch(w, '"');
  }
  dae_w_ch(w, '>');

  pInputs[pInputCount++] = inputs[posInputIdx];

  dae_w_lit(w, "<input semantic=\"");
  dae_w_name(w, DAE_EXP_NAME(VERTEX));
  dae_w_lit(w, "\" source=\"#");
  dae_w_geom_prim_id(w, geomIdx, primIdx, DAE_EXP_NAME(vertices));
  dae_w_lit(w, "\" offset=\"0\"/>");

  for (i = 0; i < inputCount; i++) {
    AkInput    *srcInput;
    DAEExpName  semanticName;
    const char *semantic;
    uint32_t    outOffset;

    srcInput = inputs[i];
    if (srcInput == prim->pos || srcInput->semantic == AK_INPUT_POSITION)
      continue;
    if (dae_input_in_vertices(prim, srcInput, morphVertexGeometry))
      continue;

    if (!shareSingleIndex) {
      if (pInputCount >= AK_ARRAY_LEN(pInputs))
        return false;
      pInputs[pInputCount] = srcInput;
      outOffset = pInputCount++;
    } else {
      outOffset = 0;
    }

    semantic = dae_semantic_name(srcInput);
    semanticName = DAE_EXP_NAME_CSTR(semantic);

    dae_w_lit(w, "<input semantic=\"");
    dae_w_name(w, semanticName);
    dae_w_lit(w, "\" source=\"#");
    dae_w_geom_prim_id(w, geomIdx, primIdx, semanticName);
    dae_w_ch(w, '_');
    dae_w_uint_fast(w, i);
    dae_w_lit(w, "\" offset=\"");
    dae_w_uint_fast(w, outOffset);
    dae_w_ch(w, '"');
    if ((srcInput->semantic == AK_INPUT_TEXCOORD
         || srcInput->semantic == AK_INPUT_UV
         || srcInput->semantic == AK_INPUT_COLOR)) {
      dae_w_lit(w, " set=\"");
      dae_w_uint_fast(w, srcInput->set);
      dae_w_ch(w, '"');
    }
    dae_w_lit(w, "/>");
  }

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
  if (pInputCount == 1u
      && prim->type != AK_PRIMITIVE_LINES
      && dae_write_p_single_input_fast(w, prim, pInputs[0], vertexCount)) {
  } else if (prim->type == AK_PRIMITIVE_LINES
      && ((AkLines *)prim)->mode == AK_LINE_LOOP) {
    bool firstIndex;

    firstIndex = true;
    for (i = 0; i < vertexCount; i++) {
      uint32_t edgeVerts[2];
      uint32_t edgeVertIdx;

      edgeVerts[0] = i;
      edgeVerts[1] = (i + 1u) == vertexCount ? 0u : i + 1u;

      for (edgeVertIdx = 0; edgeVertIdx < 2u; edgeVertIdx++) {
        uint32_t k;

        for (k = 0; k < pInputCount; k++) {
          if (!firstIndex)
            dae_w_ch(w, ' ');
          firstIndex = false;
          dae_w_uint_fast(w,
                     io_primitive_input_index(prim,
                                               pInputs[k],
                                               edgeVerts[edgeVertIdx]));
        }
      }
    }
  } else {
    for (i = 0; i < vertexCount; i++) {
      uint32_t k;

      for (k = 0; k < pInputCount; k++) {
        if (i > 0 || k > 0)
          dae_w_ch(w, ' ');
        dae_w_uint_fast(w,
                   io_primitive_input_index(prim,
                                             pInputs[k],
                                             i));
      }
    }
  }
  dae_w_lit(w, "</p>");
  dae_write_extra(w, prim->extra);
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');

  return w->result == AK_OK;
}


AK_HIDE
bool
dae_write_geometry(DAEExpState * __restrict st,
                   AkGeometry  * __restrict geom,
                   uint32_t                 geomIdx) {
  DAEExpWriter *w;
  AkMesh       *mesh;
  AkMeshPrimitive *prim;
  uint32_t      primIdx;
  bool          morphVertexGeometry;

  if (!geom || !geom->gdata)
    return false;

  if (geom->gdata->type == AK_GEOMETRY_BREP)
    return dae_write_brep_geometry(st, geom, geomIdx);

  if (geom->gdata->type == AK_GEOMETRY_SPLINE)
    return dae_write_spline_geometry(st, geom, geomIdx);

  if (geom->gdata->type != AK_GEOMETRY_MESH)
    return false;

  mesh = ak_objGet(geom->gdata);
  if (!mesh)
    return false;

  w = &st->w;
  dae_w_lit(w, "<geometry id=\"");
  dae_w_geom_id(w, geomIdx);
  if (geom->name || mesh->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, geom->name ? geom->name : mesh->name, true);
  }
  dae_w_lit(w, "\"><mesh>");

  morphVertexGeometry = st->morphVertexGeometries
                        && rb_find(st->morphVertexGeometries, geom);
  primIdx = 0;
  for (prim = mesh->primitive; prim; prim = prim->next, primIdx++) {
    if (!dae_write_primitive(st,
                             prim,
                             geomIdx,
                             primIdx,
                             morphVertexGeometry))
      return false;
  }

  dae_write_extra(w, mesh->extra);
  dae_w_lit(w, "</mesh>");
  dae_write_extra(w, geom->extra);
  dae_w_lit(w, "</geometry>");
  return w->result == AK_OK;
}
