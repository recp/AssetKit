/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "curve.h"
#include "nurb.h"
#include "../../array.h"

AkCurve* _assetkit_hide
dae_curve(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap        *heap;
  AkObject      *obj;
  AkCurve       *curve;
  char          *sval;

  heap  = dst->heap;
  curve = ak_heap_calloc(heap, memp, sizeof(*curve));

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_line)) {
      AkLine *line;
      xml_t  *xline;
      
      xline = xml->val;
      obj   = ak_objAlloc(heap, curve, sizeof(*line), AK_CURVE_LINE, true);
      line  = ak_objGet(obj);
      
      while (xline) {
        if (xml_tag_eq(xline, _s_dae_origin) && (sval = xline->val)) {
          ak_strtof(&sval, line->origin, 3);
        } else if (xml_tag_eq(xline, _s_dae_direction)
                   && (sval = xline->val)) {
          ak_strtof(&sval, line->direction, 3);
        } else if (xml_tag_eq(xline, _s_dae_extra)) {
          line->extra = tree_fromxml(heap, obj, xml);
        }
        
        xline = xline->next;
      }

      curve->curve = obj;
    } else if (xml_tag_eq(xml, _s_dae_circle)) {
      AkCircle *circ;
      xml_t    *xcirc;
      
      xcirc = xml->val;
      obj     = ak_objAlloc(heap, curve, sizeof(*circ), AK_CURVE_CIRCLE, true);
      circ  = ak_objGet(obj);
      
      while (xcirc) {
        if (xml_tag_eq(xcirc, _s_dae_radius) && xcirc->val) {
          circ->radius = xml_float(xcirc->val, 0.0f);
        } else if (xml_tag_eq(xcirc, _s_dae_extra)) {
          circ->extra = tree_fromxml(heap, obj, xml);
        }
        xcirc = xcirc->next;
      }

      curve->curve = obj;
    } else if (xml_tag_eq(xml, _s_dae_ellipse)) {
      AkEllipse *ell;
      xml_t     *xell;

      xell = xml->val;
      obj  = ak_objAlloc(heap, curve, sizeof(*ell), AK_CURVE_ELLIPSE, true);
      ell  = ak_objGet(obj);
      
      while (xell) {
        if (xml_tag_eq(xell, _s_dae_radius) && (sval = xell->val)) {
          ak_strtof(&sval, (AkFloat *)&ell->radius, 2);
        } else if (xml_tag_eq(xell, _s_dae_extra)) {
          ell->extra = tree_fromxml(heap, obj, xml);
        }
        xell = xell->next;
      }

      curve->curve = obj;
    } else if (xml_tag_eq(xml, _s_dae_parabola)) {
      AkParabola *par;
      xml_t      *xpar;

      xpar = xml->val;
      obj  = ak_objAlloc(heap, curve, sizeof(*par), AK_CURVE_PARABOLA, true);
      par  = ak_objGet(obj);

      while (xpar) {
        if (xml_tag_eq(xpar, _s_dae_focal) && xpar->val) {
          par->focal = xml_float(xpar->val, 0.0f);
        } else if (xml_tag_eq(xpar, _s_dae_extra)) {
          par->extra = tree_fromxml(heap, obj, xml);
        }
        xpar = xpar->next;
      }

      curve->curve = obj;
    } else if (xml_tag_eq(xml, _s_dae_hyperbola)) {
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
        if (xml_tag_eq(xhpar, _s_dae_radius) && (sval = xhpar->val)) {
          ak_strtof(&sval, (AkFloat *)&hpar->radius, 2);
        } else if (xml_tag_eq(xhpar, _s_dae_extra)) {
          hpar->extra = tree_fromxml(heap, obj, xml);
        }
        xhpar = xhpar->next;
      }
      
      curve->curve = obj;
    } else if (xml_tag_eq(xml, _s_dae_nurbs)) {
      curve->curve = dae_nurbs(dst, xml, curve);
    } else if (xml_tag_eq(xml, _s_dae_orient) && (sval = xml->val)) {
      AkFloatArrayL *orient;
      AkResult       ret;
      
      ret = ak_strtof_arrayL(heap, curve, sval, &orient);
      if (ret == AK_OK) {
        orient->next  = curve->orient;
        curve->orient = orient;
      }
      
    } else if (xml_tag_eq(xml, _s_dae_origin) && (sval = xml->val)) {
      ak_strtof(&sval, curve->origin, 3);
    }
    xml = xml->next;
  }

  return curve;
}

AkCurves* _assetkit_hide
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
    if (xml_tag_eq(xml, _s_dae_curve)) {
      if ((curve = dae_curve(dst, xml, curves))) {
        curve->next   = curves->curve;
        curves->curve = curve;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      curves->extra = tree_fromxml(heap, curves, xml);
    }
    xml = xml->next;
  }
  
  return curves;
}
