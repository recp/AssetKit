/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_camera.h"
#include "ak_collada_common.h"
#include "ak_collada_asset.h"
#include "ak_collada_technique.h"

int _assetkit_hide
ak_dae_camera(void * __restrict memParent,
               xmlTextReaderPtr reader,
               ak_camera ** __restrict  dest) {
  ak_camera    *camera;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  camera = ak_calloc(memParent, sizeof(*camera), 1);

  _xml_readAttr(camera, camera->id, _s_dae_id);
  _xml_readAttr(camera, camera->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_camera);

    if (_xml_eqElm(_s_dae_asset)) {
      ak_assetinf *assetInf;
      int ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(camera, reader, &assetInf);
      if (ret == 0)
        camera->inf = assetInf;

    } else if (_xml_eqElm(_s_dae_optics)) {
      ak_optics           *optics;
      ak_technique        *last_tq;
      ak_technique_common *last_tc;

      optics = ak_calloc(camera, sizeof(*optics), 1);

      last_tq = optics->technique;
      last_tc = optics->technique_common;

      do {
        _xml_beginElement(_s_dae_optics);

        if (_xml_eqElm(_s_dae_techniquec)) {
          ak_technique_common *tc;
          int                   ret;

          tc = NULL;
          ret = ak_dae_techniquec(optics, reader, &tc);
          if (ret == 0) {
            if (last_tc)
              last_tc->next = tc;
            else
              optics->technique_common = tc;

            last_tc = tc;
          }

        } else if (_xml_eqElm(_s_dae_technique)) {
          ak_technique *tq;
          int            ret;

          tq = NULL;
          ret = ak_dae_technique(optics, reader, &tq);
          if (ret == 0) {
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
      ak_imager    *imager;
      ak_technique *last_tq;

      imager  = ak_calloc(camera, sizeof(*imager), 1);
      last_tq = imager->technique;

      do {
        _xml_beginElement(_s_dae_imager);

        if (_xml_eqElm(_s_dae_technique)) {
          ak_technique *tq;
          int            ret;

          tq = NULL;
          ret = ak_dae_technique(imager, reader, &tq);
          if (ret == 0) {
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

          ak_tree_fromXmlNode(imager, nodePtr, &tree, NULL);
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

      ak_tree_fromXmlNode(camera, nodePtr, &tree, NULL);
      camera->extra = tree;

      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = camera;

  return 0;
}
