/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_pass.h"
#include "../ak_collada_common.h"
#include "../ak_collada_asset.h"
#include "../ak_collada_common.h"
#include "../ak_collada_annotate.h"

#include "ak_collada_fx_states.h"
#include "ak_collada_fx_program.h"
#include "ak_collada_fx_evaluate.h"

int _assetkit_hide
ak_dae_fxPass(void * __restrict memParent,
               xmlTextReaderPtr reader,
               ak_pass ** __restrict dest) {
  ak_pass      *pass;
  ak_annotate  *last_annotate;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  pass = ak_calloc(memParent, sizeof(*pass), 1);

  _xml_readAttr(pass, pass->sid, _s_dae_sid);

  last_annotate = NULL;

  do {
    _xml_beginElement(_s_dae_pass);

    if (_xml_eqElm(_s_dae_asset)) {
      ak_assetinf *assetInf;
      int ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(pass, reader, &assetInf);
      if (ret == 0)
        pass->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_annotate)) {
      ak_annotate *annotate;
      int           ret;

      ret = ak_dae_annotate(pass, reader, &annotate);

      if (ret == 0) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          pass->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (_xml_eqElm(_s_dae_states)) {
      ak_states *states;
      int         ret;

      ret = ak_dae_fxState(pass, reader, &states);
      if (ret == 0)
        pass->states = states;

    } else if (_xml_eqElm(_s_dae_program)) {
      ak_program *prog;
      int          ret;

      ret = ak_dae_fxProg(pass, reader, &prog);
      if (ret == 0)
        pass->program = prog;
    } else if (_xml_eqElm(_s_dae_evaluate)) {
      ak_evaluate * evaluate;
      int ret;

      ret = ak_dae_fxEvaluate(pass, reader, &evaluate);
      if (ret == 0)
        pass->evaluate = evaluate;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      ak_tree  *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(pass, nodePtr, &tree, NULL);
      pass->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = pass;
  
  return 0;
}
