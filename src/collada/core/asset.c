/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "asset.h"

_assetkit_hide
AkAssetInf*
dae_asset(DAEState   * __restrict dst,
          xml_t      * __restrict xml,
          void       * __restrict memp,
          AkAssetInf * __restrict inf) {
  AkHeap        *heap;
  xml_attr_t    *attr;
  AkContributor *cont;
  xml_t         *xcont;
  const char    *val;

  heap = dst->heap;

  if (!inf)
    inf = ak_heap_alloc(heap, memp, sizeof(*inf));

  /* DAE default definitions */
  
  /* CoordSys is Y_UP */
  inf->coordSys    = AK_YUP;

  /* Unit is 1 meter */
  inf->unit        = ak_heap_calloc(heap, inf, sizeof(*inf->unit));
  inf->unit->dist  = 1.0;
  inf->unit->name  = ak_heap_strdup(heap, inf->unit, _s_dae_meter);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_contributor)) {
      cont  = ak_heap_calloc(heap, inf, sizeof(*cont));
      xcont = xml->val;
      while (xcont) {
        if (xml_tag_eq(xcont, _s_dae_author))
          cont->author = xml_strdup(xcont, heap, inf);
        else if (xml_tag_eq(xcont, _s_dae_author_email))
          cont->authorEmail = xml_strdup(xcont, heap, inf);
        else if (xml_tag_eq(xcont, _s_dae_author_website))
          cont->authorWebsite = xml_strdup(xcont, heap, inf);
        else if (xml_tag_eq(xcont, _s_dae_authoring_tool))
          cont->authoringTool = xml_strdup(xcont, heap, inf);
        else if (xml_tag_eq(xcont, _s_dae_comments))
          cont->comments = xml_strdup(xcont, heap, inf);
        else if (xml_tag_eq(xcont, _s_dae_copyright))
          cont->copyright = xml_strdup(xcont, heap, inf);
        else if (xml_tag_eq(xcont, _s_dae_source_data))
          cont->sourceData = xml_strdup(xcont, heap, inf);
        xcont = xcont->next;
      }
    
      inf->contributor = cont;
    } else if (xml_tag_eq(xml, _s_dae_created)) {
      if ((val = xml_string(xml))) {
        memset(&xml[xml->valsize], '\0', xml->valsize);
        inf->created = ak_parse_date(val, NULL);
      }
    } else if (xml_tag_eq(xml, _s_dae_modified)) {
      if ((val = xml_string(xml))) {
        memset(&xml[xml->valsize], '\0', xml->valsize);
        inf->modified = ak_parse_date(val, NULL);
      }
    } else if (xml_tag_eq(xml, _s_dae_keywords)) {
      inf->keywords = xml_strdup(xml, heap, inf);
    } else if (xml_tag_eq(xml, _s_dae_revision)) {
      inf->revision = xml_strdup(xml, heap, inf);
    } else if (xml_tag_eq(xml, _s_dae_subject)) {
      inf->subject = xml_strdup(xml, heap, inf);
    } else if (xml_tag_eq(xml, _s_dae_title)) {
      inf->title = xml_strdup(xml, heap, inf);
    } else if (xml_tag_eq(xml, _s_dae_unit)) {
      if ((attr = xmla(xml, _s_dae_name)))
        inf->unit->name = xmla_strdup(attr, heap, inf->unit);

      if ((attr = xmla(xml, _s_dae_meter))) {
        memset((char *)attr->val +attr->valsize, '\0', 1);
        inf->unit->dist = xmla_double(attr, 0.0);
      }
    } else if (xml_tag_eq(xml, _s_dae_axis)) {
      if ((val = xml->val)) {
        if (strcasecmp(val, _s_dae_z_up) == 0)
          inf->coordSys = AK_ZUP;
        else if (strcasecmp(val, _s_dae_x_up) == 0)
          inf->coordSys = AK_XUP;
        else
          inf->coordSys = AK_YUP;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      inf->extra = tree_fromxml(heap, inf, xml);
    }

    xml = xml->next;
  }

  *(AkAssetInf **)ak_heap_ext_add(heap,
                                  ak__alignof(memp),
                                  AK_HEAP_NODE_FLAGS_INF) = inf;
  
  return inf;
}
