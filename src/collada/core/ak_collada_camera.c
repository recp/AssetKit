/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_camera.h"
#include "ak_collada_asset.h"
#include "ak_collada_technique.h"

AkResult _assetkit_hide
ak_dae_camera(AkXmlState * __restrict xst,
              void * __restrict memParent,
              void ** __restrict  dest) {
  AkCamera     *camera;
  AkXmlElmState xest;

  camera = ak_heap_calloc(xst->heap, memParent, sizeof(*camera));

  ak_xml_readid(xst, camera);
  camera->name = ak_xml_attr(xst, camera, _s_dae_name);

  ak_xest_init(xest, _s_dae_camera)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, camera, &assetInf);
      if (ret == AK_OK)
        camera->inf = assetInf;

    } else if (ak_xml_eqelm(xst, _s_dae_optics)) {
      AkOptics          *optics;
      AkTechnique       *last_tq;
      AkTechniqueCommon *last_tc;
      AkXmlElmState      xest2;

      optics = ak_heap_calloc(xst->heap, camera, sizeof(*optics));

      last_tq = optics->technique;
      last_tc = optics->techniqueCommon;

      ak_xest_init(xest2, _s_dae_optics)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_techniquec)) {
          AkTechniqueCommon *tc;
          AkResult ret;

          tc = NULL;
          ret = ak_dae_techniquec(xst, optics, &tc);
          if (ret == AK_OK) {
            if (last_tc)
              last_tc->next = tc;
            else
              optics->techniqueCommon = tc;

            last_tc = tc;
          }

        } else if (ak_xml_eqelm(xst, _s_dae_technique)) {
          AkTechnique *tq;
          AkResult ret;

          tq = NULL;
          ret = ak_dae_technique(xst, optics, &tq);
          if (ret == AK_OK) {
            if (last_tq)
              last_tq->next = tq;
            else
              optics->technique = tq;

            last_tq = tq;
          }
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      camera->optics = optics;
    } else if (ak_xml_eqelm(xst, _s_dae_imager)) {
      AkImager     *imager;
      AkTechnique  *last_tq;
      AkXmlElmState xest2;

      imager  = ak_heap_calloc(xst->heap, camera, sizeof(*imager));
      last_tq = imager->technique;

      ak_xest_init(xest2, _s_dae_imager)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_technique)) {
          AkTechnique *tq;
          AkResult ret;

          tq = NULL;
          ret = ak_dae_technique(xst, imager, &tq);
          if (ret == AK_OK) {
            if (last_tq)
              last_tq->next = tq;
            else
              imager->technique = tq;

            last_tq = tq;
          }
        } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(xst->reader);
          tree = NULL;

          ak_tree_fromXmlNode(xst->heap,
                              imager,
                              nodePtr,
                              &tree,
                              NULL);
          imager->extra = tree;

          ak_xml_skipelm(xst);
        } else {
          ak_xml_skipelm(xst);
        }
        
        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      camera->imager = imager;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          camera,
                          nodePtr,
                          &tree,
                          NULL);
      camera->extra = tree;

      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = camera;

  return AK_OK;
}
