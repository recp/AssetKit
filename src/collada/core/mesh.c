/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "mesh.h"
#include "source.h"
#include "vert.h"
#include "triangle.h"
#include "poly.h"
#include "line.h"

AkObject* _assetkit_hide
dae_mesh(DAEState   * __restrict dst,
         xml_t      * __restrict xml,
         AkGeometry * __restrict geom) {
  AkVertices  *vert;
  AkObject    *obj;
  AkMesh      *mesh;
  AkDoc       *doc;
  AkHeap      *heap;
  uint32_t     m;

  heap = dst->heap;
  doc  = dst->doc;
  xml  = xml->val;

  obj  = ak_objAlloc(heap, geom, sizeof(*mesh), AK_GEOMETRY_MESH, true);
  mesh = ak_objGet(obj);

  mesh->geom         = geom;
  mesh->convexHullOf = xmla_strdup_by(xml, heap, _s_dae_convex_hull_of, obj);

  vert = NULL;
  
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_source)) {
      (void)dae_source(dst, xml, NULL, 0);
    } else if (xml_tag_eq(xml, _s_dae_vertices)) {
      vert = dae_vert(dst, xml, obj);
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

  /* copy <vertices> to all primitives */
  if (vert) {
    AkMeshPrimitive *prim;
    AkInput         *inp;
    AkInput         *inpv;
    bool             setMeshInfo;
    
    prim        = mesh->primitive;
    setMeshInfo = false;
    
    while (prim) {
      inpv = vert->input;
      while (inpv) {
        AkURL *url;
        
        inp  = ak_heap_calloc(heap, prim, sizeof(*inp));
        inp->semantic = inpv->semantic;
        if (inpv->semanticRaw)
          inp->semanticRaw = ak_heap_strdup(heap, inp, inpv->semanticRaw);
        
        inp->offset = prim->reserved1;
        inp->set    = prim->reserved2;
        inp->next   = prim->input;
        prim->input = inp;
        
        if (inp->semantic == AK_INPUT_SEMANTIC_POSITION) {
          prim->pos = inp;
          if (!setMeshInfo) {
            AkDaeMeshInfo *mi;
            
            mi      = ak_heap_calloc(heap, NULL, sizeof(*mi));
            mi->pos = inp;
            
            rb_insert(dst->meshInfo, mesh, mi);
            
            setMeshInfo = true;
          }
        }
        
        url = rb_find(dst->inputmap, inpv);
        rb_insert(dst->inputmap, inp, url);
        rb_remove(dst->inputmap, inpv);
        ak_mem_setp(url, inp);
        
        prim->inputCount++;
        inpv = inpv->next;
      }
      prim = prim->next;
    }
    
    /* dont keep vertices */
    ak_free(vert);
  }

  return obj;
}
