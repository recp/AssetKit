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

#include "curve.h"
#include "nurb.h"
#include "../../../array.h"

AK_HIDE
AkCurve*
dae_curve(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap      *heap;
  AkObject    *obj;
  AkCurve     *curve;
  const xml_t *sval;

  heap  = dst->heap;
  curve = ak_heap_calloc(heap, memp, sizeof(*curve));

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, line)) {
      AkLine *line;
      xml_t  *xline;
      
      xline = xml->val;
      obj   = ak_objAlloc(heap, curve, sizeof(*line), AK_CURVE_LINE, true);
      line  = ak_objGet(obj);
      
      while (xline) {
        if (DAE_XML_TAG_EQ(xline, origin) && (sval = xmls(xline))) {
          xml_strtof_fast(sval, line->origin, 3);
        } else if (DAE_XML_TAG_EQ(xline, direction) && (sval = xmls(xline))) {
          xml_strtof_fast(sval, line->direction, 3);
        } else if (DAE_XML_TAG_EQ(xline, extra)) {
          line->extra = tree_fromxml(heap, obj, xml);
        }
        
        xline = xline->next;
      }

      curve->curve = obj;
    } else if (DAE_XML_TAG_EQ(xml, circle)) {
      AkCircle *circ;
      xml_t    *xcirc;
      
      xcirc = xml->val;
      obj     = ak_objAlloc(heap, curve, sizeof(*circ), AK_CURVE_CIRCLE, true);
      circ  = ak_objGet(obj);
      
      while (xcirc) {
        if (DAE_XML_TAG_EQ(xcirc, radius) && xcirc->val) {
          circ->radius = xml_float(xcirc->val, 0.0f);
        } else if (DAE_XML_TAG_EQ(xcirc, extra)) {
          circ->extra = tree_fromxml(heap, obj, xml);
        }
        xcirc = xcirc->next;
      }

      curve->curve = obj;
    } else if (DAE_XML_TAG_EQ(xml, ellipse)) {
      AkEllipse *ell;
      xml_t     *xell;

      xell = xml->val;
      obj  = ak_objAlloc(heap, curve, sizeof(*ell), AK_CURVE_ELLIPSE, true);
      ell  = ak_objGet(obj);
      
      while (xell) {
        if (DAE_XML_TAG_EQ(xell, radius) && (sval = xmls(xell))) {
          xml_strtof_fast(sval, (AkFloat *)&ell->radius, 2);
        } else if (DAE_XML_TAG_EQ(xell, extra)) {
          ell->extra = tree_fromxml(heap, obj, xml);
        }
        xell = xell->next;
      }

      curve->curve = obj;
    } else if (DAE_XML_TAG_EQ(xml, parabola)) {
      AkParabola *par;
      xml_t      *xpar;

      xpar = xml->val;
      obj  = ak_objAlloc(heap, curve, sizeof(*par), AK_CURVE_PARABOLA, true);
      par  = ak_objGet(obj);

      while (xpar) {
        if (DAE_XML_TAG_EQ(xpar, focal) && xpar->val) {
          par->focal = xml_float(xpar->val, 0.0f);
        } else if (DAE_XML_TAG_EQ(xpar, extra)) {
          par->extra = tree_fromxml(heap, obj, xml);
        }
        xpar = xpar->next;
      }

      curve->curve = obj;
    } else if (DAE_XML_TAG_EQ(xml, hyperbola)) {
      AkHyperbola *hpar;
      xml_t       *xhpar;
      
      xhpar = xml->val;
      obj   = ak_objAlloc(heap,
                          curve,
                          sizeof(*hpar),
                          AK_CURVE_HYPERBOLA,
                          true);
      hpar  = ak_objGet(obj);
      
      while (xhpar) {
        if (DAE_XML_TAG_EQ(xhpar, radius) && (sval = xmls(xhpar))) {
          xml_strtof_fast(sval, (AkFloat *)&hpar->radius, 2);
        } else if (DAE_XML_TAG_EQ(xhpar, extra)) {
          hpar->extra = tree_fromxml(heap, obj, xml);
        }
        xhpar = xhpar->next;
      }
      
      curve->curve = obj;
    } else if (DAE_XML_TAG_EQ(xml, nurbs)) {
      curve->curve = dae_nurbs(dst, xml, curve);
    } else if (DAE_XML_TAG_EQ(xml, orient) && (sval = xmls(xml))) {
      AkFloatArrayL *orient;
      AkResult       ret;
      
      ret = xml_strtof_arrayL(heap, curve, sval, &orient);
      if (ret == AK_OK) {
        orient->next  = curve->orient;
        curve->orient = orient;
      }
    } else if (DAE_XML_TAG_EQ(xml, origin) && (sval = xmls(xml))) {
      xml_strtof_fast(sval, curve->origin, 3);
    }
    xml = xml->next;
  }

  return curve;
}

AK_HIDE
AkCurves*
dae_curves(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp) {
  AkHeap   *heap;
  AkCurves *curves;
  AkCurve  *curve;

  heap   = dst->heap;
  curves = ak_heap_calloc(heap, memp, sizeof(*curves));

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, curve)) {
      if ((curve = dae_curve(dst, xml, curves))) {
        curve->next   = curves->curve;
        curves->curve = curve;
      }
    } else if (DAE_XML_TAG_EQ(xml, extra)) {
      curves->extra = tree_fromxml(heap, curves, xml);
    }
    xml = xml->next;
  }
  
  return curves;
}
