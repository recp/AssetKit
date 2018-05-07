/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_brep_curve.h"
#include "dae_brep_nurbs.h"
#include "../../array.h"

#define k_s_dae_orient 101
#define k_s_dae_origin 102

static ak_enumpair curveMap[] = {
  {_s_dae_line,      AK_CURVE_ELEMENT_TYPE_LINE},
  {_s_dae_circle,    AK_CURVE_ELEMENT_TYPE_CIRCLE},
  {_s_dae_ellipse,   AK_CURVE_ELEMENT_TYPE_ELLIPSE},
  {_s_dae_parabola,  AK_CURVE_ELEMENT_TYPE_PARABOLA},
  {_s_dae_hyperbola, AK_CURVE_ELEMENT_TYPE_HYPERBOLA},
  {_s_dae_nurbs,     AK_CURVE_ELEMENT_TYPE_NURBS},
  {_s_dae_orient,    k_s_dae_orient},
  {_s_dae_origin,    k_s_dae_origin}
};

static size_t curveMapLen = 0;

AkResult _assetkit_hide
ak_dae_curve(AkXmlState * __restrict xst,
             void * __restrict memParent,
             bool asObject,
             AkCurve ** __restrict dest) {
  AkObject       *obj;
  AkCurve        *curve;
  void           *memPtr;
  AkFloatArrayL  *last_orient;
  AkXmlElmState   xest;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*curve),
                      0,
                      true);

    curve = ak_objGet(obj);
    memPtr = obj;
  } else {
    curve = ak_heap_calloc(xst->heap, memParent, sizeof(*curve));
    memPtr = curve;
  }

  if (curveMapLen == 0) {
    curveMapLen = AK_ARRAY_LEN(curveMap);
    qsort(curveMap,
          curveMapLen,
          sizeof(curveMap[0]),
          ak_enumpair_cmp);
  }

  last_orient = NULL;

  ak_xest_init(xest, _s_dae_curve)

  do {
    const ak_enumpair *found;

    if (ak_xml_begin(&xest))
      break;

    found = bsearch(xst->nodeName,
                    curveMap,
                    curveMapLen,
                    sizeof(curveMap[0]),
                    ak_enumpair_cmp2);
    if (!found) {
      ak_xml_skipelm(xst);
      goto skip;
    }

    switch (found->val) {
      case AK_CURVE_ELEMENT_TYPE_LINE: {
        AkObject     *obj;
        AkLine       *line;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          memPtr,
                          sizeof(*line),
                          AK_CURVE_ELEMENT_TYPE_LINE,
                          true);

        line = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_line)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_origin)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, line->origin, 3);
              xmlFree(content);
            }
          } else if (ak_xml_eqelm(xst, _s_dae_direction)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, line->direction, 3);
              xmlFree(content);
            }
          } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                obj,
                                nodePtr,
                                &tree,
                                NULL);
            line->extra = tree;
            
            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }
          
          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        curve->curve = obj;

        break;
      }
      case AK_CURVE_ELEMENT_TYPE_CIRCLE: {
        AkObject     *obj;
        AkCircle     *circle;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          memPtr,
                          sizeof(*circle),
                          AK_CURVE_ELEMENT_TYPE_CIRCLE,
                          true);

        circle = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_circle)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_radius)) {
            circle->radius = ak_xml_valf(xst);
          } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                obj,
                                nodePtr,
                                &tree,
                                NULL);
            circle->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        curve->curve = obj;
        
        break;
      }
      case AK_CURVE_ELEMENT_TYPE_ELLIPSE: {
        AkObject     *obj;
        AkEllipse    *ellipse;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          memPtr,
                          sizeof(*ellipse),
                          AK_CURVE_ELEMENT_TYPE_ELLIPSE,
                          true);

        ellipse = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_ellipse)

        do {
          if (ak_xml_begin(&xest2))
              break;

          if (ak_xml_eqelm(xst, _s_dae_radius)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, (AkFloat *)&ellipse->radius, 2);
              xmlFree(content);
            }
          } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                obj,
                                nodePtr,
                                &tree,
                                NULL);
            ellipse->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        curve->curve = obj;
        
        break;
      }
      case AK_CURVE_ELEMENT_TYPE_PARABOLA: {
        AkObject     *obj;
        AkParabola   *parabola;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          memPtr,
                          sizeof(*parabola),
                          AK_CURVE_ELEMENT_TYPE_PARABOLA,
                          true);

        parabola = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_parabola)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_radius)) {
            parabola->focal = ak_xml_valf(xst);
          } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                obj,
                                nodePtr,
                                &tree,
                                NULL);
            parabola->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        curve->curve = obj;
        
        break;
      }
      case AK_CURVE_ELEMENT_TYPE_HYPERBOLA: {
        AkObject     *obj;
        AkHyperbola  *hyperbola;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          memPtr,
                          sizeof(*hyperbola),
                          AK_CURVE_ELEMENT_TYPE_HYPERBOLA,
                          true);

        hyperbola = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_hyperbola)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_radius)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, (AkFloat *)&hyperbola->radius, 2);
              xmlFree(content);
            }
          } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                obj,
                                nodePtr,
                                &tree,
                                NULL);
            hyperbola->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        curve->curve = obj;
        
        break;
      }
      case AK_CURVE_ELEMENT_TYPE_NURBS: {
        AkNurbs *nurbs;
        AkResult ret;

        ret = ak_dae_nurbs(xst, memPtr, true, &nurbs);
        if (ret == AK_OK)
          curve->curve = ak_objFrom(nurbs);

        break;
      }
      case k_s_dae_orient: {
        char *content;
        content = ak_xml_rawval(xst);

        if (content) {
          AkFloatArrayL *orient;
          AkResult       ret;

          ret = ak_strtof_arrayL(xst->heap,
                                 memPtr,
                                 content,
                                 &orient);
          if (ret == AK_OK) {
            if (last_orient)
              last_orient->next = orient;
            else
              curve->orient = orient;

            last_orient = orient;
          }
          
          xmlFree(content);
        }

        break;
      }
      case k_s_dae_origin: {
        char *content;
        content = ak_xml_rawval(xst);

        if (content) {
          ak_strtof(&content, curve->origin, 3);
          xmlFree(content);
        }
        break;
      }
      default:
        ak_xml_skipelm(xst);
        break;
    }

  skip:
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = curve;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_curves(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkCurves ** __restrict dest) {
  AkCurves     *curves;
  AkCurve      *last_curve;
  AkXmlElmState xest;

  curves = ak_heap_calloc(xst->heap, memParent, sizeof(*curves));

  last_curve = NULL;

  ak_xest_init(xest, _s_dae_curves)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_curve)) {
      AkCurve *curve;
      AkResult ret;

      ret = ak_dae_curve(xst, curves, false, &curve);
      if (ret == AK_OK) {
        if (last_curve)
          last_curve->next = curve;
        else
          curves->curve = curve;

        last_curve = curve;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree    *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          curves,
                          nodePtr,
                          &tree,
                          NULL);
      curves->extra = tree;

      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = curves;
  
  return AK_OK;
}
