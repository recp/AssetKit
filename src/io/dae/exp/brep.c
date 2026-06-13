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

#include "brep.h"
#include "source.h"
#include "../brep/semantic.h"
#include "../strpool.h"

#include <string.h>

typedef enum DAEExpBRepObjectSemantic {
  DAE_EXP_BREP_OBJECT_NONE = 0,
  DAE_EXP_BREP_OBJECT_CURVES,
  DAE_EXP_BREP_OBJECT_SURFACE_CURVES,
  DAE_EXP_BREP_OBJECT_SURFACES,
  DAE_EXP_BREP_OBJECT_VERTICES,
  DAE_EXP_BREP_OBJECT_EDGES,
  DAE_EXP_BREP_OBJECT_WIRES,
  DAE_EXP_BREP_OBJECT_FACES,
  DAE_EXP_BREP_OBJECT_SHELLS
} DAEExpBRepObjectSemantic;

static
bool
dae_accessor_layout(AkAccessor * __restrict acc,
                    size_t     * __restrict fillSize,
                    size_t     * __restrict stride) {
  if (!acc || !fillSize || !stride)
    return false;

  *fillSize = acc->fillByteSize
              ? acc->fillByteSize
              : (size_t)acc->bytesPerComponent * acc->componentCount;
  *stride   = acc->byteStride ? acc->byteStride : *fillSize;

  return acc->componentCount > 0
         && acc->bytesPerComponent > 0
         && *fillSize > 0
         && *stride >= *fillSize;
}

static
uint32_t
dae_accessor_effective_count(AkAccessor * __restrict acc) {
  size_t fillSize;
  size_t stride;
  size_t available;
  size_t maxRows;

  if (!acc || !acc->buffer || !acc->buffer->data)
    return 0;

  if (!dae_accessor_layout(acc, &fillSize, &stride)
      || acc->byteOffset > acc->buffer->length
      || acc->count == 0)
    return 0;

  available = acc->buffer->length - acc->byteOffset;
  if (available < fillSize)
    return 0;

  maxRows = 1u + (available - fillSize) / stride;
  return acc->count < maxRows ? acc->count : (uint32_t)maxRows;
}

static
bool
dae_accessor_bounds_valid(AkAccessor * __restrict acc) {
  size_t fillSize;
  size_t stride;

  if (!acc || !acc->buffer || !acc->buffer->data)
    return false;

  if (!dae_accessor_layout(acc, &fillSize, &stride)
      || acc->byteOffset > acc->buffer->length)
    return false;

  return acc->count == 0 || dae_accessor_effective_count(acc) > 0;
}

static
bool
dae_brep_accessor_supported(AkAccessor * __restrict acc) {
  if (!dae_accessor_bounds_valid(acc))
    return false;

  switch (acc->componentType) {
    case AKT_FLOAT:
    case AKT_DOUBLE:
    case AKT_INT:
    case AKT_UINT:
    case AKT_BYTE:
    case AKT_UBYTE:
    case AKT_SHORT:
    case AKT_USHORT:
    case AKT_BOOL:
    case AKT_IDREF:
    case AKT_NAME:
    case AKT_SIDREF:
    case AKT_TOKEN:
      return true;
    default:
      break;
  }

  return false;
}

static
DAEExpBRepObjectSemantic
dae_brep_object_semantic(const char * __restrict sem) {
  size_t len;

  if (!sem)
    return DAE_EXP_BREP_OBJECT_NONE;

  len = strlen(sem);
  switch (sem[0]) {
    case 'C':
      if (ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_CURVE, 5u))
        return DAE_EXP_BREP_OBJECT_CURVES;
      if (ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_CURVE2D, 7u))
        return DAE_EXP_BREP_OBJECT_SURFACE_CURVES;
      break;
    case 'S':
      if (ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_SURFACE, 7u))
        return DAE_EXP_BREP_OBJECT_SURFACES;
      if (ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_SHELL, 5u))
        return DAE_EXP_BREP_OBJECT_SHELLS;
      break;
    case 'V':
      if (ak_str_eq_packed_fast(sem,
                                len,
                                _s_dae_VERTEX_u64_exact,
                                _s_dae_VERTEX_len))
        return DAE_EXP_BREP_OBJECT_VERTICES;
      break;
    case 'E':
      if (ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_EDGE, 4u)
          || ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_EGDE, 4u))
        return DAE_EXP_BREP_OBJECT_EDGES;
      break;
    case 'W':
      if (ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_WIRE, 4u))
        return DAE_EXP_BREP_OBJECT_WIRES;
      break;
    case 'F':
      if (ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_FACE, 4u))
        return DAE_EXP_BREP_OBJECT_FACES;
      break;
    default:
      break;
  }

  return DAE_EXP_BREP_OBJECT_NONE;
}

static
bool
dae_brep_input_needs_source(AkInput * __restrict input) {
  const char *sem;

  sem = dae_semantic_name(input);
  if (!sem)
    return false;

  return dae_brep_object_semantic(sem) == DAE_EXP_BREP_OBJECT_NONE;
}

static
bool
dae_brep_topology_inputs_supported(AkInput * __restrict input) {
  for (; input; input = input->next) {
    if (dae_brep_input_needs_source(input)
        && !dae_brep_accessor_supported(input->accessor))
      return false;
  }

  return true;
}

static
bool
dae_vertices_supported(AkVertices * __restrict vertices) {
  AkInput *input;

  if (!vertices || !vertices->input)
    return false;

  for (input = vertices->input; input; input = input->next) {
    if (!dae_semantic_name(input)
        || !input->accessor
        || !dae_brep_accessor_supported(input->accessor))
      return false;
  }

  return true;
}

AK_HIDE
bool
dae_spline_supported(AkSpline * __restrict spline) {
  return spline && dae_vertices_supported(spline->cverts);
}

static
bool
dae_nurbs_supported(AkNurbs * __restrict nurbs) {
  return nurbs && dae_vertices_supported(nurbs->cverts);
}

static
bool
dae_nurbs_surface_supported(AkNurbsSurface * __restrict nurbsSurface) {
  return nurbsSurface && dae_vertices_supported(nurbsSurface->cverts);
}

static
bool
dae_brep_curve_supported(AkCurve * __restrict curve) {
  if (!curve || !curve->curve)
    return true;

  switch (curve->curve->type) {
    case AK_CURVE_LINE:
    case AK_CURVE_CIRCLE:
    case AK_CURVE_ELLIPSE:
    case AK_CURVE_PARABOLA:
    case AK_CURVE_HYPERBOLA:
      return true;
    case AK_CURVE_NURBS:
      return dae_nurbs_supported(ak_objGet(curve->curve));
    default:
      return false;
  }
}

AK_HIDE
bool
dae_brep_supported(AkBoundryRep * __restrict brep) {
  AkCurve   *curve;
  AkSurface *surface;

  if (!brep)
    return false;

  for (curve = brep->curves ? brep->curves->curve : NULL;
       curve;
       curve = curve->next) {
    if (!dae_brep_curve_supported(curve))
      return false;
  }

  for (curve = brep->surfaceCurves ? brep->surfaceCurves->curve : NULL;
       curve;
       curve = curve->next) {
    if (!dae_brep_curve_supported(curve))
      return false;
  }

  for (surface = brep->surfaces ? brep->surfaces->surface : NULL;
       surface;
       surface = surface->next) {
    if (!surface->surface)
      return false;
    switch (surface->surface->type) {
      case AK_SURFACE_CONE:
      case AK_SURFACE_PLANE:
      case AK_SURFACE_CYLINDER:
      case AK_SURFACE_SPHERE:
      case AK_SURFACE_TORUS:
        break;
      case AK_SURFACE_NURBS_SURFACE:
        if (!dae_nurbs_surface_supported(ak_objGet(surface->surface)))
          return false;
        break;
      case AK_SURFACE_SWEPT_SURFACE: {
        AkSweptSurface *sweptSurface;

        sweptSurface = ak_objGet(surface->surface);
        if (!sweptSurface || !dae_brep_curve_supported(sweptSurface->curve))
          return false;
        break;
      }
      default:
        return false;
    }
  }

  if (brep->vertices
      && brep->vertices->input
      && brep->vertices->input->accessor
      && !dae_brep_accessor_supported(brep->vertices->input->accessor))
    return false;

  return (!brep->edges || dae_brep_topology_inputs_supported(brep->edges->input))
         && (!brep->wires || dae_brep_topology_inputs_supported(brep->wires->input))
         && (!brep->faces || dae_brep_topology_inputs_supported(brep->faces->input))
         && (!brep->pcurves || dae_brep_topology_inputs_supported(brep->pcurves->input))
         && (!brep->shells || dae_brep_topology_inputs_supported(brep->shells->input))
         && (!brep->solids || dae_brep_topology_inputs_supported(brep->solids->input));
}

static
uint32_t
dae_curve_count(AkCurve * __restrict curve) {
  uint32_t count;

  count = 0;
  for (; curve; curve = curve->next)
    count++;

  return count;
}

static
uint32_t
dae_surface_count(AkSurface * __restrict surface) {
  uint32_t count;

  count = 0;
  for (; surface; surface = surface->next)
    count++;

  return count;
}

static
void
dae_w_brep_id(DAEExpWriter * __restrict w,
              uint32_t                  geomIdx,
              DAEExpName                suffix) {
  dae_w_geom_id(w, geomIdx);
  dae_w_lit(w, "_brep_");
  dae_w_name(w, suffix);
}

static
void
dae_w_brep_indexed_id(DAEExpWriter * __restrict w,
                      uint32_t                  geomIdx,
                      DAEExpName                suffix,
                      uint32_t                  idx) {
  dae_w_brep_id(w, geomIdx, suffix);
  dae_w_ch(w, '_');
  dae_w_uint(w, idx);
}

static
void
dae_w_brep_source_id(DAEExpWriter * __restrict w,
                     uint32_t                  geomIdx,
                     DAEExpName                owner,
                     uint32_t                  inputIdx) {
  dae_w_brep_id(w, geomIdx, owner);
  dae_w_lit(w, "_input_");
  dae_w_uint(w, inputIdx);
}

static
const unsigned char*
dae_accessor_row(AkAccessor * __restrict acc, uint32_t row) {
  size_t fillSize;
  size_t stride;

  fillSize = acc->fillByteSize
             ? acc->fillByteSize
             : (size_t)acc->bytesPerComponent * acc->componentCount;
  stride   = acc->byteStride ? acc->byteStride : fillSize;

  return (const unsigned char *)acc->buffer->data
         + acc->byteOffset
         + (size_t)row * stride;
}

static
DAEExpName
dae_brep_array_tag(AkTypeId type) {
  switch (type) {
    case AKT_FLOAT:
    case AKT_DOUBLE: return DAE_EXP_NAME(float_array);
    case AKT_BOOL:   return DAE_EXP_NAME(bool_array);
    case AKT_IDREF:  return DAE_EXP_NAME(IDREF_array);
    case AKT_NAME:   return DAE_EXP_NAME(Name_array);
    case AKT_SIDREF: return DAE_EXP_NAME(SIDREF_array);
    case AKT_TOKEN:  return DAE_EXP_NAME(token_array);
    default:         return DAE_EXP_NAME(int_array);
  }
}

static
DAEExpName
dae_brep_param_type(AkTypeId type) {
  switch (type) {
    case AKT_FLOAT:
    case AKT_DOUBLE: return DAE_EXP_NAME(float);
    case AKT_BOOL:   return DAE_EXP_NAME(bool);
    case AKT_IDREF:  return DAE_EXP_NAME_LIT("IDREF");
    case AKT_NAME:   return DAE_EXP_NAME_LIT("Name");
    case AKT_SIDREF: return DAE_EXP_NAME_LIT("SIDREF");
    case AKT_TOKEN:  return DAE_EXP_NAME_LIT("token");
    default:         return DAE_EXP_NAME(int);
  }
}

static
DAEExpName
dae_brep_param_name(const char * __restrict semantic, uint32_t idx) {
  if (semantic) {
    size_t len;

    len = strlen(semantic);
    if (ak_str_eq_packed_fast(semantic,
                              len,
                              _s_dae_POSITION_u64_exact,
                              _s_dae_POSITION_len))
      return dae_param_exp_name(idx);
    if (ak_str_eq_packed_fast(semantic, len, DAE_BREP_SEM_PARAM, 5u))
      return idx == 0
             ? DAE_EXP_NAME_LIT("START")
             : (idx == 1
                  ? DAE_EXP_NAME_LIT("END")
                  : DAE_EXP_NAME_LIT("PARAM"));
    if (idx == 0)
      return DAE_EXP_NAME_CSTR(semantic);
  }

  return dae_param_exp_name(idx);
}

static
void
dae_w_brep_scalar(DAEExpWriter       * __restrict w,
                  const unsigned char * __restrict ptr,
                  AkTypeId                        type) {
  switch (type) {
    case AKT_FLOAT: {
      float v;
      memcpy(&v, ptr, sizeof(v));
      dae_w_float(w, v);
      break;
    }
    case AKT_DOUBLE: {
      double v;
      memcpy(&v, ptr, sizeof(v));
      dae_w_double(w, v);
      break;
    }
    case AKT_BOOL: {
      bool v;
      memcpy(&v, ptr, sizeof(v));
      if (v)
        dae_w_lit(w, "true");
      else
        dae_w_lit(w, "false");
      break;
    }
    case AKT_BYTE: {
      int8_t v;
      memcpy(&v, ptr, sizeof(v));
      if (v < 0) {
        dae_w_ch(w, '-');
        dae_w_uint(w, (size_t)-(int)v);
      } else {
        dae_w_uint(w, (uint8_t)v);
      }
      break;
    }
    case AKT_UBYTE: {
      uint8_t v;
      memcpy(&v, ptr, sizeof(v));
      dae_w_uint(w, v);
      break;
    }
    case AKT_SHORT: {
      int16_t v;
      memcpy(&v, ptr, sizeof(v));
      if (v < 0) {
        dae_w_ch(w, '-');
        dae_w_uint(w, (size_t)-(int)v);
      } else {
        dae_w_uint(w, (uint16_t)v);
      }
      break;
    }
    case AKT_USHORT: {
      uint16_t v;
      memcpy(&v, ptr, sizeof(v));
      dae_w_uint(w, v);
      break;
    }
    case AKT_INT: {
      int32_t v;
      memcpy(&v, ptr, sizeof(v));
      if (v < 0) {
        dae_w_ch(w, '-');
        dae_w_uint(w, (size_t)-(int64_t)v);
      } else {
        dae_w_uint(w, (uint32_t)v);
      }
      break;
    }
    case AKT_UINT: {
      uint32_t v;
      memcpy(&v, ptr, sizeof(v));
      dae_w_uint(w, v);
      break;
    }
    case AKT_IDREF:
    case AKT_NAME:
    case AKT_SIDREF:
    case AKT_TOKEN: {
      const char *v;
      memcpy(&v, ptr, sizeof(v));
      if (v)
        dae_w_xml(w, v, false);
      break;
    }
    default:
      dae_w_ch(w, '0');
      break;
  }
}

static
bool
dae_write_brep_accessor_source(DAEExpState  * __restrict st,
                               AkAccessor   * __restrict acc,
                               uint32_t                  geomIdx,
                               DAEExpName                owner,
                               uint32_t                  inputIdx,
                               const char   * __restrict semantic) {
  DAEExpWriter *w;
  uint32_t      count;
  uint32_t      row;
  uint32_t      col;

  if (!dae_brep_accessor_supported(acc))
    return false;

  count = dae_accessor_effective_count(acc);
  w = &st->w;
  dae_w_lit(w, "<source id=\"");
  dae_w_brep_source_id(w, geomIdx, owner, inputIdx);
  dae_w_lit(w, "\"><");
  dae_w_name(w, dae_brep_array_tag(acc->componentType));
  dae_w_lit(w, " id=\"");
  dae_w_brep_source_id(w, geomIdx, owner, inputIdx);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint(w, (size_t)count * acc->componentCount);
  dae_w_lit(w, "\">");

  for (row = 0; row < count; row++) {
    const unsigned char *ptr;

    ptr = dae_accessor_row(acc, row);
    for (col = 0; col < acc->componentCount; col++) {
      if (row > 0 || col > 0)
        dae_w_ch(w, ' ');
      dae_w_brep_scalar(w,
                        ptr + (size_t)col * acc->bytesPerComponent,
                        acc->componentType);
    }
  }

  dae_w_lit(w, "</");
  dae_w_name(w, dae_brep_array_tag(acc->componentType));
  dae_w_lit(w, "><technique_common><accessor source=\"#");
  dae_w_brep_source_id(w, geomIdx, owner, inputIdx);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint(w, count);
  dae_w_lit(w, "\" stride=\"");
  dae_w_uint(w, acc->componentCount);
  dae_w_lit(w, "\">");

  for (col = 0; col < acc->componentCount; col++) {
    dae_w_lit(w, "<param name=\"");
    dae_w_name(w, dae_brep_param_name(semantic, col));
    dae_w_lit(w, "\" type=\"");
    dae_w_name(w, dae_brep_param_type(acc->componentType));
    dae_w_lit(w, "\"/>");
  }

  dae_w_lit(w, "</accessor></technique_common></source>");
  return w->result == AK_OK;
}

static
bool
dae_write_control_vertex_sources(DAEExpState * __restrict st,
                                 AkVertices  * __restrict vertices,
                                 uint32_t                 geomIdx,
                                 DAEExpName               owner) {
  AkInput  *input;
  uint32_t  inputIdx;

  if (!dae_vertices_supported(vertices))
    return false;

  inputIdx = 0;
  for (input = vertices->input; input; input = input->next, inputIdx++) {
    if (!dae_write_brep_accessor_source(st,
                                        input->accessor,
                                        geomIdx,
                                        owner,
                                        inputIdx,
                                        dae_semantic_name(input)))
      return false;
  }

  return true;
}

static
void
dae_write_control_vertices(DAEExpState * __restrict st,
                           AkVertices  * __restrict vertices,
                           uint32_t                 geomIdx,
                           DAEExpName               owner) {
  DAEExpWriter *w;
  AkInput      *input;
  uint32_t      inputIdx;

  w = &st->w;
  dae_w_lit(w, "<control_vertices id=\"");
  dae_w_brep_id(w, geomIdx, owner);
  dae_w_lit(w, "\">");

  inputIdx = 0;
  for (input = vertices->input; input; input = input->next, inputIdx++) {
    dae_w_lit(w, "<input semantic=\"");
    dae_w_xml(w, dae_semantic_name(input), true);
    dae_w_lit(w, "\" source=\"#");
    dae_w_brep_source_id(w, geomIdx, owner, inputIdx);
    dae_w_lit(w, "\"/>");
  }

  dae_write_extra(w, vertices ? vertices->extra : NULL);
  dae_w_lit(w, "</control_vertices>");
}

static
bool
dae_write_nurbs_body(DAEExpState * __restrict st,
                     AkNurbs     * __restrict nurbs,
                     uint32_t                 geomIdx,
                     DAEExpName               owner) {
  DAEExpWriter *w;

  if (!dae_nurbs_supported(nurbs))
    return false;

  w = &st->w;
  dae_w_lit(w, "<nurbs");
  dae_w_attr_uint(w, DAE_EXP_NAME(degree), nurbs->degree);
  if (nurbs->closed)
    dae_w_attr_uint(w, DAE_EXP_NAME(closed), 1);
  dae_w_ch(w, '>');

  if (!dae_write_control_vertex_sources(st, nurbs->cverts, geomIdx, owner))
    return false;

  dae_write_control_vertices(st, nurbs->cverts, geomIdx, owner);
  dae_write_extra(w, nurbs->extra);
  dae_w_lit(w, "</nurbs>");

  return w->result == AK_OK;
}

static
bool
dae_write_nurbs_surface_body(DAEExpState    * __restrict st,
                             AkNurbsSurface * __restrict nurbsSurface,
                             uint32_t                    geomIdx,
                             DAEExpName                  owner) {
  DAEExpWriter *w;

  if (!dae_nurbs_surface_supported(nurbsSurface))
    return false;

  w = &st->w;
  dae_w_lit(w, "<nurbs_surface");
  dae_w_attr_uint(w, DAE_EXP_NAME(degree_u), nurbsSurface->degree_u);
  dae_w_attr_uint(w, DAE_EXP_NAME(degree_v), nurbsSurface->degree_v);
  if (nurbsSurface->closed_u)
    dae_w_attr_uint(w, DAE_EXP_NAME(closed_u), 1);
  if (nurbsSurface->closed_v)
    dae_w_attr_uint(w, DAE_EXP_NAME(closed_v), 1);
  dae_w_ch(w, '>');

  if (!dae_write_control_vertex_sources(st,
                                        nurbsSurface->cverts,
                                        geomIdx,
                                        owner))
    return false;

  dae_write_control_vertices(st, nurbsSurface->cverts, geomIdx, owner);
  dae_write_extra(w, nurbsSurface->extra);
  dae_w_lit(w, "</nurbs_surface>");

  return w->result == AK_OK;
}

static
void
dae_write_brep_sidref_source(DAEExpState  * __restrict st,
                             uint32_t                  geomIdx,
                             DAEExpName                sourceName,
                             DAEExpName                itemName,
                             DAEExpName                paramName,
                             uint32_t                  count) {
  DAEExpWriter *w;
  uint32_t      i;

  if (count == 0)
    return;

  w = &st->w;
  dae_w_lit(w, "<source id=\"");
  dae_w_brep_id(w, geomIdx, sourceName);
  dae_w_lit(w, "\"><SIDREF_array id=\"");
  dae_w_brep_id(w, geomIdx, sourceName);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint(w, count);
  dae_w_lit(w, "\">");
  for (i = 0; i < count; i++) {
    if (i > 0)
      dae_w_ch(w, ' ');
    dae_w_geom_id(w, geomIdx);
    dae_w_ch(w, '/');
    dae_w_brep_indexed_id(w, geomIdx, itemName, i);
  }
  dae_w_lit(w, "</SIDREF_array><technique_common><accessor source=\"#");
  dae_w_brep_id(w, geomIdx, sourceName);
  dae_w_lit(w, "_array\" count=\"");
  dae_w_uint(w, count);
  dae_w_lit(w, "\" stride=\"1\"><param name=\"");
  dae_w_name(w, paramName);
  dae_w_lit(w, "\" type=\"SIDREF\"/></accessor></technique_common></source>");
}

static
void
dae_write_brep_float3(DAEExpWriter * __restrict w,
                      const float  * __restrict v) {
  dae_w_float(w, v[0]);
  dae_w_ch(w, ' ');
  dae_w_float(w, v[1]);
  dae_w_ch(w, ' ');
  dae_w_float(w, v[2]);
}

static
void
dae_write_brep_float4(DAEExpWriter * __restrict w,
                      const float  * __restrict v) {
  dae_w_float(w, v[0]);
  dae_w_ch(w, ' ');
  dae_w_float(w, v[1]);
  dae_w_ch(w, ' ');
  dae_w_float(w, v[2]);
  dae_w_ch(w, ' ');
  dae_w_float(w, v[3]);
}

static
void
dae_write_brep_orients(DAEExpWriter  * __restrict w,
                       AkFloatArrayL * __restrict orient) {
  for (; orient; orient = orient->next) {
    uint32_t i;

    dae_w_lit(w, "<orient>");
    for (i = 0; i < orient->count; i++) {
      if (i > 0)
        dae_w_ch(w, ' ');
      dae_w_float(w, orient->items[i]);
    }
    dae_w_lit(w, "</orient>");
  }
}

static
bool
dae_write_brep_curve(DAEExpState  * __restrict st,
                     AkCurve      * __restrict curve,
                     uint32_t                  geomIdx,
                     DAEExpName                itemName,
                     uint32_t                  idx) {
  DAEExpWriter *w;
  AkObject     *obj;

  w   = &st->w;
  obj = curve->curve;
  dae_w_lit(w, "<curve sid=\"");
  dae_w_brep_indexed_id(w, geomIdx, itemName, idx);
  if (!obj) {
    dae_w_lit(w, "\"/>");
    return w->result == AK_OK;
  }
  dae_w_lit(w, "\">");

  switch (obj->type) {
    case AK_CURVE_LINE: {
      AkLine *line;

      line = ak_objGet(obj);
      dae_w_lit(w, "<line><origin>");
      dae_write_brep_float3(w, line->origin);
      dae_w_lit(w, "</origin><direction>");
      dae_write_brep_float3(w, line->direction);
      dae_w_lit(w, "</direction>");
      dae_write_extra(w, line->extra);
      dae_w_lit(w, "</line>");
      break;
    }
    case AK_CURVE_CIRCLE: {
      AkCircle *circle;

      circle = ak_objGet(obj);
      dae_w_lit(w, "<circle><radius>");
      dae_w_float(w, circle->radius);
      dae_w_lit(w, "</radius>");
      dae_write_extra(w, circle->extra);
      dae_w_lit(w, "</circle>");
      break;
    }
    case AK_CURVE_ELLIPSE: {
      AkEllipse *ellipse;

      ellipse = ak_objGet(obj);
      dae_w_lit(w, "<ellipse><radius>");
      dae_w_float(w, ellipse->radius[0]);
      dae_w_ch(w, ' ');
      dae_w_float(w, ellipse->radius[1]);
      dae_w_lit(w, "</radius>");
      dae_write_extra(w, ellipse->extra);
      dae_w_lit(w, "</ellipse>");
      break;
    }
    case AK_CURVE_PARABOLA: {
      AkParabola *parabola;

      parabola = ak_objGet(obj);
      dae_w_lit(w, "<parabola><focal>");
      dae_w_float(w, parabola->focal);
      dae_w_lit(w, "</focal>");
      dae_write_extra(w, parabola->extra);
      dae_w_lit(w, "</parabola>");
      break;
    }
    case AK_CURVE_HYPERBOLA: {
      AkHyperbola *hyperbola;

      hyperbola = ak_objGet(obj);
      dae_w_lit(w, "<hyperbola><radius>");
      dae_w_float(w, hyperbola->radius[0]);
      dae_w_ch(w, ' ');
      dae_w_float(w, hyperbola->radius[1]);
      dae_w_lit(w, "</radius>");
      dae_write_extra(w, hyperbola->extra);
      dae_w_lit(w, "</hyperbola>");
      break;
    }
    case AK_CURVE_NURBS: {
      AkNurbs *nurbs;
      char     owner[64];

      nurbs = ak_objGet(obj);
      snprintf(owner,
               sizeof(owner),
               "%.*s_%u_cverts",
               (int)itemName.len,
               itemName.ptr,
               idx);
      if (!dae_write_nurbs_body(st, nurbs, geomIdx, DAE_EXP_NAME_CSTR(owner)))
        return false;
      break;
    }
    default:
      return false;
  }

  dae_write_brep_orients(w, curve->orient);
  dae_w_lit(w, "<origin>");
  dae_write_brep_float3(w, curve->origin);
  dae_w_lit(w, "</origin></curve>");

  return w->result == AK_OK;
}

static
bool
dae_write_brep_curves(DAEExpState  * __restrict st,
                      DAEExpName                tag,
                      DAEExpName                itemName,
                      AkCurve      * __restrict curve,
                      AkTree       * __restrict extra,
                      uint32_t                  geomIdx) {
  uint32_t idx;

  if (!curve && !extra)
    return true;

  dae_w_ch(&st->w, '<');
  dae_w_name(&st->w, tag);
  dae_w_ch(&st->w, '>');
  idx = 0;
  for (; curve; curve = curve->next) {
    if (!dae_write_brep_curve(st, curve, geomIdx, itemName, idx++))
      return false;
  }
  dae_write_extra(&st->w, extra);
  dae_w_lit(&st->w, "</");
  dae_w_name(&st->w, tag);
  dae_w_ch(&st->w, '>');

  return st->w.result == AK_OK;
}

static
bool
dae_write_brep_surface(DAEExpState  * __restrict st,
                       AkSurface    * __restrict surface,
                       uint32_t                  geomIdx,
                       uint32_t                  idx) {
  DAEExpWriter *w;
  AkObject     *obj;

  w   = &st->w;
  obj = surface->surface;
  dae_w_lit(w, "<surface sid=\"");
  dae_w_brep_indexed_id(w, geomIdx, DAE_EXP_NAME(surface), idx);
  if (surface->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, surface->name, true);
  }
  dae_w_lit(w, "\">");

  switch (obj->type) {
    case AK_SURFACE_CONE: {
      AkCone *cone;

      cone = ak_objGet(obj);
      dae_w_lit(w, "<cone><radius>");
      dae_w_float(w, cone->radius);
      dae_w_lit(w, "</radius><angle>");
      dae_w_float(w, cone->angle);
      dae_w_lit(w, "</angle>");
      dae_write_extra(w, cone->extra);
      dae_w_lit(w, "</cone>");
      break;
    }
    case AK_SURFACE_PLANE: {
      AkPlane *plane;

      plane = ak_objGet(obj);
      dae_w_lit(w, "<plane><equation>");
      dae_write_brep_float4(w, plane->equation);
      dae_w_lit(w, "</equation>");
      dae_write_extra(w, plane->extra);
      dae_w_lit(w, "</plane>");
      break;
    }
    case AK_SURFACE_CYLINDER: {
      AkCylinder *cylinder;

      cylinder = ak_objGet(obj);
      dae_w_lit(w, "<cylinder><radius>");
      dae_w_float(w, cylinder->radius[0]);
      if (cylinder->radius[1] != 0.0f) {
        dae_w_ch(w, ' ');
        dae_w_float(w, cylinder->radius[1]);
      }
      dae_w_lit(w, "</radius>");
      dae_write_extra(w, cylinder->extra);
      dae_w_lit(w, "</cylinder>");
      break;
    }
    case AK_SURFACE_SPHERE: {
      AkSphere *sphere;

      sphere = ak_objGet(obj);
      dae_w_lit(w, "<sphere><radius>");
      dae_w_float(w, sphere->radius);
      dae_w_lit(w, "</radius>");
      dae_write_extra(w, sphere->extra);
      dae_w_lit(w, "</sphere>");
      break;
    }
    case AK_SURFACE_TORUS: {
      AkTorus *torus;

      torus = ak_objGet(obj);
      dae_w_lit(w, "<torus><radius>");
      dae_w_float(w, torus->radius[0]);
      dae_w_ch(w, ' ');
      dae_w_float(w, torus->radius[1]);
      dae_w_lit(w, "</radius>");
      dae_write_extra(w, torus->extra);
      dae_w_lit(w, "</torus>");
      break;
    }
    case AK_SURFACE_NURBS_SURFACE: {
      AkNurbsSurface *nurbsSurface;
      char            owner[64];

      nurbsSurface = ak_objGet(obj);
      snprintf(owner, sizeof(owner), "surface_%u_cverts", idx);
      if (!dae_write_nurbs_surface_body(st,
                                        nurbsSurface,
                                        geomIdx,
                                        DAE_EXP_NAME_CSTR(owner)))
        return false;
      break;
    }
    case AK_SURFACE_SWEPT_SURFACE: {
      AkSweptSurface *sweptSurface;

      sweptSurface = ak_objGet(obj);
      if (!sweptSurface || !dae_brep_curve_supported(sweptSurface->curve))
        return false;

      dae_w_lit(w, "<swept_surface>");
      if (!dae_write_brep_curve(st,
                                sweptSurface->curve,
                                geomIdx,
                                DAE_EXP_NAME_LIT("swept_curve"),
                                idx))
        return false;
      dae_w_lit(w, "<direction>");
      dae_write_brep_float3(w, sweptSurface->direction);
      dae_w_lit(w, "</direction><origin>");
      dae_write_brep_float3(w, sweptSurface->origin);
      dae_w_lit(w, "</origin><axis>");
      dae_write_brep_float3(w, sweptSurface->axis);
      dae_w_lit(w, "</axis>");
      dae_write_extra(w, sweptSurface->extra);
      dae_w_lit(w, "</swept_surface>");
      break;
    }
    default:
      return false;
  }

  dae_write_brep_orients(w, surface->orient);
  dae_w_lit(w, "<origin>");
  dae_write_brep_float3(w, surface->origin);
  dae_w_lit(w, "</origin></surface>");

  return w->result == AK_OK;
}

static
bool
dae_write_brep_surfaces(DAEExpState * __restrict st,
                        AkSurface   * __restrict surface,
                        AkTree      * __restrict extra,
                        uint32_t                 geomIdx) {
  uint32_t idx;

  if (!surface && !extra)
    return true;

  dae_w_lit(&st->w, "<surfaces>");
  idx = 0;
  for (; surface; surface = surface->next) {
    if (!dae_write_brep_surface(st, surface, geomIdx, idx++))
      return false;
  }
  dae_write_extra(&st->w, extra);
  dae_w_lit(&st->w, "</surfaces>");

  return st->w.result == AK_OK;
}

static
bool
dae_write_brep_vertex_source(DAEExpState * __restrict st,
                             AkVertices  * __restrict vertices,
                             uint32_t                 geomIdx) {
  bool hasInput;

  if (!vertices)
    return true;

  hasInput = vertices->input && vertices->input->accessor;
  if (!hasInput)
    return true;

  if (!dae_write_brep_accessor_source(st,
                                      vertices->input->accessor,
                                      geomIdx,
                                      DAE_EXP_NAME_LIT("positions"),
                                      0,
                                      _s_dae_POSITION))
    return false;

  dae_w_lit(&st->w, "<vertices id=\"");
  dae_w_brep_id(&st->w, geomIdx, DAE_EXP_NAME(vertices));
  dae_w_lit(&st->w, "\">");
  dae_w_lit(&st->w, "<input semantic=\"");
  dae_w_name(&st->w, DAE_EXP_NAME(POSITION));
  dae_w_lit(&st->w, "\" source=\"#");
  dae_w_brep_source_id(&st->w, geomIdx, DAE_EXP_NAME_LIT("positions"), 0);
  dae_w_lit(&st->w, "\"/>");
  dae_write_extra(&st->w, vertices->extra);
  dae_w_lit(&st->w, "</vertices>");

  return st->w.result == AK_OK;
}

static
bool
dae_brep_input_object_source(DAEExpWriter     * __restrict w,
                             uint32_t                      geomIdx,
                             const char       * __restrict semantic) {
  switch (dae_brep_object_semantic(semantic)) {
    case DAE_EXP_BREP_OBJECT_CURVES:
      dae_w_brep_id(w, geomIdx, DAE_EXP_NAME(curves));
      return true;
    case DAE_EXP_BREP_OBJECT_SURFACE_CURVES:
      dae_w_brep_id(w, geomIdx, DAE_EXP_NAME(surface_curves));
      return true;
    case DAE_EXP_BREP_OBJECT_SURFACES:
      dae_w_brep_id(w, geomIdx, DAE_EXP_NAME(surfaces));
      return true;
    case DAE_EXP_BREP_OBJECT_VERTICES:
      dae_w_brep_id(w, geomIdx, DAE_EXP_NAME(vertices));
      return true;
    case DAE_EXP_BREP_OBJECT_EDGES:
      dae_w_brep_id(w, geomIdx, DAE_EXP_NAME(edges));
      return true;
    case DAE_EXP_BREP_OBJECT_WIRES:
      dae_w_brep_id(w, geomIdx, DAE_EXP_NAME(wires));
      return true;
    case DAE_EXP_BREP_OBJECT_FACES:
      dae_w_brep_id(w, geomIdx, DAE_EXP_NAME(faces));
      return true;
    case DAE_EXP_BREP_OBJECT_SHELLS:
      dae_w_brep_id(w, geomIdx, DAE_EXP_NAME(shells));
      return true;
    default:
      return false;
  }
}

static
bool
dae_write_brep_input_sources(DAEExpState  * __restrict st,
                             AkInput      * __restrict input,
                             uint32_t                  geomIdx,
                             DAEExpName                owner) {
  uint32_t idx;

  idx = 0;
  for (; input; input = input->next, idx++) {
    const char *semantic;

    semantic = dae_semantic_name(input);
    if (!semantic || !dae_brep_input_needs_source(input))
      continue;

    if (!dae_write_brep_accessor_source(st,
                                        input->accessor,
                                        geomIdx,
                                        owner,
                                        idx,
                                        semantic))
      return false;
  }

  return true;
}

static
void
dae_write_brep_inputs(DAEExpState  * __restrict st,
                      AkInput      * __restrict input,
                      uint32_t                  geomIdx,
                      DAEExpName                owner) {
  uint32_t idx;

  idx = 0;
  for (; input; input = input->next, idx++) {
    const char *semantic;

    semantic = dae_semantic_name(input);
    if (!semantic)
      continue;

    dae_w_lit(&st->w, "<input semantic=\"");
    dae_w_xml(&st->w, semantic, true);
    dae_w_lit(&st->w, "\" source=\"#");
    if (!dae_brep_input_object_source(&st->w, geomIdx, semantic))
      dae_w_brep_source_id(&st->w, geomIdx, owner, idx);
    dae_w_lit(&st->w, "\" offset=\"");
    dae_w_uint(&st->w, input->indexOffset);
    dae_w_lit(&st->w, "\"/>");
  }
}

static
void
dae_write_brep_uint_array(DAEExpWriter * __restrict w,
                          AkUIntArray  * __restrict array,
                          DAEExpName                tag) {
  size_t i;

  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
  if (array) {
    for (i = 0; i < array->count; i++) {
      if (i > 0)
        dae_w_ch(w, ' ');
      dae_w_uint(w, array->items[i]);
    }
  }
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');
}

static
bool
dae_write_brep_topology(DAEExpState  * __restrict st,
                        DAEExpName                tag,
                        DAEExpName                idSuffix,
                        const char   * __restrict name,
                        AkInput      * __restrict input,
                        AkUIntArray  * __restrict vcount,
                        AkUIntArray  * __restrict primitives,
                        AkTree       * __restrict extra,
                        uint32_t                  count,
                        uint32_t                  geomIdx) {
  DAEExpWriter *w;

  if (!dae_write_brep_input_sources(st, input, geomIdx, idSuffix))
    return false;

  w = &st->w;
  dae_w_ch(w, '<');
  dae_w_name(w, tag);
  dae_w_lit(w, " id=\"");
  dae_w_brep_id(w, geomIdx, idSuffix);
  if (name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, name, true);
  }
  dae_w_lit(w, "\" count=\"");
  dae_w_uint(w, count);
  dae_w_lit(w, "\">");
  dae_write_brep_inputs(st, input, geomIdx, idSuffix);
  if (vcount)
    dae_write_brep_uint_array(w, vcount, DAE_EXP_NAME(vcount));
  dae_write_brep_uint_array(w, primitives, DAE_EXP_NAME(p));
  dae_write_extra(w, extra);
  dae_w_lit(w, "</");
  dae_w_name(w, tag);
  dae_w_ch(w, '>');

  return w->result == AK_OK;
}

AK_HIDE
bool
dae_write_spline_geometry(DAEExpState * __restrict st,
                          AkGeometry  * __restrict geom,
                          uint32_t                 geomIdx) {
  DAEExpWriter *w;
  AkSpline     *spline;
  DAEExpName    owner;

  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_SPLINE)
    return false;

  spline = ak_objGet(geom->gdata);
  if (!dae_spline_supported(spline))
    return false;

  owner = DAE_EXP_NAME_LIT("spline_cverts");
  w     = &st->w;
  dae_w_lit(w, "<geometry id=\"");
  dae_w_geom_id(w, geomIdx);
  if (geom->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, geom->name, true);
  }
  dae_w_lit(w, "\"><spline");
  if (spline->closed)
    dae_w_attr_uint(w, DAE_EXP_NAME(closed), 1);
  dae_w_ch(w, '>');

  if (!dae_write_control_vertex_sources(st, spline->cverts, geomIdx, owner))
    return false;

  dae_write_control_vertices(st, spline->cverts, geomIdx, owner);
  dae_write_extra(w, spline->extra);
  dae_w_lit(w, "</spline>");
  dae_write_extra(w, geom->extra);
  dae_w_lit(w, "</geometry>");

  return w->result == AK_OK;
}

AK_HIDE
bool
dae_write_brep_geometry(DAEExpState * __restrict st,
                        AkGeometry  * __restrict geom,
                        uint32_t                 geomIdx) {
  DAEExpWriter *w;
  AkBoundryRep *brep;
  uint32_t      nCurves;
  uint32_t      nSurfaceCurves;
  uint32_t      nSurfaces;

  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_BREP)
    return false;

  brep = ak_objGet(geom->gdata);
  if (!dae_brep_supported(brep))
    return false;

  w = &st->w;
  dae_w_lit(w, "<geometry id=\"");
  dae_w_geom_id(w, geomIdx);
  if (geom->name) {
    dae_w_lit(w, "\" name=\"");
    dae_w_xml(w, geom->name, true);
  }
  dae_w_lit(w, "\"><brep>");

  nCurves        = dae_curve_count(brep->curves ? brep->curves->curve : NULL);
  nSurfaceCurves = dae_curve_count(brep->surfaceCurves
                                   ? brep->surfaceCurves->curve
                                   : NULL);
  nSurfaces      = dae_surface_count(brep->surfaces
                                     ? brep->surfaces->surface
                                     : NULL);

  if (!dae_write_brep_curves(st,
                             DAE_EXP_NAME(curves),
                             DAE_EXP_NAME(curve),
                             brep->curves ? brep->curves->curve : NULL,
                             brep->curves ? brep->curves->extra : NULL,
                             geomIdx)
      || !dae_write_brep_curves(st,
                                DAE_EXP_NAME(surface_curves),
                                DAE_EXP_NAME_LIT("surface_curve"),
                                brep->surfaceCurves
                                  ? brep->surfaceCurves->curve
                                  : NULL,
                                brep->surfaceCurves
                                  ? brep->surfaceCurves->extra
                                  : NULL,
                                geomIdx)
      || !dae_write_brep_surfaces(st,
                                  brep->surfaces
                                    ? brep->surfaces->surface
                                    : NULL,
                                  brep->surfaces
                                    ? brep->surfaces->extra
                                    : NULL,
                                  geomIdx))
    return false;

  dae_write_brep_sidref_source(st,
                               geomIdx,
                               DAE_EXP_NAME(curves),
                               DAE_EXP_NAME(curve),
                               DAE_EXP_NAME_LIT("CURVE"),
                               nCurves);
  dae_write_brep_sidref_source(st,
                               geomIdx,
                               DAE_EXP_NAME(surface_curves),
                               DAE_EXP_NAME_LIT("surface_curve"),
                               DAE_EXP_NAME_LIT("CURVE2D"),
                               nSurfaceCurves);
  dae_write_brep_sidref_source(st,
                               geomIdx,
                               DAE_EXP_NAME(surfaces),
                               DAE_EXP_NAME(surface),
                               DAE_EXP_NAME_LIT("SURFACE"),
                               nSurfaces);
  if (!dae_write_brep_vertex_source(st, brep->vertices, geomIdx))
    return false;

  if (brep->edges
      && !dae_write_brep_topology(st,
                                  DAE_EXP_NAME(edges),
                                  DAE_EXP_NAME(edges),
                                  brep->edges->name,
                                  brep->edges->input,
                                  NULL,
                                  brep->edges->primitives,
                                  brep->edges->extra,
                                  brep->edges->count,
                                  geomIdx))
    return false;

  if (brep->wires
      && !dae_write_brep_topology(st,
                                  DAE_EXP_NAME(wires),
                                  DAE_EXP_NAME(wires),
                                  brep->wires->name,
                                  brep->wires->input,
                                  brep->wires->vcount,
                                  brep->wires->primitives,
                                  brep->wires->extra,
                                  brep->wires->count,
                                  geomIdx))
    return false;

  if (brep->faces
      && !dae_write_brep_topology(st,
                                  DAE_EXP_NAME(faces),
                                  DAE_EXP_NAME(faces),
                                  brep->faces->name,
                                  brep->faces->input,
                                  brep->faces->vcount,
                                  brep->faces->primitives,
                                  brep->faces->extra,
                                  brep->faces->count,
                                  geomIdx))
    return false;

  if (brep->pcurves
      && !dae_write_brep_topology(st,
                                  DAE_EXP_NAME(pcurves),
                                  DAE_EXP_NAME(pcurves),
                                  brep->pcurves->name,
                                  brep->pcurves->input,
                                  brep->pcurves->vcount,
                                  brep->pcurves->primitives,
                                  brep->pcurves->extra,
                                  brep->pcurves->count,
                                  geomIdx))
    return false;

  if (brep->shells
      && !dae_write_brep_topology(st,
                                  DAE_EXP_NAME(shells),
                                  DAE_EXP_NAME(shells),
                                  brep->shells->name,
                                  brep->shells->input,
                                  brep->shells->vcount,
                                  brep->shells->primitives,
                                  brep->shells->extra,
                                  brep->shells->count,
                                  geomIdx))
    return false;

  if (brep->solids
      && !dae_write_brep_topology(st,
                                  DAE_EXP_NAME(solids),
                                  DAE_EXP_NAME(solids),
                                  brep->solids->name,
                                  brep->solids->input,
                                  brep->solids->vcount,
                                  brep->solids->primitives,
                                  brep->solids->extra,
                                  brep->solids->count,
                                  geomIdx))
    return false;

  dae_write_extra(w, brep->extra);
  dae_w_lit(w, "</brep>");
  dae_write_extra(w, geom->extra);
  dae_w_lit(w, "</geometry>");
  return w->result == AK_OK;
}
