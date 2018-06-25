/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_mesh.h"
#include "dae_source.h"
#include "dae_vertices.h"
#include "dae_lines.h"
#include "dae_polygons.h"
#include "dae_triangles.h"
#include "../../mesh/mesh_util.h"

#define k_s_dae_source     1
#define k_s_dae_vertices   2
#define k_s_dae_lines      3
#define k_s_dae_linestrips 4
#define k_s_dae_polygons   5
#define k_s_dae_polylist   6
#define k_s_dae_triangles  7
#define k_s_dae_trifans    8
#define k_s_dae_tristrips  9
#define k_s_dae_extra      10

static ak_enumpair meshMap[] = {
  {_s_dae_source,     k_s_dae_source},
  {_s_dae_vertices,   k_s_dae_vertices},
  {_s_dae_lines,      k_s_dae_lines},
  {_s_dae_linestrips, k_s_dae_linestrips},
  {_s_dae_polygons,   k_s_dae_polygons},
  {_s_dae_polylist,   k_s_dae_polylist},
  {_s_dae_triangles,  k_s_dae_triangles},
  {_s_dae_trifans,    k_s_dae_trifans},
  {_s_dae_tristrips,  k_s_dae_tristrips},
  {_s_dae_extra,      k_s_dae_extra},
};

static size_t meshMapLen = 0;

AkResult _assetkit_hide
ak_dae_mesh(AkXmlState * __restrict xst,
            void * __restrict memParent,
            const char * elm,
            AkMesh ** __restrict dest,
            bool asObject) {
  AkObject        *obj;
  AkSource        *last_source;
  AkMeshPrimitive *last_prim;
  AkMesh          *mesh;
  void            *memPtr;
  AkVertices      *vertices;
  AkXmlElmState    xest;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*mesh),
                      AK_GEOMETRY_TYPE_MESH,
                      true);

    mesh = ak_objGet(obj);

    memPtr = obj;
  } else {
    mesh = ak_heap_calloc(xst->heap, memParent, sizeof(*mesh));
    memPtr = mesh;
  }

  mesh->geom         = memParent;
  mesh->convexHullOf = ak_xml_attr(xst, memPtr, _s_dae_convex_hull_of);

  if (meshMapLen == 0) {
    meshMapLen = AK_ARRAY_LEN(meshMap);
    qsort(meshMap,
          meshMapLen,
          sizeof(meshMap[0]),
          ak_enumpair_cmp);
  }

  last_prim   = NULL;
  last_source = NULL;
  vertices    = NULL;

  ak_xest_init(xest, elm)

  do {
    const ak_enumpair *found;

    if (ak_xml_begin(&xest))
        break;

    found = bsearch(xst->nodeName,
                    meshMap,
                    meshMapLen,
                    sizeof(meshMap[0]),
                    ak_enumpair_cmp2);
    if (!found) {
      ak_xml_skipelm(xst);
      goto skip;
    }

    switch (found->val) {
      case k_s_dae_source: {
        AkSource *source;
        AkResult ret;

        ret = ak_dae_source(xst, memPtr, NULL, 0, &source);
        if (ret == AK_OK) {
          if (last_source)
            last_source->next = source;
          else
            mesh->source = source;

          last_source = source;
        }
        break;
      }
      case k_s_dae_vertices: {
        ak_dae_vertices(xst, memPtr, &vertices);
        break;
      }
      case k_s_dae_lines:
      case k_s_dae_linestrips: {
        AkLines   *lines;
        AkLineMode lineMode;
        AkResult   ret;

        if (found->val == k_s_dae_lines)
          lineMode = AK_LINE_MODE_LINES;
        else
          lineMode = AK_LINE_MODE_LINE_STRIP;

        ret = ak_dae_lines(xst,
                           memPtr,
                           lineMode,
                           &lines);
        if (ret == AK_OK) {
          if (last_prim)
            last_prim->next = &lines->base;
          else
            mesh->primitive = &lines->base;

          last_prim = &lines->base;

          last_prim->mesh = mesh;
          if (last_prim->material)
            ak_meshSetMaterial(last_prim,
                               last_prim->material);

          mesh->primitiveCount++;
        }

        break;
      }
      case k_s_dae_polygons:
      case k_s_dae_polylist: {
        AkPolygon    *polygon;
        AkPolygonMode mode;
        AkResult      ret;

        if (found->val == k_s_dae_polygons)
          mode = AK_POLYGON_MODE_POLYGONS;
        else
          mode = AK_POLYGON_MODE_POLYLIST;

        ret = ak_dae_polygon(xst,
                             memPtr,
                             found->key,
                             mode,
                             &polygon);
        if (ret == AK_OK) {
          if (last_prim)
            last_prim->next = &polygon->base;
          else
            mesh->primitive = &polygon->base;

          last_prim = &polygon->base;

          last_prim->mesh = mesh;
          if (last_prim->material)
            ak_meshSetMaterial(last_prim,
                               last_prim->material);

          mesh->primitiveCount++;
        }

        break;
      }
      case k_s_dae_triangles:
      case k_s_dae_trifans:
      case k_s_dae_tristrips: {
        AkTriangles   *triangles;
        AkTriangleMode mode;
        AkResult       ret;

        if (found->val == k_s_dae_triangles)
          mode = AK_TRIANGLE_MODE_TRIANGLES;
        else if (found->val == k_s_dae_tristrips)
          mode = AK_TRIANGLE_MODE_TRIANGLE_STRIP;
        else
          mode = AK_TRIANGLE_MODE_TRIANGLE_FAN;

        ret = ak_dae_triangles(xst,
                               memPtr,
                               found->key,
                               mode,
                               &triangles);
        if (ret == AK_OK) {
          if (last_prim)
            last_prim->next = &triangles->base;
          else
            mesh->primitive = &triangles->base;

          last_prim = &triangles->base;

          last_prim->mesh = mesh;
          if (last_prim->material)
            ak_meshSetMaterial(last_prim,
                               last_prim->material);

          mesh->primitiveCount++;
        }

        break;
      }
      case k_s_dae_extra: {
        xmlNodePtr nodePtr;
        AkTree    *tree;

        nodePtr = xmlTextReaderExpand(xst->reader);
        tree = NULL;

        ak_tree_fromXmlNode(xst->heap,
                            mesh,
                            nodePtr,
                            &tree,
                            NULL);
        mesh->extra = tree;

        ak_xml_skipelm(xst);
        break;
      }
      default:
        ak_xml_skipelm(xst);
        break;
    }

  skip:
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  /* copy <vertices> to all primitives */
  if (vertices) {
    AkMeshPrimitive *prim;
    AkInput         *inp;
    AkInput         *inpv;

    prim = mesh->primitive;

    while (prim) {
      inpv = vertices->input;
      while (inpv) {
        inp  = ak_heap_calloc(xst->heap, prim, sizeof(*inp));
        inp->semantic = inpv->semantic;
        if (inpv->semanticRaw)
          inp->semanticRaw = ak_heap_strdup(xst->heap,
                                            inp,
                                            inpv->semanticRaw);

        ak_url_dup(&inpv->source, prim, &inp->source);
        ak_xml_url_add(xst, &inp->source);

        inp->offset    = prim->reserved1;
        inp->set       = prim->reserved2;
        inp->next      = prim->input;
        prim->input    = inp;

        if (inp->semantic == AK_INPUT_SEMANTIC_POSITION)
          prim->pos = inp;

        prim->inputCount++;
        inpv = inpv->next;
      }
      prim = prim->next;
    }

    /* dont keep vertices */
    ak_free(vertices);
  }

  *dest = mesh;

  return AK_OK;
}
