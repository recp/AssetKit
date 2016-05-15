/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_geometry.h"
#include "../ak_collada_asset.h"
#include "ak_collada_mesh.h"
#include "ak_collada_spline.h"
#include "../brep/ak_collada_brep.h"

AkResult _assetkit_hide
ak_dae_geometry(void * __restrict memParent,
                xmlTextReaderPtr reader,
                AkGeometry ** __restrict dest) {
  AkGeometry    *geometry;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  geometry = ak_calloc(memParent, sizeof(*geometry), false);

  _xml_readAttr(geometry, geometry->id, _s_dae_id);
  _xml_readAttr(geometry, geometry->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_geometry);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(geometry, reader, &assetInf);
      if (ret == AK_OK)
        geometry->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_mesh)
               || _xml_eqElm(_s_dae_convex_mesh)) {
      AkMesh  *mesh;
      AkResult ret;

      ret = ak_dae_mesh(geometry,
                        reader,
                        (const char *)nodeName,
                        &mesh,
                        true);
      if (ret == AK_OK)
        geometry->gdata = ak_objFrom(mesh);

    } else if (_xml_eqElm(_s_dae_spline)) {
      AkSpline *spline;
      AkResult  ret;

      ret = ak_dae_spline(geometry,
                        reader,
                        true,
                        &spline);
      if (ret == AK_OK)
        geometry->gdata = ak_objFrom(spline);

    } else if (_xml_eqElm(_s_dae_brep)) {
      AkBoundryRep *brep;
      AkResult      ret;

      ret = ak_dae_brep(geometry,
                        reader,
                        true,
                        &brep);
      if (ret == AK_OK)
        geometry->gdata = ak_objFrom(brep);

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(geometry, nodePtr, &tree, NULL);
      geometry->extra = tree;
      
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = geometry;
  
  return AK_OK;
}
