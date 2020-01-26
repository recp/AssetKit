/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "colortex.h"
#include "../core/dae_param.h"
#include "../core/dae_color.h"
#include "dae_fx_enums.h"

AkResult _assetkit_hide
dae_colorOrTex(AkXmlState   * __restrict xst,
               void         * __restrict memParent,
               const char   * elm,
               AkColorDesc ** __restrict dest) {
  AkColorDesc  *colorOrTex;
  AkParam      *last_param;
  AkXmlElmState xest;

  colorOrTex = ak_heap_calloc(xst->heap,
                              memParent,
                              sizeof(*colorOrTex));

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
      dae_color(xst, color, true, false, color);
      colorOrTex->color = color;
    } else if (ak_xml_eqelm(xst, _s_dae_texture)) {
      AkDAETextureRef *tex;

      tex = ak_heap_calloc(xst->heap, colorOrTex, sizeof(*tex));
      ak_setypeid(tex, AKT_TEXTURE);

      tex->texture  = ak_xml_attr(xst, tex, _s_dae_texture);
      tex->texcoord = ak_xml_attr(xst, tex, _s_dae_texcoord);

      if (tex->texture)
        ak_setypeid((void *)tex->texture, AKT_TEXTURE_NAME);

      if (tex->texcoord)
        ak_setypeid((void *)tex->texcoord, AKT_TEXCOORD);
      
      rb_insert(xst->texmap, colorOrTex, tex);
    } else if (ak_xml_eqelm(xst, _s_dae_param)) {
      AkParam * param;
      AkResult   ret;

      ret = dae_param(xst, colorOrTex, &param);

      if (ret == AK_OK) {
        if (last_param)
          last_param->next = param;
        else
          colorOrTex->param = param;

        param->prev = last_param;
        last_param  = param;
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
