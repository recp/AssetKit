/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "asset.h"
#include "../../../string_fast.h"

AK_HIDE
AkAssetInf*
dae_asset(DAEState   * __restrict dst,
          xml_t      * __restrict xml,
          void       * __restrict memp,
          AkAssetInf * __restrict inf) {
  AkHeap        *heap;
  xml_attr_t    *attr;
  AkContributor *cont;
  xml_t         *xcont;
  char          *val;

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

  if (xml) {
    xml = xml->val;
    while (xml) {
      if (DAE_XML_TAG_EQ(xml, contributor)) {
        cont  = ak_heap_calloc(heap, inf, sizeof(*cont));
        xcont = xml->val;
        while (xcont) {
          if (DAE_XML_TAG_EQ8(xcont, author))
            cont->author = xml_strdup(xcont, heap, inf);
          else if (DAE_XML_TAG_EQ(xcont, author_email))
            cont->authorEmail = xml_strdup(xcont, heap, inf);
          else if (DAE_XML_TAG_EQ(xcont, author_website))
            cont->authorWebsite = xml_strdup(xcont, heap, inf);
          else if (DAE_XML_TAG_EQ(xcont, authoring_tool))
            cont->authoringTool = xml_strdup(xcont, heap, inf);
          else if (DAE_XML_TAG_EQ8(xcont, comments))
            cont->comments = xml_strdup(xcont, heap, inf);
          else if (DAE_XML_TAG_EQ(xcont, copyright))
            cont->copyright = xml_strdup(xcont, heap, inf);
          else if (DAE_XML_TAG_EQ(xcont, source_data))
            cont->sourceData = xml_strdup(xcont, heap, inf);
          xcont = xcont->next;
        }
        
        inf->contributor = cont;
      } else if (DAE_XML_TAG_EQ8(xml, created)) {
        if ((val = xml_strdup(xml, heap, inf))) {
          memset(&xml[xml->valsize], '\0', xml->valsize);
          inf->created = ak_parse_date(val, NULL);
          ak_free(val);
        }
      } else if (DAE_XML_TAG_EQ8(xml, modified)) {
        if ((val = xml_strdup(xml, heap, inf))) {
          memset(&xml[xml->valsize], '\0', xml->valsize);
          inf->modified = ak_parse_date(val, NULL);
          ak_free(val);
        }
      } else if (DAE_XML_TAG_EQ8(xml, keywords)) {
        inf->keywords = xml_strdup(xml, heap, inf);
      } else if (DAE_XML_TAG_EQ8(xml, revision)) {
        inf->revision = xml_strdup(xml, heap, inf);
      } else if (DAE_XML_TAG_EQ8(xml, subject)) {
        inf->subject = xml_strdup(xml, heap, inf);
      } else if (DAE_XML_TAG_EQ8(xml, title)) {
        inf->title = xml_strdup(xml, heap, inf);
      } else if (DAE_XML_TAG_EQ8(xml, unit)) {
        if ((attr = DAE_XMLA8(xml, name)))
          inf->unit->name = xmla_strdup(attr, heap, inf->unit);
        
        if ((attr = DAE_XMLA8(xml, meter))) {
          /* memset((char *)attr->val +attr->valsize, '\0', 1); */
          inf->unit->dist = xmla_double(attr, 0.0);
        }
      } else if (DAE_XML_TAG_EQ8(xml, up_axis)) {
        if (xml->val) {
          if (ak_str_eq_packed_ci_fast(xml->val,
                                       xml->valsize,
                                       _s_dae_z_up_u32,
                                       _s_dae_z_up_len))
            inf->coordSys = AK_ZUP;
          else if (ak_str_eq_packed_ci_fast(xml->val,
                                            xml->valsize,
                                            _s_dae_x_up_u32,
                                            _s_dae_x_up_len))
            inf->coordSys = AK_XUP;
          else
            inf->coordSys = AK_YUP;
        }
      } else if (DAE_XML_TAG_EQ8(xml, extra)) {
        inf->extra = tree_fromxml(heap, inf, xml);
      }
      
      xml = xml->next;
    }
  } /* else -> default */

  *(AkAssetInf **)ak_heap_ext_add(heap,
                                  ak__alignof(memp),
                                  AK_HEAP_NODE_FLAGS_INF) = inf;

  return inf;
}
