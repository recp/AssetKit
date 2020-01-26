/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "vert.h"
#include "enum.h"

AkResult _assetkit_hide
dae_vert(AkXmlState * __restrict xst,
         void * __restrict memParent,
         AkVertices ** __restrict dest) {
  AkVertices   *vertices;
  AkInput      *last_input;
  AkXmlElmState xest;

  vertices = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*vertices));

  ak_xml_readid(xst, vertices);
  vertices->name = ak_xml_attr(xst, vertices, _s_dae_name);

  last_input = NULL;

  ak_xest_init(xest, _s_dae_vertices)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(xst->heap, vertices, sizeof(*input));
      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      if (!input->semanticRaw) {
        ak_free(input);
      } else {
        AkURL *url;
        input->semantic = dae_enumInputSemantic(input->semanticRaw);

        url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
        rb_insert(xst->inputmap, input, url);

        if (last_input)
          last_input->next = input;
        else
          vertices->input = input;

        last_input = input;

        vertices->inputCount++;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, vertices, &vertices->extra);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = vertices;

  return AK_OK;
}
