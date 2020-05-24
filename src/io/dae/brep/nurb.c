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

#include "nurb.h"
#include "../core/vert.h"
#include "../core/source.h"
#include "../core/enum.h"

AkObject* _assetkit_hide
dae_nurbs(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap   *heap;
  AkObject *obj;
  AkNurbs  *nurbs;
  AkSource *source;

  heap  = dst->heap;
  obj   = ak_objAlloc(heap, memp, sizeof(*nurbs), 0, true);
  nurbs = ak_objGet(obj);

  nurbs->degree = xmla_u32(xmla(xml, _s_dae_degree), 0);
  nurbs->closed = xmla_u32(xmla(xml, _s_dae_closed), 0);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_source)) {
      if ((source = dae_source(dst, xml, NULL, 0))) {
        source->next  = nurbs->source;
        nurbs->source = source;
      }
    } else if (xml_tag_eq(xml, _s_dae_control_vertices)) {
      nurbs->cverts = dae_vert(dst, xml, obj);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      nurbs->extra = tree_fromxml(heap, nurbs, xml);
    }
    xml = xml->next;
  }
  
  return obj;
}

AkObject* _assetkit_hide
dae_nurbs_surface(DAEState * __restrict dst,
                  xml_t    * __restrict xml,
                  void     * __restrict memp) {
  AkHeap         *heap;
  AkObject       *obj;
  AkNurbsSurface *nurbsSurface;
  AkSource       *source;

  heap  = dst->heap;
  obj   = ak_objAlloc(heap, memp, sizeof(*nurbsSurface), 0, true);
  nurbsSurface = ak_objGet(obj);

  nurbsSurface->degree_u = xmla_u32(xmla(xml, _s_dae_degree_u), 0);
  nurbsSurface->degree_v = xmla_u32(xmla(xml, _s_dae_degree_v), 0);
  nurbsSurface->closed_u = xmla_u32(xmla(xml, _s_dae_closed_u), 0);
  nurbsSurface->closed_v = xmla_u32(xmla(xml, _s_dae_closed_v), 0);
  
  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_source)) {
      if ((source = dae_source(dst, xml, NULL, 0))) {
        source->next         = nurbsSurface->source;
        nurbsSurface->source = source;
      }
    } else if (xml_tag_eq(xml, _s_dae_control_vertices)) {
      nurbsSurface->cverts = dae_vert(dst, xml, obj);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      nurbsSurface->extra = tree_fromxml(heap, nurbsSurface, xml);
    }
    xml = xml->next;
  }
  
  return obj;
}
