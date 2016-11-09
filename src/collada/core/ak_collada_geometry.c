/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_geometry.h"
#include "../core/ak_collada_asset.h"
#include "ak_collada_mesh.h"
#include "ak_collada_spline.h"
#include "../brep/ak_collada_brep.h"

AkResult _assetkit_hide
ak_dae_geometry(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkGeometry ** __restrict dest) {
  AkGeometry *geometry;

  geometry = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*geometry),
                            true);

  _xml_readId(geometry);
  geometry->name = ak_xml_attr(xst, geometry, _s_dae_name);

  do {
    if (ak_xml_beginelm(xst, _s_dae_geometry))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, geometry, &assetInf);
      if (ret == AK_OK)
        geometry->inf = assetInf;

    } else if (ak_xml_eqelm(xst, _s_dae_mesh)
               || ak_xml_eqelm(xst, _s_dae_convex_mesh)) {
      AkMesh  *mesh;
      AkResult ret;

      ret = ak_dae_mesh(xst,
                        geometry,
                        (const char *)xst->nodeName,
                        &mesh,
                        true);
      if (ret == AK_OK)
        geometry->gdata = ak_objFrom(mesh);

    } else if (ak_xml_eqelm(xst, _s_dae_spline)) {
      AkSpline *spline;
      AkResult  ret;

      ret = ak_dae_spline(xst,
                          geometry,
                          true,
                          &spline);
      if (ret == AK_OK)
        geometry->gdata = ak_objFrom(spline);

    } else if (ak_xml_eqelm(xst, _s_dae_brep)) {
      AkBoundryRep *brep;
      AkResult      ret;

      ret = ak_dae_brep(xst,
                        geometry,
                        true,
                        &brep);
      if (ret == AK_OK)
        geometry->gdata = ak_objFrom(brep);

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          geometry,
                          nodePtr,
                          &tree,
                          NULL);
      geometry->extra = tree;
      
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = geometry;
  
  return AK_OK;
}
