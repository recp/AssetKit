/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_technique.h"

#include "../aio_collada_common.h"
#include "../aio_collada_asset.h"
#include "../aio_collada_annotate.h"

#include "aio_collada_fx_blinn_phong.h"
#include "aio_collada_fx_constant.h"
#include "aio_collada_fx_lambert.h"
#include "aio_collada_fx_pass.h"

int _assetio_hide
aio_dae_techniqueFx(void * __restrict memParent,
                    xmlTextReaderPtr reader,
                    aio_technique_fx ** __restrict dest) {
  aio_technique_fx *technique;
  aio_annotate     *last_annotate;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  technique = aio_calloc(memParent, sizeof(*technique), 1);

  _xml_readAttr(technique, technique->id, _s_dae_id);
  _xml_readAttr(technique, technique->sid, _s_dae_sid);

  last_annotate = NULL;

  do {
    _xml_beginElement(_s_dae_technique);

    if (_xml_eqElm(_s_dae_asset)) {
      aio_assetinf *assetInf;
      int ret;

      assetInf = NULL;
      ret = aio_dae_assetInf(technique, reader, &assetInf);
      if (ret == 0)
        technique->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_annotate)) {
      aio_annotate *annotate;
      int           ret;

      ret = aio_dae_annotate(technique, reader, &annotate);

      if (ret == 0) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          technique->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (_xml_eqElm(_s_dae_pass)) {
      aio_pass * pass;
      int        ret;

      ret = aio_dae_fxPass(technique, reader, &pass);
      if (ret == 0)
        technique->pass = pass;

    } else if (_xml_eqElm(_s_dae_blinn)) {
      aio_blinn_phong * blinn_phong;
      int ret;

      ret = aio_dae_blinn_phong(technique,
                                reader,
                                (const char *)nodeName,
                                &blinn_phong);
      if (ret == 0)
        technique->blinn = (aio_blinn *)blinn_phong;

    } else if (_xml_eqElm(_s_dae_constant)) {
      aio_constant_fx * constant_fx;
      int ret;

      ret = aio_dae_fxConstant(technique, reader, &constant_fx);
      if (ret == 0)
        technique->constant = constant_fx;

    } else if (_xml_eqElm(_s_dae_lambert)) {
      aio_lambert * lambert;
      int ret;

      ret = aio_dae_fxLambert(technique, reader, &lambert);
      if (ret == 0)
        technique->lambert = lambert;

    } else if (_xml_eqElm(_s_dae_phong)) {
      aio_blinn_phong * blinn_phong;
      int ret;

      ret = aio_dae_blinn_phong(technique,
                                reader,
                                (const char *)nodeName,
                                &blinn_phong);
      if (ret == 0)
        technique->phong = (aio_phong *)blinn_phong;

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      aio_tree  *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      aio_tree_fromXmlNode(technique, nodePtr, &tree, NULL);
      technique->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = technique;
  
  return 0;

}
