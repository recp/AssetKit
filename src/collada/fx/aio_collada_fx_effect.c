/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_effect.h"

#include "../aio_collada_common.h"
#include "../aio_collada_asset.h"
#include "../aio_collada_technique.h"
#include "../aio_collada_annotate.h"
#include "../aio_collada_param.h"

#include "aio_collada_fx_profile.h"

int _assetio_hide
aio_dae_effect(xmlTextReaderPtr __restrict reader,
               aio_effect ** __restrict  dest) {
  aio_effect   *effect;
  aio_annotate *last_annotate;
  aio_newparam *last_newparam;
  aio_profile  *last_profile;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  effect = aio_calloc(sizeof(*effect), 1);

  last_annotate = NULL;
  last_newparam = NULL;
  last_profile  = NULL;
  
  _xml_readAttr(effect->id, _s_dae_id);
  _xml_readAttr(effect->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_effect);

    if (_xml_eqElm(_s_dae_asset)) {
      aio_assetinf *assetInf;
      int ret;

      assetInf = NULL;
      ret = aio_dae_assetInf(reader, &assetInf);
      if (ret == 0)
        effect->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_annotate)) {
      aio_annotate *annotate;
      int           ret;

      ret = aio_dae_annotate(reader, &annotate);

      if (ret == 0) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          effect->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (_xml_eqElm(_s_dae_newparam)) {
      aio_newparam *newparam;
      int            ret;

      ret = aio_dae_newparam(reader, &newparam);

      if (ret == 0) {
        if (last_newparam)
          last_newparam->next = newparam;
        else
          effect->newparam = newparam;

        last_newparam = newparam;
      }
    } else if (_xml_eqElm(_s_dae_prfl_common)
               || _xml_eqElm(_s_dae_prfl_glsl)
               || _xml_eqElm(_s_dae_prfl_gles2)
               || _xml_eqElm(_s_dae_prfl_gles)
               || _xml_eqElm(_s_dae_prfl_cg)
               || _xml_eqElm(_s_dae_prfl_bridge)) {
      aio_profile *profile;
      int          ret;

      ret = aio_dae_profile(reader, &profile);

      if (ret == 0) {
        if (last_profile)
          last_profile->next = profile;
        else
          effect->profile = profile;

        last_profile = profile;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      aio_tree  *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      aio_tree_fromXmlNode(nodePtr, &tree, NULL);
      effect->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = effect;

  return 0;
}
