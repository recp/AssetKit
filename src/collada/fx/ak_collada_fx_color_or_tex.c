/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_color_or_tex.h"
#include "../core/ak_collada_param.h"
#include "ak_collada_fx_enums.h"

AkResult _assetkit_hide
ak_dae_colorOrTex(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  const char * elm,
                  AkFxColorOrTex ** __restrict dest) {
  AkFxColorOrTex *colorOrTex;
  AkParam        *last_param;

  colorOrTex = ak_heap_calloc(xst->heap,
                              memParent,
                              sizeof(*colorOrTex),
                              false);
  _xml_readAttrAsEnum(colorOrTex->opaque,
                      _s_dae_opaque,
                      ak_dae_fxEnumOpaque);

  last_param = NULL;

  do {
    if (ak_xml_beginelm(xst, elm))
      break;

    if (_xml_eqElm(_s_dae_color)) {
      AkColor *color;
      char *colorStr;

      color = ak_heap_calloc(xst->heap,
                             colorOrTex,
                             sizeof(*color),
                             false);

      _xml_readAttr(color, color->sid, _s_dae_sid);
      colorStr = ak_xml_rawval(xst);

      if (colorStr) {
        ak_strtof4(&colorStr, &color->color.vec);
        colorOrTex->color = color;
        xmlFree(colorStr);
      }
    } else if (_xml_eqElm(_s_dae_texture)) {
      AkFxTexture *tex;

      tex = ak_heap_calloc(xst->heap,
                           colorOrTex,
                           sizeof(*tex),
                           false);
      _xml_readAttr(tex, tex->texture, _s_dae_texture);
      _xml_readAttr(tex, tex->texcoord, _s_dae_texcoord);

      if (!xmlTextReaderIsEmptyElement(xst->reader)) {
        do {
          if (ak_xml_beginelm(xst, _s_dae_texture))
            break;

          if (_xml_eqElm(_s_dae_extra)) {
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

            ak_xml_skipelm(xst);;
          } else {
            ak_xml_skipelm(xst);;
          }

          /* end element */
          ak_xml_endelm(xst);;
        } while (xst->nodeRet);
      }
    } else if (_xml_eqElm(_s_dae_param)) {
      AkParam * param;
      AkResult   ret;

      ret = ak_dae_param(xst,
                         colorOrTex,
                         AK_PARAM_TYPE_BASIC,
                         &param);

      if (ret == AK_OK) {
        if (last_param)
          last_param->next = param;
        else
          colorOrTex->param = param;

        last_param = param;
      }
    } else {
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = colorOrTex;

  return AK_OK;
}
