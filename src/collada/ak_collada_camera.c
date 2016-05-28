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
ak_dae_camera(AkHeap * __restrict heap,
              void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkCamera ** __restrict  dest) {
  AkCamera    *camera;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  camera = ak_heap_calloc(heap, memParent, sizeof(*camera), false);

  _xml_readAttr(camera, camera->id, _s_dae_id);
  _xml_readAttr(camera, camera->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_camera);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(heap, camera, reader, &assetInf);
      if (ret == AK_OK)
        camera->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_optics)) {
      AkOptics           *optics;
      AkTechnique        *last_tq;
      AkTechniqueCommon *last_tc;

      optics = ak_heap_calloc(heap, camera, sizeof(*optics), false);

      last_tq = optics->technique;
      last_tc = optics->techniqueCommon;

      do {
        _xml_beginElement(_s_dae_optics);

        if (_xml_eqElm(_s_dae_techniquec)) {
          AkTechniqueCommon *tc;
          AkResult ret;

          tc = NULL;
          ret = ak_dae_techniquec(heap, optics, reader, &tc);
          if (ret == AK_OK) {
            if (last_tc)
              last_tc->next = tc;
            else
              optics->techniqueCommon = tc;

            last_tc = tc;
          }

        } else if (_xml_eqElm(_s_dae_technique)) {
          AkTechnique *tq;
          AkResult ret;

          tq = NULL;
          ret = ak_dae_technique(heap, optics, reader, &tq);
          if (ret == AK_OK) {
            if (last_tq)
              last_tq->next = tq;
            else
              optics->technique = tq;

            last_tq = tq;
          }
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      camera->optics = optics;
    } else if (_xml_eqElm(_s_dae_imager)) {
      AkImager    *imager;
      AkTechnique *last_tq;

      imager  = ak_heap_calloc(heap, camera, sizeof(*imager), false);
      last_tq = imager->technique;

      do {
        _xml_beginElement(_s_dae_imager);

        if (_xml_eqElm(_s_dae_technique)) {
          AkTechnique *tq;
          AkResult ret;

          tq = NULL;
          ret = ak_dae_technique(heap, imager, reader, &tq);
          if (ret == AK_OK) {
            if (last_tq)
              last_tq->next = tq;
            else
              imager->technique = tq;

            last_tq = tq;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(heap, imager, nodePtr, &tree, NULL);
          imager->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);

      camera->imager = imager;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, camera, nodePtr, &tree, NULL);
      camera->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = camera;

  return AK_OK;
}
