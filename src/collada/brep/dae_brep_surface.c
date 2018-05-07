/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_brep_surface.h"
#include "dae_brep_nurbs.h"
#include "dae_brep_curve.h"
#include "../../array.h"

#define k_s_dae_orient 101
#define k_s_dae_origin 102

static ak_enumpair surfaceMap[] = {
  {_s_dae_cone,          AK_SURFACE_ELEMENT_TYPE_CONE},
  {_s_dae_plane,         AK_SURFACE_ELEMENT_TYPE_PLANE},
  {_s_dae_cylinder,      AK_SURFACE_ELEMENT_TYPE_CYLINDER},
  {_s_dae_nurbs_surface, AK_SURFACE_ELEMENT_TYPE_NURBS_SURFACE},
  {_s_dae_sphere,        AK_SURFACE_ELEMENT_TYPE_SPHERE},
  {_s_dae_torus,         AK_SURFACE_ELEMENT_TYPE_TORUS},
  {_s_dae_swept_surface, AK_SURFACE_ELEMENT_TYPE_SWEPT_SURFACE},
  {_s_dae_orient,        k_s_dae_orient},
  {_s_dae_origin,        k_s_dae_origin}
};

static size_t surfaceMapLen = 0;

AkResult _assetkit_hide
ak_dae_surface(AkXmlState * __restrict xst,
               void * __restrict memParent,
               AkSurface ** __restrict dest) {
  AkSurface     *surface;
  AkFloatArrayL *last_orient;
  AkXmlElmState  xest;

  surface = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*surface));

  ak_xml_readsid(xst, surface);

  surface->name = ak_xml_attr(xst, surface, _s_dae_name);

  if (surfaceMapLen == 0) {
    surfaceMapLen = AK_ARRAY_LEN(surfaceMap);
    qsort(surfaceMap,
          surfaceMapLen,
          sizeof(surfaceMap[0]),
          ak_enumpair_cmp);
  }

  last_orient = NULL;

  ak_xest_init(xest, _s_dae_surface)

  do {
    const ak_enumpair *found;

    if (ak_xml_begin(&xest))
      break;

    found = bsearch(xst->nodeName,
                    surfaceMap,
                    surfaceMapLen,
                    sizeof(surfaceMap[0]),
                    ak_enumpair_cmp2);
    if (!found) {
      ak_xml_skipelm(xst);
      goto skip;
    }

    switch (found->val) {
      case AK_SURFACE_ELEMENT_TYPE_CONE: {
        AkObject     *obj;
        AkCone       *cone;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          surface,
                          sizeof(*cone),
                          AK_SURFACE_ELEMENT_TYPE_CONE,
                          true);

        cone = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_cone)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_radius)) {
            cone->angle = ak_xml_valf(xst);
          } else if (ak_xml_eqelm(xst, _s_dae_angle)) {
            cone->radius = ak_xml_valf(xst);
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
            cone->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        surface->surface = obj;
        break;
      }
      case AK_SURFACE_ELEMENT_TYPE_PLANE: {
        AkObject     *obj;
        AkPlane      *plane;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          surface,
                          sizeof(*plane),
                          AK_SURFACE_ELEMENT_TYPE_CONE,
                          true);

        plane = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_plane)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_radius)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, (AkFloat *)&plane->equation, 4);
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
            plane->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        surface->surface = obj;
        break;
      }
      case AK_SURFACE_ELEMENT_TYPE_CYLINDER: {
        AkObject     *obj;
        AkCylinder   *cylinder;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          surface,
                          sizeof(*cylinder),
                          AK_SURFACE_ELEMENT_TYPE_CYLINDER,
                          true);

        cylinder = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_cylinder)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_radius)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, (AkFloat *)&cylinder->radius, 2);
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
            cylinder->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        surface->surface = obj;
        break;
      }
      case AK_SURFACE_ELEMENT_TYPE_NURBS_SURFACE: {
        AkNurbsSurface *nurbsSurface;
        AkResult ret;

        ret = ak_dae_nurbs_surface(xst,
                                   surface,
                                   true,
                                   &nurbsSurface);
        if (ret == AK_OK) {
          surface->surface = ak_objFrom(nurbsSurface);
          surface->surface->type = AK_SURFACE_ELEMENT_TYPE_NURBS_SURFACE;
        }

        break;
      }
      case AK_SURFACE_ELEMENT_TYPE_SPHERE: {
        AkObject     *obj;
        AkSphere     *sphere;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          surface,
                          sizeof(*sphere),
                          AK_SURFACE_ELEMENT_TYPE_SPHERE,
                          true);

        sphere = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_sphere)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_radius)) {
            sphere->radius = ak_xml_valf(xst);
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
            sphere->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        surface->surface = obj;
        break;
      }
      case AK_SURFACE_ELEMENT_TYPE_TORUS: {
        AkObject     *obj;
        AkTorus      *torus;
        AkXmlElmState xest2;

        obj = ak_objAlloc(xst->heap,
                          surface,
                          sizeof(*torus),
                          AK_SURFACE_ELEMENT_TYPE_TORUS,
                          true);

        torus = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_torus)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_radius)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, (AkFloat *)&torus->radius, 2);
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
            torus->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        surface->surface = obj;
        break;
      }
      case AK_SURFACE_ELEMENT_TYPE_SWEPT_SURFACE: {
        AkObject       *obj;
        AkSweptSurface *sweptSurface;
        AkXmlElmState   xest2;

        obj = ak_objAlloc(xst->heap,
                          surface,
                          sizeof(*sweptSurface),
                          AK_SURFACE_ELEMENT_TYPE_SWEPT_SURFACE,
                          true);

        sweptSurface = ak_objGet(obj);

        ak_xest_init(xest2, _s_dae_swept_surface)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_curve)) {
            AkCurve *curve;
            AkResult ret;

            ret = ak_dae_curve(xst,
                               surface,
                               false,
                               &curve);
            if (ret == AK_OK)
              sweptSurface->curve = curve;
          } else if (ak_xml_eqelm(xst, _s_dae_direction)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, (AkFloat *)&sweptSurface->direction, 3);
              xmlFree(content);
            }
          } else if (ak_xml_eqelm(xst, _s_dae_origin)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, (AkFloat *)&sweptSurface->origin, 3);
              xmlFree(content);
            }
          } else if (ak_xml_eqelm(xst, _s_dae_axis)) {
            char *content;
            content = ak_xml_rawval(xst);

            if (content) {
              ak_strtof(&content, (AkFloat *)&sweptSurface->axis, 3);
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
            sweptSurface->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);
        
        surface->surface = obj;
        break;
      }
      case k_s_dae_orient: {
        char *content;
        content = ak_xml_rawval(xst);

        if (content) {
          AkFloatArrayL *orient;
          AkResult       ret;

          ret = ak_strtof_arrayL(xst->heap,
                                 surface,
                                 content,
                                 &orient);
          if (ret == AK_OK) {
            if (last_orient)
              last_orient->next = orient;
            else
              surface->orient = orient;

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
          ak_strtof(&content, surface->origin, 3);
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
  
  *dest = surface;
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_surfaces(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkSurfaces ** __restrict dest) {
  AkSurfaces   *surfaces;
  AkSurface    *last_surface;
  AkXmlElmState xest;

  surfaces = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*surfaces));

  last_surface = NULL;

  ak_xest_init(xest, _s_dae_surfaces)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_surface)) {
      AkSurface *surface;
      AkResult ret;

      ret = ak_dae_surface(xst, surfaces, &surface);
      if (ret == AK_OK) {
        if (last_surface)
          last_surface->next = surface;
        else
          surfaces->surface = surface;

        last_surface = surface;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree    *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          surfaces,
                          nodePtr,
                          &tree,
                          NULL);
      surfaces->extra = tree;

      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  *dest = surfaces;
  
  return AK_OK;
}
