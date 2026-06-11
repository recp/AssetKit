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

#include "spline.h"
#include "source.h"
#include "enum.h"
#include "vert.h"

AK_HIDE
AkObject*
dae_spline(DAEState   * __restrict dst,
           xml_t      * __restrict xml,
           AkGeometry * __restrict geom) {
  AkHeap   *heap;
  AkObject *obj;
  AkSpline *spline;

  heap   = dst->heap;

  obj    = ak_objAlloc(heap, geom, sizeof(*spline), AK_GEOMETRY_SPLINE, true);
  spline = ak_objGet(obj);

  spline->geom   = geom;
  spline->closed = xmla_u32(DAE_XMLA8(xml, closed), 0);

  xml = xml->val;
  
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, source)) {
      (void)dae_source(dst, xml, NULL, 0);
    } else if (DAE_XML_TAG_EQ(xml, control_vertices)) {
      spline->cverts = dae_vert(dst, xml, obj);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      spline->extra = tree_fromxml(heap, obj, xml);
    }

    xml = xml->next;
  }

  return obj;
}
