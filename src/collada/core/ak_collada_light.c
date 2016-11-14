/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_light.h"
#include "ak_collada_asset.h"
#include "ak_collada_technique.h"

AkResult _assetkit_hide
ak_dae_light(AkXmlState * __restrict xst,
             void * __restrict memParent,
             void ** __restrict dest) {
  AkLight           *light;
  AkTechnique       *last_tq;
  AkTechniqueCommon *last_tc;

  light = ak_heap_calloc(xst->heap, memParent, sizeof(*light));

  ak_xml_readid(xst, light);
  light->name = ak_xml_attr(xst, light, _s_dae_name);

  last_tq = light->technique;
  last_tc = light->techniqueCommon;

  do {
    if (ak_xml_beginelm(xst, _s_dae_light))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, light, &assetInf);
      if (ret == AK_OK)
        light->inf = assetInf;

    } else if (ak_xml_eqelm(xst, _s_dae_techniquec)) {
      AkTechniqueCommon *tc;
      AkResult ret;

      tc = NULL;
      ret = ak_dae_techniquec(xst, light, &tc);
      if (ret == AK_OK) {
        if (last_tc)
          last_tc->next = tc;
        else
          light->techniqueCommon = tc;

        last_tc = tc;
      }

    } else if (ak_xml_eqelm(xst, _s_dae_technique)) {
      AkTechnique *tq;
      AkResult ret;

      tq = NULL;
      ret = ak_dae_technique(xst, light, &tq);
      if (ret == AK_OK) {
        if (last_tq)
          last_tq->next = tq;
        else
          light->technique = tq;

        last_tq = tq;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          light,
                          nodePtr,
                          &tree,
                          NULL);
      light->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  *dest = light;

  return AK_OK;
}
