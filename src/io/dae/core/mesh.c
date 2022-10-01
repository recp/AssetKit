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
#include "vert.h"
#include "triangle.h"
#include "poly.h"
#include "line.h"

AK_HIDE
AkObject*
dae_mesh(DAEState   * __restrict dst,
         xml_t      * __restrict xml,
         AkGeometry * __restrict geom) {
  AkObject   *obj;
  AkMesh     *mesh;
  AkHeap     *heap;
  uint32_t    m;

  heap = dst->heap;
  xml  = xml->val;

  obj  = ak_objAlloc(heap, geom, sizeof(*mesh), AK_GEOMETRY_MESH, true);
  mesh = ak_objGet(obj);

  mesh->geom         = geom;
  mesh->convexHullOf = xmla_strdup_by(xml, heap, _s_dae_convex_hull_of, obj);

  while (xml) {
    if (xml_tag_eq(xml, _s_dae_source)) {
      (void)dae_source(dst, xml, NULL, 0);
    } else if (xml_tag_eq(xml, _s_dae_vertices)) {
      (void)dae_vert(dst, xml, dst->tempmem);
    } else if ((xml_tag_eq(xml, _s_dae_triangles) & (m = AK_TRIANGLES))
            || (xml_tag_eq(xml, _s_dae_trifans)   & (m = AK_TRIANGLE_FAN))
            || (xml_tag_eq(xml, _s_dae_tristrips) & (m = AK_TRIANGLE_STRIP))) {
      AkTriangles *tri;
      
      if ((tri = dae_triangles(dst, xml, obj, m))) {
        tri->base.next  = mesh->primitive;
        mesh->primitive = &tri->base;

        tri->base.mesh  = mesh;
        if (tri->base.bindmaterial)
          ak_meshSetMaterial(&tri->base, tri->base.bindmaterial);
        
        mesh->primitiveCount++;
      }
    } else if ((xml_tag_eq(xml, _s_dae_polygons) & (m = AK_POLY_POLYGONS))
            || (xml_tag_eq(xml, _s_dae_polylist) & (m = AK_POLY_POLYLIST))) {
      AkPolygon *poly;

      if ((poly = dae_poly(dst, xml, obj, m))) {
        poly->base.next = mesh->primitive;
        mesh->primitive = &poly->base;
      
        poly->base.mesh = mesh;
        if (poly->base.bindmaterial)
          ak_meshSetMaterial(&poly->base, poly->base.bindmaterial);
        
        mesh->primitiveCount++;
      }
      
    } else if (xml_tag_eq(xml, _s_dae_lines)      & (m = AK_LINES)
           || (xml_tag_eq(xml, _s_dae_linestrips) & (m = AK_LINE_STRIP))) {
      AkLines *lines;
      
      if ((lines = dae_lines(dst, xml, obj, m))) {
        lines->base.next = mesh->primitive;
        mesh->primitive  = &lines->base;

        lines->base.mesh = mesh;
        if (lines->base.bindmaterial)
          ak_meshSetMaterial(&lines->base, lines->base.bindmaterial);
        
        mesh->primitiveCount++;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      mesh->extra = tree_fromxml(heap, obj, xml);
    }

    xml = xml->next;
  }

  return obj;
}
