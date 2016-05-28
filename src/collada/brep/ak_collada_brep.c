/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_brep.h"
#include "../core/ak_collada_source.h"
#include "../core/ak_collada_vertices.h"
#include "ak_collada_brep_curve.h"
#include "ak_collada_brep_surface.h"
#include "ak_collada_brep_topology.h"

#define k_s_dae_curves         1
#define k_s_dae_surface_curves 2
#define k_s_dae_surfaces       3
#define k_s_dae_source         4
#define k_s_dae_vertices       5
#define k_s_dae_edges          6
#define k_s_dae_wires          7
#define k_s_dae_faces          8
#define k_s_dae_pcurves        9
#define k_s_dae_shells         10
#define k_s_dae_solids         11
#define k_s_dae_extra          12

static ak_enumpair brepMap[] = {
  {_s_dae_curves,         k_s_dae_curves},
  {_s_dae_surface_curves, k_s_dae_surface_curves},
  {_s_dae_surfaces,       k_s_dae_surfaces},
  {_s_dae_source,         k_s_dae_source},
  {_s_dae_vertices,       k_s_dae_vertices},
  {_s_dae_edges,          k_s_dae_edges},
  {_s_dae_wires,          k_s_dae_wires},
  {_s_dae_faces,          k_s_dae_faces},
  {_s_dae_pcurves,        k_s_dae_pcurves},
  {_s_dae_shells,         k_s_dae_shells},
  {_s_dae_solids,         k_s_dae_solids},
  {_s_dae_extra,          k_s_dae_extra},
};

static size_t brepMapLen = 0;

AkResult _assetkit_hide
ak_dae_brep(AkHeap * __restrict heap,
            void * __restrict memParent,
            xmlTextReaderPtr reader,
            bool asObject,
            AkBoundryRep ** __restrict dest) {
  AkObject *obj;
  AkSource *last_source;
  AkBoundryRep *brep;
  void         *memPtr;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  if (asObject) {
    obj = ak_objAlloc(heap,
                      memParent,
                      sizeof(*brep),
                      0,
                      true,
                      false);

    brep = ak_objGet(obj);

    memPtr = obj;
  } else {
    brep = ak_heap_calloc(heap, memParent, sizeof(*brep), false);
    memPtr = brep;
  }

  if (brepMapLen == 0) {
    brepMapLen = AK_ARRAY_LEN(brepMap);
    qsort(brepMap,
          brepMapLen,
          sizeof(brepMap[0]),
          ak_enumpair_cmp);
  }

  last_source = NULL;

  do {
    const ak_enumpair *found;

    _xml_beginElement(_s_dae_brep);

    found = bsearch(nodeName,
                    brepMap,
                    brepMapLen,
                    sizeof(brepMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_curves: {
        AkCurves *curves;
        AkResult  ret;

        ret = ak_dae_curves(heap, memPtr, reader, &curves);
        if (ret == AK_OK)
          brep->curves = curves;

        break;
      }
      case k_s_dae_surface_curves: {
        AkCurves *curves;
        AkResult  ret;

        ret = ak_dae_curves(heap, memPtr, reader, &curves);
        if (ret == AK_OK)
          brep->surfaceCurves = curves;

        break;
      }
      case k_s_dae_surfaces: {
        AkSurfaces *surfaces;
        AkResult    ret;

        ret = ak_dae_surfaces(heap, memPtr, reader, &surfaces);
        if (ret == AK_OK)
          brep->surfaces = surfaces;

        break;
      }
      case k_s_dae_source: {
        AkSource *source;
        AkResult ret;

        ret = ak_dae_source(heap, memPtr, reader, &source);
        if (ret == AK_OK) {
          if (last_source)
            last_source->next = source;
          else
            brep->source = source;

          last_source = source;
        }
        break;
      }
      case k_s_dae_vertices: {
        AkVertices *vertices;
        AkResult ret;

        ret = ak_dae_vertices(heap, memPtr, reader, &vertices);
        if (ret == AK_OK)
          brep->vertices = vertices;

        break;
      }
      case k_s_dae_edges: {
        AkEdges *edges;
        AkResult ret;

        ret = ak_dae_edges(heap, memPtr, reader, &edges);
        if (ret == AK_OK)
          brep->edges = edges;

        break;
      }
      case k_s_dae_wires: {
        AkWires *wires;
        AkResult ret;

        ret = ak_dae_wires(heap, memPtr, reader, &wires);
        if (ret == AK_OK)
          brep->wires = wires;

        break;
      }
      case k_s_dae_faces: {
        AkFaces *faces;
        AkResult ret;

        ret = ak_dae_faces(heap, memPtr, reader, &faces);
        if (ret == AK_OK)
          brep->faces = faces;

        break;
      }
      case k_s_dae_pcurves: {
        AkPCurves *pcurves;
        AkResult   ret;

        ret = ak_dae_pcurves(heap, memPtr, reader, &pcurves);
        if (ret == AK_OK)
          brep->pcurves = pcurves;

        break;
      }
      case k_s_dae_shells: {
        AkShells *shells;
        AkResult  ret;

        ret = ak_dae_shells(heap, memPtr, reader, &shells);
        if (ret == AK_OK)
          brep->shells = shells;

        break;
      }
      case k_s_dae_solids: {
        AkSolids *solids;
        AkResult  ret;

        ret = ak_dae_solids(heap, memPtr, reader, &solids);
        if (ret == AK_OK)
          brep->solids = solids;

        break;
      }
      case k_s_dae_extra: {
        xmlNodePtr nodePtr;
        AkTree    *tree;

        nodePtr = xmlTextReaderExpand(reader);
        tree = NULL;

        ak_tree_fromXmlNode(heap, memPtr, nodePtr, &tree, NULL);
        brep->extra = tree;

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
  
  *dest = brep;
  
  return AK_OK;
}
