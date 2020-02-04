/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "spline.h"
#include "source.h"
#include "enum.h"
#include "vert.h"

AkObject* _assetkit_hide
dae_spline(DAEState   * __restrict dst,
           xml_t      * __restrict xml,
           AkGeometry * __restrict geom) {
  AkObject *obj;
  AkSpline *spline;
  AkDoc    *doc;
  AkHeap   *heap;

  heap   = dst->heap;
  doc    = dst->doc;
  xml    = xml->val;

  obj    = ak_objAlloc(heap, geom, sizeof(*spline), AK_GEOMETRY_SPLINE, true);
  spline = ak_objGet(obj);

  spline->geom   = geom;
  spline->closed = xmla_uint32(xml_attr(xml, _s_dae_closed), 0);
  
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_source)) {
      (void)dae_source(dst, xml, NULL, 0);
    } else if (xml_tag_eq(xml, _s_dae_vertices)) {
      spline->cverts = dae_vert(dst, xml, obj);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      spline->extra = tree_fromxml(heap, obj, xml);
    }

    xml = xml->next;
  }

  return obj;
}
