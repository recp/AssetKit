/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "colortex.h"
#include "../core/param.h"
#include "../core/color.h"
#include "../core/enum.h"

AkColorDesc* _assetkit_hide
dae_colorOrTex(DAEState * __restrict dst,
               xml_t    * __restrict xml,
               void     * __restrict memp) {
  AkHeap      *heap;
  AkColorDesc *clr;

  heap = dst->heap;
  clr  = ak_heap_calloc(heap, memp, sizeof(*clr));

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_color)) {
      clr->color = ak_heap_calloc(heap, clr, sizeof(*clr->color));
      dae_color(xml, clr->color, true, false, clr->color);
    } else if (xml_tag_eq(xml, _s_dae_texture)) {
      AkDAETextureRef *tex;
      
      tex = ak_heap_calloc(heap, clr, sizeof(*tex));
      ak_setypeid(tex, AKT_TEXTURE);
      
      tex->texture  = xmla_strdup(xmla(xml, _s_dae_texture),  heap, tex);
      tex->texcoord = xmla_strdup(xmla(xml, _s_dae_texcoord), heap, tex);
      
      if (tex->texture)
        ak_setypeid((void *)tex->texture, AKT_TEXTURE_NAME);
      
      if (tex->texcoord)
        ak_setypeid((void *)tex->texcoord, AKT_TEXCOORD);
      
      rb_insert(dst->texmap, clr, tex);
    } else if (xml_tag_eq(xml, _s_dae_param)) {
      AkParam *param;
      
      if ((param = dae_param(dst, xml, clr))) {
        if (clr->param)
          clr->param->prev = param;

        param->next = clr->param;
        clr->param  = param;
      }
    }
    xml = xml->next;
  }

  return clr;
}
