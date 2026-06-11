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
#include "../../common/primitive.h"

#include <string.h>

AK_HIDE
uint32_t
dae_primitive_vertex_count(AkMeshPrimitive * __restrict prim) {
  return io_primitive_vertex_count(prim);
}

static
bool
dae_polygon_vcount_valid(AkPolygon * __restrict poly) {
  size_t   sum;
  size_t   i;
  uint32_t vertexCount;

  if (!poly || !poly->vcount || poly->vcount->count == 0)
    return false;

  if (poly->base.nPolygons != 0 && poly->base.nPolygons != poly->vcount->count)
    return false;

  vertexCount = dae_primitive_vertex_count(&poly->base);
  sum         = 0;
  for (i = 0; i < poly->vcount->count; i++) {
    AkUInt vc;

    vc = poly->vcount->items[i];
    if (vc < 3)
      return false;
    if (sum > (size_t)-1 - vc)
      return false;
    sum += vc;
  }

  return sum == vertexCount;
}

AK_HIDE
AkUInt
dae_primitive_input_index(AkMeshPrimitive * __restrict prim,
                          AkInput         * __restrict input,
                          uint32_t                     vertexIndex) {
  return io_primitive_input_index(prim, input, vertexIndex);
}

AK_HIDE
bool
dae_primitive_supported(AkMeshPrimitive * __restrict prim) {
  uint32_t vertexCount;

  if (!prim || !prim->pos || !prim->pos->accessor)
    return false;

  vertexCount = dae_primitive_vertex_count(prim);
  switch (prim->type) {
    case AK_PRIMITIVE_TRIANGLES: {
      AkTriangleMode mode;

      mode = ((AkTriangles *)prim)->mode;
      if (mode == 0 || mode == AK_TRIANGLES)
        return vertexCount % 3u == 0;
      if (mode == AK_TRIANGLE_STRIP || mode == AK_TRIANGLE_FAN)
        return vertexCount >= 3u;
      return false;
    }
    case AK_PRIMITIVE_LINES: {
      AkLineMode mode;

      mode = ((AkLines *)prim)->mode;
      if (mode == 0 || mode == AK_LINES)
        return vertexCount % 2u == 0;
      if (mode == AK_LINE_STRIP || mode == AK_LINE_LOOP)
        return vertexCount >= 2u;
      return false;
    }
    case AK_PRIMITIVE_POINTS:
      return true;
    case AK_PRIMITIVE_POLYGONS:
      return dae_polygon_vcount_valid((AkPolygon *)prim);
    default:
      return false;
  }
}

static
bool
dae_mesh_supported(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;
  uint32_t         primCount;

  if (!mesh)
    return false;

  if (!mesh->primitive)
    return true;

  primCount = 0;
  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (!dae_primitive_supported(prim))
      return false;
    primCount++;
  }

  return primCount > 0
         && (mesh->primitiveCount == 0 || mesh->primitiveCount == primCount);
}

AK_HIDE
bool
dae_geometry_supported(AkGeometry * __restrict geom) {
  if (!geom || !geom->gdata)
    return false;

  switch (geom->gdata->type) {
    case AK_GEOMETRY_MESH:
      return dae_mesh_supported(ak_objGet(geom->gdata));
    case AK_GEOMETRY_SPLINE:
      return dae_spline_supported(ak_objGet(geom->gdata));
    case AK_GEOMETRY_BREP:
      return dae_brep_supported(ak_objGet(geom->gdata));
    default:
      return false;
  }
}

AK_HIDE
bool
dae_primitive_tag(AkMeshPrimitive * __restrict prim,
                  DAEExpName      * __restrict tag) {
  switch (prim->type) {
    case AK_PRIMITIVE_TRIANGLES: {
      AkTriangleMode mode;

      mode = ((AkTriangles *)prim)->mode;
      if (mode == AK_TRIANGLE_STRIP) {
        *tag = DAE_EXP_NAME(tristrips);
        return true;
      }
      if (mode == AK_TRIANGLE_FAN) {
        *tag = DAE_EXP_NAME(trifans);
        return true;
      }
      *tag = DAE_EXP_NAME(triangles);
      return true;
    }
    case AK_PRIMITIVE_LINES:
      if (((AkLines *)prim)->mode == AK_LINE_STRIP)
        *tag = DAE_EXP_NAME(linestrips);
      else
        *tag = DAE_EXP_NAME(lines);
      return true;
    case AK_PRIMITIVE_POINTS:
      *tag = DAE_EXP_NAME_LIT("points");
      return true;
    case AK_PRIMITIVE_POLYGONS:
      *tag = DAE_EXP_NAME(polylist);
      return true;
    default:
      return false;
  }
}

AK_HIDE
uint32_t
dae_primitive_count(AkMeshPrimitive * __restrict prim) {
  uint32_t vertexCount;

  vertexCount = dae_primitive_vertex_count(prim);
  switch (prim->type) {
    case AK_PRIMITIVE_TRIANGLES:
      if (((AkTriangles *)prim)->mode == AK_TRIANGLE_STRIP
          || ((AkTriangles *)prim)->mode == AK_TRIANGLE_FAN)
        return 1u;
      return vertexCount / 3u;
    case AK_PRIMITIVE_LINES:
      if (((AkLines *)prim)->mode == AK_LINE_STRIP)
        return 1u;
      if (((AkLines *)prim)->mode == AK_LINE_LOOP)
        return vertexCount;
      return vertexCount / 2u;
    case AK_PRIMITIVE_POINTS:    return vertexCount;
    case AK_PRIMITIVE_POLYGONS:  return (uint32_t)((AkPolygon *)prim)->vcount->count;
    default:                     return 0;
  }
}
