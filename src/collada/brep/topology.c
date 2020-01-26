/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "topology.h"
#include "../core/enum.h"
#include "../../array.h"

AkResult _assetkit_hide
dae_edges(AkXmlState * __restrict xst,
          void * __restrict memParent,
          AkEdges ** __restrict dest) {
  AkEdges      *edges;
  AkInput      *last_input;
  AkXmlElmState xest;

  edges = ak_heap_calloc(xst->heap,
                         memParent,
                         sizeof(*edges));

  ak_xml_readid(xst, memParent);
  edges->name  = ak_xml_attr(xst, memParent, _s_dae_name);
  edges->count = ak_xml_attrui(xst, _s_dae_count);

  last_input = NULL;

  ak_xest_init(xest, _s_dae_edges)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;
      AkURL   *url;

      input = ak_heap_calloc(xst->heap,
                             edges,
                             sizeof(*input));

      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
      rb_insert(xst->inputmap, input, url);

      if (!input->semanticRaw)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = dae_enumInputSemantic(input->semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->semantic = inputSemantic;
      }

      input->offset = ak_xml_attrui(xst, _s_dae_offset);
      input->set    = ak_xml_attrui(xst, _s_dae_set);

      if (last_input)
        last_input->next = input;
      else
        edges->input = input;

      last_input = input;
    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *indices;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               edges,
                               content,
                               &indices);
        if (ret == AK_OK)
          edges->primitives = indices;

        xmlFree(content);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, edges, &edges->extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = edges;

  return AK_OK;
}

AkResult _assetkit_hide
dae_wires(AkXmlState * __restrict xst,
          void * __restrict memParent,
          AkWires ** __restrict dest) {
  AkWires      *wires;
  AkInput      *last_input;
  AkXmlElmState xest;

  wires = ak_heap_calloc(xst->heap,
                         memParent,
                         sizeof(*wires));

  ak_xml_readid(xst, memParent);
  wires->name  = ak_xml_attr(xst, memParent, _s_dae_name);
  wires->count = ak_xml_attrui(xst, _s_dae_count);

  last_input = NULL;

  ak_xest_init(xest, _s_dae_wires)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;
      AkURL   *url;

      input = ak_heap_calloc(xst->heap,
                             wires,
                             sizeof(*input));

      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
      rb_insert(xst->inputmap, input, url);

      if (!input->semanticRaw)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = dae_enumInputSemantic(input->semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->semantic = inputSemantic;
      }

      input->offset = ak_xml_attrui(xst, _s_dae_offset);
      input->set    = ak_xml_attrui(xst, _s_dae_set);

      if (last_input)
        last_input->next = input;
      else
        wires->input = input;

      last_input = input;
    } else if (ak_xml_eqelm(xst, _s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *intArray;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               wires,
                               content,
                               &intArray);
        if (ret == AK_OK)
          wires->vcount = intArray;

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *indices;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               wires,
                               content,
                               &indices);
        if (ret == AK_OK)
          wires->primitives = indices;

        xmlFree(content);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, wires, &wires->extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = wires;

  return AK_OK;
}

AkResult _assetkit_hide
dae_faces(AkXmlState * __restrict xst,
          void * __restrict memParent,
          AkFaces ** __restrict dest) {
  AkFaces      *faces;
  AkInput      *last_input;
  AkXmlElmState xest;

  faces = ak_heap_calloc(xst->heap,
                         memParent,
                         sizeof(*faces));

  ak_xml_readid(xst, memParent);
  faces->name  = ak_xml_attr(xst, memParent, _s_dae_name);
  faces->count = ak_xml_attrui(xst, _s_dae_count);

  last_input = NULL;

  ak_xest_init(xest, _s_dae_faces)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;
      AkURL   *url;

      input = ak_heap_calloc(xst->heap,
                             faces,
                             sizeof(*input));

      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
      rb_insert(xst->inputmap, input, url);

      if (!input->semanticRaw)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = dae_enumInputSemantic(input->semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->semantic = inputSemantic;
      }

      input->offset = ak_xml_attrui(xst, _s_dae_offset);
      input->set    = ak_xml_attrui(xst, _s_dae_set);

      if (last_input)
        last_input->next = input;
      else
        faces->input = input;

      last_input = input;
    } else if (ak_xml_eqelm(xst, _s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *intArray;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               faces,
                               content,
                               &intArray);
        if (ret == AK_OK)
          faces->vcount = intArray;

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *indices;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               faces,
                               content,
                               &indices);
        if (ret == AK_OK)
          faces->primitives = indices;

        xmlFree(content);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, faces, &faces->extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = faces;

  return AK_OK;
}

AkResult _assetkit_hide
dae_pcurves(AkXmlState * __restrict xst,
            void * __restrict memParent,
            AkPCurves ** __restrict dest) {
  AkPCurves    *pcurves;
  AkInput      *last_input;
  AkXmlElmState xest;

  pcurves = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*pcurves));

  ak_xml_readid(xst, memParent);
  pcurves->name  = ak_xml_attr(xst, memParent, _s_dae_name);
  pcurves->count = ak_xml_attrui(xst, _s_dae_count);

  last_input = NULL;

  ak_xest_init(xest, _s_dae_pcurves)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;
      AkURL   *url;

      input = ak_heap_calloc(xst->heap,
                             pcurves,
                             sizeof(*input));

      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
      rb_insert(xst->inputmap, input, url);

      if (!input->semanticRaw)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = dae_enumInputSemantic(input->semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->semantic = inputSemantic;
      }

      input->offset = ak_xml_attrui(xst, _s_dae_offset);
      input->set    = ak_xml_attrui(xst, _s_dae_set);

      if (last_input)
        last_input->next = input;
      else
        pcurves->input = input;

      last_input = input;
    } else if (ak_xml_eqelm(xst, _s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *intArray;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               pcurves,
                               content,
                               &intArray);
        if (ret == AK_OK)
          pcurves->vcount = intArray;

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *indices;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               pcurves,
                               content,
                               &indices);
        if (ret == AK_OK)
          pcurves->primitives = indices;

        xmlFree(content);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, pcurves, &pcurves->extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = pcurves;

  return AK_OK;
}

AkResult _assetkit_hide
dae_shells(AkXmlState * __restrict xst,
           void * __restrict memParent,
           AkShells ** __restrict dest) {
  AkShells     *shells;
  AkInput      *last_input;
  AkXmlElmState xest;

  shells = ak_heap_calloc(xst->heap,
                          memParent,
                          sizeof(*shells));

  ak_xml_readid(xst, memParent);
  shells->name  = ak_xml_attr(xst, memParent, _s_dae_name);
  shells->count = ak_xml_attrui(xst, _s_dae_count);

  last_input = NULL;

  ak_xest_init(xest, _s_dae_shells)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;
      AkURL   *url;

      input = ak_heap_calloc(xst->heap,
                             shells,
                             sizeof(*input));

      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);
      
      url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
      rb_insert(xst->inputmap, input, url);

      if (!input->semanticRaw)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = dae_enumInputSemantic(input->semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->semantic = inputSemantic;
      }

      input->offset = ak_xml_attrui(xst, _s_dae_offset);
      input->set    = ak_xml_attrui(xst, _s_dae_set);

      if (last_input)
        last_input->next = input;
      else
        shells->input = input;

      last_input = input;
    } else if (ak_xml_eqelm(xst, _s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *intArray;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               shells,
                               content,
                               &intArray);
        if (ret == AK_OK)
          shells->vcount = intArray;

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *indices;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               shells,
                               content,
                               &indices);
        if (ret == AK_OK)
          shells->primitives = indices;

        xmlFree(content);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, shells, &shells->extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = shells;

  return AK_OK;
}

AkResult _assetkit_hide
dae_solids(AkXmlState * __restrict xst,
           void * __restrict memParent,
           AkSolids ** __restrict dest) {
  AkSolids     *solids;
  AkInput      *last_input;
  AkXmlElmState xest;

  solids = ak_heap_calloc(xst->heap,
                          memParent,
                          sizeof(*solids));

  ak_xml_readid(xst, memParent);
  solids->name  = ak_xml_attr(xst, memParent, _s_dae_name);
  solids->count = ak_xml_attrui(xst, _s_dae_count);

  last_input = NULL;

  ak_xest_init(xest, _s_dae_solids)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;
      AkURL   *url;

      input = ak_heap_calloc(xst->heap,
                             solids,
                             sizeof(*input));

      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
      rb_insert(xst->inputmap, input, url);

      if (!input->semanticRaw)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = dae_enumInputSemantic(input->semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->semantic = inputSemantic;
      }

      input->offset = ak_xml_attrui(xst, _s_dae_offset);
      input->set    = ak_xml_attrui(xst, _s_dae_set);

      if (last_input)
        last_input->next = input;
      else
        solids->input = input;

      last_input = input;
    } else if (ak_xml_eqelm(xst, _s_dae_vcount)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *intArray;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               solids,
                               content,
                               &intArray);
        if (ret == AK_OK)
          solids->vcount = intArray;

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *indices;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap,
                               solids,
                               content,
                               &indices);
        if (ret == AK_OK)
          solids->primitives = indices;

        xmlFree(content);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, solids, &solids->extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = solids;

  return AK_OK;
}
