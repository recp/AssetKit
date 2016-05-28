/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_technique.h"

#include "../ak_collada_asset.h"
#include "../ak_collada_annotate.h"

#include "ak_collada_fx_blinn_phong.h"
#include "ak_collada_fx_constant.h"
#include "ak_collada_fx_lambert.h"
#include "ak_collada_fx_pass.h"

AkResult _assetkit_hide
ak_dae_techniqueFx(AkHeap * __restrict heap,
                   void * __restrict memParent,
                   xmlTextReaderPtr reader,
                   AkTechniqueFx ** __restrict dest) {
  AkTechniqueFx *technique;
  AkAnnotate    *last_annotate;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  technique = ak_heap_calloc(heap, memParent, sizeof(*technique), false);

  _xml_readAttr(technique, technique->id, _s_dae_id);
  _xml_readAttr(technique, technique->sid, _s_dae_sid);

  last_annotate = NULL;

  do {
    _xml_beginElement(_s_dae_technique);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(heap, technique, reader, &assetInf);
      if (ret == AK_OK)
        technique->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_annotate)) {
      AkAnnotate *annotate;
      AkResult    ret;

      ret = ak_dae_annotate(heap, technique, reader, &annotate);

      if (ret == AK_OK) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          technique->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (_xml_eqElm(_s_dae_pass)) {
      AkPass * pass;
      AkResult ret;

      ret = ak_dae_fxPass(heap, technique, reader, &pass);
      if (ret == AK_OK)
        technique->pass = pass;

    } else if (_xml_eqElm(_s_dae_blinn)) {
      ak_blinn_phong * blinn_phong;
      AkResult ret;

      ret = ak_dae_blinn_phong(heap,
                               technique,
                               reader,
                               (const char *)nodeName,
                               &blinn_phong);
      if (ret == AK_OK)
        technique->blinn = (AkBlinn *)blinn_phong;

    } else if (_xml_eqElm(_s_dae_constant)) {
      AkConstantFx * constant_fx;
      AkResult ret;

      ret = ak_dae_fxConstant(heap, technique, reader, &constant_fx);
      if (ret == AK_OK)
        technique->constant = constant_fx;

    } else if (_xml_eqElm(_s_dae_lambert)) {
      AkLambert * lambert;
      AkResult ret;

      ret = ak_dae_fxLambert(heap, technique, reader, &lambert);
      if (ret == AK_OK)
        technique->lambert = lambert;

    } else if (_xml_eqElm(_s_dae_phong)) {
      ak_blinn_phong * blinn_phong;
      AkResult ret;

      ret = ak_dae_blinn_phong(heap,
                               technique,
                               reader,
                               (const char *)nodeName,
                               &blinn_phong);
      if (ret == AK_OK)
        technique->phong = (AkPhong *)blinn_phong;

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, technique, nodePtr, &tree, NULL);
      technique->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = technique;
  
  return AK_OK;

}
