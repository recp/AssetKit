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
#include "../ak_collada_geomety_fixup.h"

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
ak_dae_mesh(AkHeap * __restrict heap,
            void * __restrict memParent,
            xmlTextReaderPtr reader,
            const char * elm,
            AkMesh ** __restrict dest,
            bool asObject) {
  AkObject    *obj;
  AkSource    *last_source;
  AkObject    *last_primitive;
  AkPolygon   *last_polygon;
  AkTriangles *last_triangles;
  AkMesh      *mesh;
  void        *memPtr;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  if (asObject) {
    obj = ak_objAlloc(heap,
                      memParent,
                      sizeof(*mesh),
                      AK_GEOMETRY_TYPE_MESH,
                      true,
                      false);

    mesh = ak_objGet(obj);

    memPtr = obj;
  } else {
    mesh = ak_heap_calloc(heap, memParent, sizeof(*mesh), false);
    memPtr = mesh;
  }

  _xml_readAttr(memPtr, mesh->convexHullOf, _s_dae_convex_hull_of);

  if (meshMapLen == 0) {
    meshMapLen = AK_ARRAY_LEN(meshMap);
    qsort(meshMap,
          meshMapLen,
          sizeof(meshMap[0]),
          ak_enumpair_cmp);
  }

  last_primitive = NULL;
  last_polygon   = NULL;
  last_triangles = NULL;
  last_source    = NULL;

  do {
    const ak_enumpair *found;

    _xml_beginElement(elm);

    found = bsearch(nodeName,
                    meshMap,
                    meshMapLen,
                    sizeof(meshMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_source: {
        AkSource *source;
        AkResult ret;

        ret = ak_dae_source(heap, memPtr, reader, &source);
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

        ret = ak_dae_vertices(heap, memPtr, reader, &vertices);
        if (ret == AK_OK)
          mesh->vertices = vertices;

        break;
      }
      case k_s_dae_lines:
      case k_s_dae_linestrips: {
        AkObject  *primitiveObj;
        AkLines   *lines;
        AkLineMode lineMode;
        AkResult   ret;

        if (found->val == k_s_dae_lines)
          lineMode = AK_LINE_MODE_LINES;
        else
          lineMode = AK_LINE_MODE_LINE_STRIP;

        ret = ak_dae_lines(heap,
                           memPtr,
                           reader,
                           lineMode,
                           true,
                           &lines);
        if (ret == AK_OK) {
          primitiveObj = ak_objFrom(lines);

          if (last_primitive)
            last_primitive->next = primitiveObj;
          else
            mesh->gprimitive = primitiveObj;

          last_primitive = primitiveObj;
        }

        break;
      }
      case k_s_dae_polygons:
      case k_s_dae_polylist: {
        AkObject  *primitiveObj;
        AkPolygon *polygon;
        AkPolygonMode mode;
        AkResult      ret;

        if (found->val == k_s_dae_polygons)
          mode = AK_POLYGON_MODE_POLYGONS;
        else
          mode = AK_POLYGON_MODE_POLYLIST;

        ret = ak_dae_polygon(heap,
                             memPtr,
                             reader,
                             found->key,
                             mode,
                             true,
                             &polygon);
        if (ret == AK_OK) {
          primitiveObj = ak_objFrom(polygon);

          if (last_primitive)
            last_primitive->next = primitiveObj;
          else
            mesh->gprimitive = primitiveObj;

          last_primitive = primitiveObj;

          if (last_polygon)
            last_polygon->next = polygon;
          last_polygon = polygon;
        }
        
        break;
      }
      case k_s_dae_triangles:
      case k_s_dae_trifans:
      case k_s_dae_tristrips: {
        AkObject    *primitiveObj;
        AkTriangles *triangles;
        AkTriangleMode mode;
        AkResult       ret;

        if (found->val == k_s_dae_triangles)
          mode = AK_TRIANGLE_MODE_TRIANGLES;
        else if (found->val == k_s_dae_tristrips)
          mode = AK_TRIANGLE_MODE_TRIANGLE_STRIP;
        else
          mode = AK_TRIANGLE_MODE_TRIANGLE_FAN;

        ret = ak_dae_triangles(heap,
                               memPtr,
                               reader,
                               found->key,
                               mode,
                               true,
                               &triangles);
        if (ret == AK_OK) {
          primitiveObj = ak_objFrom(triangles);

          if (last_primitive)
            last_primitive->next = primitiveObj;
          else
            mesh->gprimitive = primitiveObj;

          last_primitive = primitiveObj;

          if (last_triangles)
            last_triangles->next = triangles;
          last_triangles = triangles;
        }

        break;
      }
      case k_s_dae_extra: {
        xmlNodePtr nodePtr;
        AkTree    *tree;

        nodePtr = xmlTextReaderExpand(reader);
        tree = NULL;

        ak_tree_fromXmlNode(heap, mesh, nodePtr, &tree, NULL);
        mesh->extra = tree;

        _xml_skipElement;
        break;
      }
      default:
        _xml_skipElement;
        break;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  ak_dae_mesh_fixup(mesh);
  *dest = mesh;

  return AK_OK;
}
