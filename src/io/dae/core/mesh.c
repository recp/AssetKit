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

AK_INLINE
void
dae_mesh_appendPrim(AkMesh           * __restrict mesh,
                    AkMeshPrimitive ** __restrict lastPrim,
                    AkMeshPrimitive  * __restrict prim) {
  prim->next = NULL;
  prim->mesh = mesh;

  if (*lastPrim) (*lastPrim)->next = prim;
  else           mesh->primitive   = prim;

  *lastPrim = prim;
  mesh->primitiveCount++;
}

AK_HIDE
AkObject*
dae_mesh(DAEState   * __restrict dst,
         xml_t      * __restrict xml,
         AkGeometry * __restrict geom) {
  AkObject        *obj;
  AkMesh          *mesh;
  AkMeshPrimitive *lastPrim;
  AkHeap          *heap;
  double           profStart;
  uint32_t         m;

  profStart = DAE_PROF_START(dst);
  heap     = dst->heap;
  xml      = xml->val;
  lastPrim = NULL;

  obj  = ak_objAlloc(heap, geom, sizeof(*mesh), AK_GEOMETRY_MESH, true);
  mesh = ak_objGet(obj);

  mesh->geom         = geom;
  mesh->convexHullOf = DAE_XMLA_STRDUP(xml, heap, convex_hull_of, obj);

  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, source)) {
      (void)dae_source(dst, xml, NULL, 0);
    } else if (DAE_XML_TAG_EQ8(xml, vertices)) {
      (void)dae_vert(dst, xml, dst->tempmem);
    } else if ((DAE_XML_TAG_EQ(xml, triangles) & (m = AK_TRIANGLES))
            || (DAE_XML_TAG_EQ(xml, trifans)   & (m = AK_TRIANGLE_FAN))
            || (DAE_XML_TAG_EQ(xml, tristrips) & (m = AK_TRIANGLE_STRIP))) {
      AkTriangles *tri;
      
      if ((tri = dae_triangles(dst, xml, obj, m))) {
        dae_mesh_appendPrim(mesh, &lastPrim, &tri->base);

        if (tri->base.bindmaterial)
          ak_meshSetMaterial(&tri->base, tri->base.bindmaterial);
      }
    } else if ((DAE_XML_TAG_EQ(xml, polygons) & (m = AK_POLY_POLYGONS))
            || (DAE_XML_TAG_EQ8(xml, polylist) & (m = AK_POLY_POLYLIST))) {
      AkPolygon *poly;

      if ((poly = dae_poly(dst, xml, obj, m))) {
        dae_mesh_appendPrim(mesh, &lastPrim, &poly->base);

        if (poly->base.bindmaterial)
          ak_meshSetMaterial(&poly->base, poly->base.bindmaterial);
      }
      
    } else if (DAE_XML_TAG_EQ8(xml, lines)         & (m = AK_LINES)
           || (DAE_XML_TAG_EQ(xml, linestrips) & (m = AK_LINE_STRIP))) {
      AkLines *lines;
      
      if ((lines = dae_lines(dst, xml, obj, m))) {
        dae_mesh_appendPrim(mesh, &lastPrim, &lines->base);

        if (lines->base.bindmaterial)
          ak_meshSetMaterial(&lines->base, lines->base.bindmaterial);
      }
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      mesh->extra = tree_fromxml(heap, obj, xml);
    }

    xml = xml->next;
  }

  DAE_PROF_ACC(dst, profGeomMesh, profGeomMeshCount, profStart);

  return obj;
}
