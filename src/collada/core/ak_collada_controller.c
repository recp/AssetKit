/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_controller.h"
#include "../core/ak_collada_asset.h"
#include "ak_collada_skin.h"
#include "ak_collada_morph.h"

AkResult _assetkit_hide
ak_dae_controller(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  AkController ** __restrict dest) {
  AkController *controller;

  controller = ak_heap_calloc(xst->heap,
                              memParent,
                              sizeof(*controller),
                              true);

  _xml_readId(controller);
  _xml_readAttr(controller, controller->name, _s_dae_name);

  do {
    if (ak_xml_beginelm(xst, _s_dae_controller))
      break;

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, controller, &assetInf);
      if (ret == AK_OK)
        controller->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_skin)) {
      AkSkin  *skin;
      AkResult ret;

      ret = ak_dae_skin(xst, controller, true, &skin);
      if (ret == AK_OK)
        controller->data = ak_objFrom(skin);

    } else if (_xml_eqElm(_s_dae_morph)) {
      AkMorph *morph;
      AkResult ret;

      ret = ak_dae_morph(xst, controller, true, &morph);
      if (ret == AK_OK)
        controller->data = ak_objFrom(morph);

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          controller,
                          nodePtr,
                          &tree,
                          NULL);
      controller->extra = tree;

      ak_xml_skipelm(xst);;
    } else {
      ak_xml_skipelm(xst);;
    }
    
    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = controller;
  
  return AK_OK;
}
