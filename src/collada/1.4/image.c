/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "image.h"
#include "../core/asset.h"
#include "../core/enum.h"

void _assetkit_hide
dae14_fxMigrateImg(DAEState * __restrict dst,
                   xml_t    * __restrict xml,
                   void     * __restrict memp) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkImage    *img;
  AkInitFrom *initFrom;
  const char *format;
  AkLibrary  *lib;

  heap = dst->heap;
  doc  = dst->doc;

  if (memp)
    lib = memp;
  else
    lib = ak_libImageFirstOrCreat(doc);

  img           = ak_heap_calloc(heap, lib, sizeof(*img));
  initFrom      = ak_heap_alloc(heap, img, sizeof(*img->initFrom));
  img->initFrom = initFrom;

  xmla_setid(xml, heap, img);
  
  img->name = xmla_strdup_by(xml, heap, _s_dae_name, img);

  format = xmla_strdup_by(xml, heap, _s_dae_format, img->initFrom);
  initFrom->mipsGenerate = false; /* 1.4's default, 1.5's is true */
  initFrom->depth        = xmla_u32(xmla(xml, _s_dae_depth), 0);
  
  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, img, NULL);
    } else if (xml_tag_eq(xml, _s_dae_data)) {
      AkHexData *hex;
      hex = ak_heap_calloc(heap, initFrom, sizeof(*hex));
      
      hex->format = format;
      ak_heap_setpm((void *)format, hex);
      
      if (hex->format) {
        hex->hexval        = xml_strdup(xml, heap, hex);
        img->initFrom->hex = hex;
      }
    } else if (xml_tag_eq(xml, _s_dae_init_from)) {
      img->initFrom->ref = xml_strdup(xml, heap, initFrom);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      img->extra = tree_fromxml(heap, img, xml);
    }
    xml = xml->next;
  }

  if (!memp)
    ak_libInsertInto(lib, img, -1, offsetof(AkImage, next));
}
