/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_color_or_tex.h"
#include "../core/ak_collada_param.h"
#include "../core/ak_collada_color.h"
#include "ak_collada_fx_enums.h"

AkResult _assetkit_hide
ak_dae_colorOrTex(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  const char * elm,
                  AkFxColorOrTex ** __restrict dest) {
  AkFxColorOrTex *colorOrTex;
  AkParam        *last_param;
  AkXmlElmState   xest;

  colorOrTex = ak_heap_calloc(xst->heap,
                              memParent,
                              sizeof(*colorOrTex));

  colorOrTex->opaque = ak_xml_attrenum(xst,
                                       _s_dae_opaque,
                                       ak_dae_fxEnumOpaque);

  last_param = NULL;

  ak_xest_init(xest, elm)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_color)) {
      AkColor *color;
      color = ak_heap_calloc(xst->heap,
                             colorOrTex,
                             sizeof(*color));
      ak_dae_color(xst, color, true, false, color);
      colorOrTex->color = color;
    } else if (ak_xml_eqelm(xst, _s_dae_texture)) {
      AkFxTexture *tex;

      tex = ak_heap_calloc(xst->heap,
                           colorOrTex,
                           sizeof(*tex));

      tex->texture = ak_xml_attr(xst, tex, _s_dae_texture);
      tex->texcoord = ak_xml_attr(xst, tex, _s_dae_texcoord);

      if (!xmlTextReaderIsEmptyElement(xst->reader)) {
        AkXmlElmState xest2;

        ak_xest_init(xest2, _s_dae_texture)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree   *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                tex,
                                nodePtr,
                                &tree,
                                NULL);
            tex->extra = tree;

            ak_xml_skipelm(xst);
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_param)) {
      AkParam * param;
      AkResult   ret;

      ret = ak_dae_param(xst,
                         colorOrTex,
                         &param);

      if (ret == AK_OK) {
        if (last_param)
          last_param->next = param;
        else
          colorOrTex->param = param;

        last_param = param;
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = colorOrTex;

  return AK_OK;
}
