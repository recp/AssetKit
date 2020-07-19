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

#include "geom.h"
#include "mesh.h"
#include "spline.h"
#include "../core/asset.h"
#include "../brep/brep.h"

AK_HIDE
void*
dae_geom(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp) {
  AkGeometry *geom;
  AkHeap     *heap;

  heap              = dst->heap;
  geom              = ak_heap_calloc(heap, memp, sizeof(*geom));
  geom->name        = xmla_strdup_by(xml, heap, _s_dae_name, geom);
  geom->materialMap = ak_map_new(ak_cmp_str);
  
  xmla_setid(xml, heap, geom);
  
  /* destroy heap with this object */
  ak_setAttachedHeap(geom, geom->materialMap->heap);
  ak_setypeid(geom, AKT_GEOMETRY);
  
  xml = xml->val;

  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, geom, NULL);
    } else if (xml_tag_eq(xml, _s_dae_mesh)
               || xml_tag_eq(xml, _s_dae_convex_mesh)) {
      geom->gdata = dae_mesh(dst, xml, geom);
    } else if (xml_tag_eq(xml, _s_dae_spline)) {
      geom->gdata = dae_spline(dst, xml, geom);
    } else if (xml_tag_eq(xml, _s_dae_brep)) {
      geom->gdata = dae_brep(dst, xml, geom);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      geom->extra = tree_fromxml(heap, geom, xml);
    }
    
    xml = xml->next;
  }
  
  return geom;
}
