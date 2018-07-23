/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_fx_pass.h"
#include "../core/dae_asset.h"
#include "../core/dae_annotate.h"

#include "dae_fx_states.h"
#include "dae_fx_program.h"
#include "dae_fx_evaluate.h"

AkResult _assetkit_hide
dae_fxPass(AkXmlState * __restrict xst,
           void * __restrict memParent,
           AkPass ** __restrict dest) {
  AkPass       *pass;
  AkAnnotate   *last_annotate;
  AkXmlElmState xest;

  pass = ak_heap_calloc(xst->heap,
                        memParent,
                        sizeof(*pass));

  ak_xml_readsid(xst, pass);

  last_annotate = NULL;

  ak_xest_init(xest, _s_dae_pass)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)dae_assetInf(xst, pass, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_annotate)) {
      AkAnnotate *annotate;
      AkResult    ret;

      ret = dae_annotate(xst, pass, &annotate);

      if (ret == AK_OK) {
        if (last_annotate)
          last_annotate->next = annotate;
        else
          pass->annotate = annotate;

        last_annotate = annotate;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_states)) {
      AkStates *states;
      AkResult  ret;

      ret = dae_fxState(xst, pass, &states);
      if (ret == AK_OK)
        pass->states = states;

    } else if (ak_xml_eqelm(xst, _s_dae_program)) {
      AkProgram *prog;
      AkResult   ret;

      ret = dae_fxProg(xst, pass, &prog);
      if (ret == AK_OK)
        pass->program = prog;
    } else if (ak_xml_eqelm(xst, _s_dae_evaluate)) {
      AkEvaluate * evaluate;
      AkResult ret;

      ret = dae_fxEvaluate(xst, pass, &evaluate);
      if (ret == AK_OK)
        pass->evaluate = evaluate;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          pass,
                          nodePtr,
                          &tree,
                          NULL);
      pass->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = pass;
  
  return AK_OK;
}
