/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_color_or_tex.h"
#include "../aio_collada_common.h"
#include "../aio_collada_param.h"
#include "aio_collada_fx_enums.h"

int _assetio_hide
aio_dae_colorOrTex(xmlTextReaderPtr __restrict reader,
                   const char * elm,
                   aio_fx_color_or_tex ** __restrict dest) {
  aio_fx_color_or_tex *colorOrTex;
  aio_param *last_param;
  char *opaque;

  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  colorOrTex = aio_calloc(sizeof(*colorOrTex), 1);

  _xml_readAttr(opaque, _s_dae_opaque);
  if (opaque) {
    colorOrTex->opaque = aio_dae_fxEnumOpaque(opaque);
    aio_free(opaque);
  }

  last_param = NULL;

  do {
    _xml_beginElement(elm);

    if (_xml_eqElm(_s_dae_color)) {
      aio_color *color;
      char *colorStr;

      color = aio_calloc(sizeof(*color), 1);

      _xml_readAttr(color->sid, _s_dae_sid);
      _xml_readMutText(colorStr);

      if (colorStr) {
        aio_strtof4(&colorStr, &color->vec);
        colorOrTex->color = color;
        xmlFree(colorStr);
      }
    } else if (_xml_eqElm(_s_dae_texture)) {
      aio_fx_texture *tex;

      tex = aio_calloc(sizeof(*tex), 1);
      _xml_readAttr(tex->texture, _s_dae_texture);
      _xml_readAttr(tex->texcoord, _s_dae_texcoord);

      if (!xmlTextReaderIsEmptyElement(reader)) {
        do {
          _xml_beginElement(_s_dae_texture);

          if (_xml_eqElm(_s_dae_extra)) {
            xmlNodePtr nodePtr;
            aio_tree  *tree;

            nodePtr = xmlTextReaderExpand(reader);
            tree = NULL;

            aio_tree_fromXmlNode(nodePtr, &tree, NULL);
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
      aio_param * param;
      int         ret;

      ret = aio_dae_param(reader,
                          AIO_PARAM_TYPE_BASIC,
                          &param);

      if (ret == 0) {
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

  return 0;
}
