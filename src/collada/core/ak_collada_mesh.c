/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_mesh.h"
#include "ak_collada_source.h"
#include "ak_collada_vertices.h"
#include "ak_collada_lines.h"
#include "ak_collada_polygons.h"
#include "ak_collada_triangles.h"

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
  AkMeshPrimitive *last_primitive;
  AkMesh          *mesh;
  void            *memPtr;

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

  mesh->convexHullOf = ak_xml_attr(xst, memPtr, _s_dae_convex_hull_of);

  if (meshMapLen == 0) {
    meshMapLen = AK_ARRAY_LEN(meshMap);
    qsort(meshMap,
          meshMapLen,
          sizeof(meshMap[0]),
          ak_enumpair_cmp);
  }

  last_primitive = NULL;
  last_source    = NULL;

  do {
    const ak_enumpair *found;

    if (ak_xml_beginelm(xst, elm))
        break;

    found = bsearch(xst->nodeName,
                    meshMap,
                    meshMapLen,
                    sizeof(meshMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_source: {
        AkSource *source;
        AkResult ret;

        ret = ak_dae_source(xst, memPtr, &source);
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
        AkVertices *vertices;
        AkResult ret;

        ret = ak_dae_vertices(xst, memPtr, &vertices);
        if (ret == AK_OK)
          mesh->vertices = vertices;

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
          if (last_primitive)
            last_primitive->next = &lines->base;
          else
            mesh->primitive = &lines->base;

          last_primitive = &lines->base;

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
          if (last_primitive)
            last_primitive->next = &polygon->base;
          else
            mesh->primitive = &polygon->base;

          last_primitive = &polygon->base;

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
          if (last_primitive)
            last_primitive->next = &triangles->base;
          else
            mesh->primitive = &triangles->base;

          last_primitive = &triangles->base;

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

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  *dest = mesh;

  return AK_OK;
}
