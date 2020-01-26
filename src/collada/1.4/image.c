/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "image.h"
#include "../core/asset.h"
#include "../fx/enum.h"

void _assetkit_hide
dae14_fxMigrateImg(AkXmlState * __restrict xst,
                   void       * __restrict memParent) {
  AkImage      *img;
  AkInitFrom   *initFrom;
  const char   *format;
  AkLibItem    *lib;
  AkXmlElmState xest;

  if (memParent)
    lib = memParent;
  else
    lib = ak_libImageFirstOrCreat(xst->doc);

  img      = ak_heap_calloc(xst->heap, lib, sizeof(*img));
  initFrom = ak_heap_alloc(xst->heap,
                           img,
                           sizeof(*img->initFrom));
  img->initFrom = initFrom;

  ak_xml_readid(xst, img);
  img->name = ak_xml_attr(xst, img, _s_dae_name);

  format = ak_xml_attr(xst, img->initFrom, _s_dae_format);
  initFrom->mipsGenerate = false; /* 1.4's default, 1.5's is true */
  initFrom->depth = ak_xml_attrui(xst, _s_dae_depth);

  ak_xest_init(xest, _s_dae_image)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)dae_assetInf(xst, img, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_data)) {
      AkHexData *hex;
      hex = ak_heap_calloc(xst->heap,
                           initFrom,
                           sizeof(*hex));

      hex->format = format;
      ak_heap_setpm((void *)format, hex);

      if (hex->format) {
        hex->hexval = ak_xml_val(xst, hex);
        img->initFrom->hex = hex;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_init_from)) {
      img->initFrom->ref = ak_xml_val(xst, initFrom);
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, img, &img->extra);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  if (!memParent)
    ak_libInsertInto(lib, img, -1, offsetof(AkImage, next));
}
