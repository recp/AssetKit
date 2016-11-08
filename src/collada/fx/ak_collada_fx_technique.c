/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_technique.h"

#include "../core/ak_collada_asset.h"
#include "../core/ak_collada_annotate.h"

#include "ak_collada_fx_blinn_phong.h"
#include "ak_collada_fx_constant.h"
#include "ak_collada_fx_lambert.h"
#include "ak_collada_fx_pass.h"

AkResult _assetkit_hide
ak_dae_techniqueFx(AkDaeState * __restrict daestate,
                   void * __restrict memParent,
                   AkTechniqueFx ** __restrict dest) {
  AkTechniqueFx *technique;
  AkAnnotate    *last_annotate;

  technique = ak_heap_calloc(daestate->heap,
                             memParent,
                             sizeof(*technique),
                             true);

  _xml_readId(technique);
  _xml_readAttr(technique, technique->sid, _s_dae_sid);

  last_annotate = NULL;

  do {
    _xml_beginElement(_s_dae_technique);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(daestate, technique, &assetInf);
      if (ret == AK_OK)
        technique->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_annotate)) {
      AkAnnotate *annotate;
      AkResult    ret;

      ret = ak_dae_annotate(daestate, technique, &annotate);

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

      ret = ak_dae_fxPass(daestate, technique, &pass);
      if (ret == AK_OK)
        technique->pass = pass;

    } else if (_xml_eqElm(_s_dae_blinn)) {
      ak_blinn_phong * blinn_phong;
      AkResult ret;

      ret = ak_dae_blinn_phong(daestate,
                               technique,
                               (const char *)daestate->nodeName,
                               &blinn_phong);
      if (ret == AK_OK)
        technique->blinn = (AkBlinn *)blinn_phong;

    } else if (_xml_eqElm(_s_dae_constant)) {
      AkConstantFx * constant_fx;
      AkResult ret;

      ret = ak_dae_fxConstant(daestate, technique, &constant_fx);
      if (ret == AK_OK)
        technique->constant = constant_fx;

    } else if (_xml_eqElm(_s_dae_lambert)) {
      AkLambert * lambert;
      AkResult ret;

      ret = ak_dae_fxLambert(daestate, technique, &lambert);
      if (ret == AK_OK)
        technique->lambert = lambert;

    } else if (_xml_eqElm(_s_dae_phong)) {
      ak_blinn_phong * blinn_phong;
      AkResult ret;

      ret = ak_dae_blinn_phong(daestate,
                               technique,
                               (const char *)daestate->nodeName,
                               &blinn_phong);
      if (ret == AK_OK)
        technique->phong = (AkPhong *)blinn_phong;

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(daestate->reader);
      tree = NULL;

      ak_tree_fromXmlNode(daestate->heap,
                          technique,
                          nodePtr,
                          &tree,
                          NULL);
      technique->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (daestate->nodeRet);
  
  *dest = technique;
  
  return AK_OK;
}
