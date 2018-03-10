/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_fx_technique.h"

#include "../core/dae_asset.h"
#include "../core/dae_annotate.h"

#include "../1.4/dae14_image.h"

#include "dae_fx_blinn_phong.h"
#include "dae_fx_constant.h"
#include "dae_fx_lambert.h"
#include "dae_fx_pass.h"

AkResult _assetkit_hide
ak_dae_techniqueFx(AkXmlState * __restrict xst,
                   void * __restrict memParent,
                   AkTechniqueFx ** __restrict dest) {
  AkTechniqueFx *technique;
  AkAnnotate    *last_annotate;
  AkXmlElmState  xest;

  technique = ak_heap_calloc(xst->heap,
                             memParent,
                             sizeof(*technique));
  ak_setypeid(technique, AKT_TECHNIQUE_FX);

  ak_xml_readid(xst, technique);
  ak_xml_readsid(xst, technique);

  last_annotate = NULL;

  ak_xest_init(xest, _s_dae_technique)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)ak_dae_assetInf(xst, technique, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_annotate)) {
      AkAnnotate *annotate;
      AkResult    ret;

      ret = ak_dae_annotate(xst, technique, &annotate);

      if (ret == AK_OK) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          technique->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_pass)) {
      AkPass * pass;
      AkResult ret;

      ret = ak_dae_fxPass(xst, technique, &pass);
      if (ret == AK_OK)
        technique->pass = pass;

    } else if (ak_xml_eqelm(xst, _s_dae_blinn)) {
      ak_blinn_phong * blinn_phong;
      AkResult ret;

      ret = ak_dae_blinn_phong(xst,
                               technique,
                               (const char *)xst->nodeName,
                               &blinn_phong);
      if (ret == AK_OK)
        technique->blinn = (AkBlinn *)blinn_phong;

    } else if (ak_xml_eqelm(xst, _s_dae_constant)) {
      AkConstantFx * constant_fx;
      AkResult ret;

      ret = ak_dae_fxConstant(xst, technique, &constant_fx);
      if (ret == AK_OK)
        technique->constant = constant_fx;

    } else if (ak_xml_eqelm(xst, _s_dae_lambert)) {
      AkLambert * lambert;
      AkResult ret;

      ret = ak_dae_fxLambert(xst, technique, &lambert);
      if (ret == AK_OK)
        technique->lambert = lambert;

    } else if (ak_xml_eqelm(xst, _s_dae_phong)) {
      ak_blinn_phong * blinn_phong;
      AkResult ret;

      ret = ak_dae_blinn_phong(xst,
                               technique,
                               (const char *)xst->nodeName,
                               &blinn_phong);
      if (ret == AK_OK)
        technique->phong = (AkPhong *)blinn_phong;
    } else if (xst->version < AK_COLLADA_VERSION_150
               && ak_xml_eqelm(xst, _s_dae_image)) {
      /* migration from 1.4 */
      ak_dae14_fxMigrateImg(xst, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          technique,
                          nodePtr,
                          &tree,
                          NULL);
      technique->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = technique;
  
  return AK_OK;
}
