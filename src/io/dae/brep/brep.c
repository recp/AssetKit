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

#include "brep.h"
#include "../core/source.h"
#include "../core/vert.h"
#include "curve.h"
#include "surface.h"
#include "topology.h"

AK_HIDE
AkObject*
dae_brep(DAEState   * __restrict dst,
         xml_t      * __restrict xml,
         AkGeometry * __restrict geom) {
  AkObject     *obj;
  AkBoundryRep *brep;
  AkHeap       *heap;
  AkSource     *source;

  heap   = dst->heap;
  xml    = xml->val;

  obj    = ak_objAlloc(heap, geom, sizeof(*brep), AK_GEOMETRY_BREP, true);
  brep   = ak_objGet(obj);

  brep->geom = geom;

  while (xml) {
    if (DAE_XML_TAG_EQ(xml, curves)) {
      brep->curves = dae_curves(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, surface_curves)) {
      brep->surfaceCurves = dae_curves(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, surfaces)) {
      brep->surfaces = dae_surfaces(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, source)) {
      if ((source = dae_source(dst, xml, NULL, 0))) {
        source->next  = brep->source;
        brep->source = source;
      }
    } else if (DAE_XML_TAG_EQ(xml, vertices)) {
      brep->vertices = dae_vert(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, edges)) {
      brep->edges = dae_edges(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, wires)) {
      brep->wires = dae_wires(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, faces)) {
      brep->faces = dae_faces(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, pcurves)) {
      brep->pcurves = dae_pcurves(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, shells)) {
      brep->shells = dae_shells(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, solids)) {
      brep->solids = dae_solids(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ(xml, extra)) {
      brep->extra = tree_fromxml(heap, brep, xml);
    }
    xml = xml->next;
  }

  return obj;
}
