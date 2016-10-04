/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_pass.h"
#include "../core/ak_collada_asset.h"
#include "../core/ak_collada_annotate.h"

#include "ak_collada_fx_states.h"
#include "ak_collada_fx_program.h"
#include "ak_collada_fx_evaluate.h"

AkResult _assetkit_hide
ak_dae_fxPass(AkHeap * __restrict heap,
              void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkPass ** __restrict dest) {
  AkPass        *pass;
  AkAnnotate    *last_annotate;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  pass = ak_heap_calloc(heap, memParent, sizeof(*pass), false);

  _xml_readAttr(pass, pass->sid, _s_dae_sid);

  last_annotate = NULL;

  do {
    _xml_beginElement(_s_dae_pass);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(heap, pass, reader, &assetInf);
      if (ret == AK_OK)
        pass->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_annotate)) {
      AkAnnotate *annotate;
      AkResult    ret;

      ret = ak_dae_annotate(heap, pass, reader, &annotate);

      if (ret == AK_OK) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          pass->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (_xml_eqElm(_s_dae_states)) {
      AkStates *states;
      AkResult  ret;

      ret = ak_dae_fxState(heap, pass, reader, &states);
      if (ret == AK_OK)
        pass->states = states;

    } else if (_xml_eqElm(_s_dae_program)) {
      AkProgram *prog;
      AkResult   ret;

      ret = ak_dae_fxProg(heap, pass, reader, &prog);
      if (ret == AK_OK)
        pass->program = prog;
    } else if (_xml_eqElm(_s_dae_evaluate)) {
      AkEvaluate * evaluate;
      AkResult ret;

      ret = ak_dae_fxEvaluate(heap, pass, reader, &evaluate);
      if (ret == AK_OK)
        pass->evaluate = evaluate;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, pass, nodePtr, &tree, NULL);
      pass->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = pass;
  
  return AK_OK;
}
