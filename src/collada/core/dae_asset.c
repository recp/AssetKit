/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_asset.h"
#include "../../memory_common.h"

_assetkit_hide
void
dae_asset(xml_t * __restrict xml,
          void  * __restrict userdata) {
  DAEState      *dst;
  AkAssetInf    *inf;
  AkDoc         *doc;
  AkHeap        *heap;
  xml_attr_t    *attr;
  AkContributor *contrib;
  xml_t         *xcontrib;
  const char    *val;

  dst  = userdata;
  heap = dst->heap;
  doc  = dst->doc;
  inf  = &doc->inf->base;

  /* DAE default definitions */
  
  /* CoordSys is Y_UP */
  inf->coordSys = AK_YUP;
  
  /* Unit is 1 meter */
  inf->unit        = ak_heap_calloc(heap, inf, sizeof(*inf->unit));
  inf->unit->dist  = 1.0;
  inf->unit->name  = ak_heap_strdup(heap, inf->unit, _s_dae_meter);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_contributor)) {
      contrib  = ak_heap_calloc(heap, inf, sizeof(*contrib));
      xcontrib = xml->val;
      while (xcontrib) {
        if (xml_tag_eq(xcontrib, _s_dae_author))
          contrib->author = xml_strdup(xml, heap, inf);
        else if (xml_tag_eq(xcontrib, _s_dae_author_email))
          contrib->authorEmail = xml_strdup(xml, heap, inf);
        else if (xml_tag_eq(xcontrib, _s_dae_author_website))
          contrib->authorWebsite = xml_strdup(xml, heap, inf);
        else if (xml_tag_eq(xcontrib, _s_dae_authoring_tool))
          contrib->authoringTool = xml_strdup(xml, heap, inf);
        else if (xml_tag_eq(xcontrib, _s_dae_comments))
          contrib->comments = xml_strdup(xml, heap, inf);
        else if (xml_tag_eq(xcontrib, _s_dae_copyright))
          contrib->copyright = xml_strdup(xml, heap, inf);
        else if (xml_tag_eq(xcontrib, _s_dae_source_data))
          contrib->sourceData = xml_strdup(xml, heap, inf);
        xcontrib = xcontrib->next;
      }
    
      inf->contributor = contrib;
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
      if ((attr = xml_attr(xml, _s_dae_name)))
        inf->unit->name = xml_attr_strdup(attr, heap, inf->unit);

      if ((attr = xml_attr(xml, _s_dae_meter))) {
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
      /*  dae_extra(xst, ainf, &ainf->extra); */
    }

    xml = xml->next;
  }

  *(AkAssetInf **)ak_heap_ext_add(heap,
                                  ak__alignof(doc),
                                  AK_HEAP_NODE_FLAGS_INF) = inf;

  doc->coordSys = inf->coordSys;
  doc->unit     = inf->unit;
}
