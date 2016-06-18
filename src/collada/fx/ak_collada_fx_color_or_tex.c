/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_color_or_tex.h"
#include "../ak_collada_param.h"
#include "ak_collada_fx_enums.h"

AkResult _assetkit_hide
ak_dae_colorOrTex(AkHeap * __restrict heap,
                  void * __restrict memParent,
                  xmlTextReaderPtr reader,
                  const char * elm,
                  AkFxColorOrTex ** __restrict dest) {
  AkFxColorOrTex *colorOrTex;
  AkParam *last_param;

  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  colorOrTex = ak_heap_calloc(heap, memParent, sizeof(*colorOrTex), false);
  _xml_readAttrAsEnum(colorOrTex->opaque,
                      _s_dae_opaque,
                      ak_dae_fxEnumOpaque);

  last_param = NULL;

  do {
    _xml_beginElement(elm);

    if (_xml_eqElm(_s_dae_color)) {
      AkColor *color;
      char *colorStr;

      color = ak_heap_calloc(heap, colorOrTex, sizeof(*color), false);

      _xml_readAttr(color, color->sid, _s_dae_sid);
      _xml_readMutText(colorStr);

      if (colorStr) {
        ak_strtof4(&colorStr, &color->color.vec);
        colorOrTex->color = color;
        xmlFree(colorStr);
      }
    } else if (_xml_eqElm(_s_dae_texture)) {
      AkFxTexture *tex;

      tex = ak_heap_calloc(heap, colorOrTex, sizeof(*tex), false);
      _xml_readAttr(tex, tex->texture, _s_dae_texture);
      _xml_readAttr(tex, tex->texcoord, _s_dae_texcoord);

      if (!xmlTextReaderIsEmptyElement(reader)) {
        do {
          _xml_beginElement(_s_dae_texture);

          if (_xml_eqElm(_s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree   *tree;

            nodePtr = xmlTextReaderExpand(reader);
            tree = NULL;

            ak_tree_fromXmlNode(heap, tex, nodePtr, &tree, NULL);
            tex->extra = tree;

            _xml_skipElement;
          } else {
            _xml_skipElement;
          }

          /* end element */
          _xml_endElement;
        } while (nodeRet);
      }
    } else if (_xml_eqElm(_s_dae_param)) {
      AkParam * param;
      AkResult   ret;

      ret = ak_dae_param(heap,
                         colorOrTex,
                         reader,
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
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = colorOrTex;

  return AK_OK;
}
