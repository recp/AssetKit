/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "surface.h"
#include "nurb.h"
#include "curve.h"
#include "../../array.h"

AkSurface* _assetkit_hide
dae_surface(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp) {
  AkHeap      *heap;
  AkSurface   *surf;
  AkObject    *obj;
  const xml_t *sval;

  heap = dst->heap;
  surf = ak_heap_calloc(heap, memp, sizeof(*surf));

  sid_set(xml, heap, surf);

  surf->name = xmla_strdup_by(xml, heap, _s_dae_name, surf);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_cone)) {
      AkCone *cone;
      xml_t  *xcone;
      
      obj  = ak_objAlloc(heap, surf, sizeof(*cone), AK_SURFACE_CONE, true);
      cone = ak_objGet(obj);
      
      xcone = xml->val;
      while (xcone) {
        if (xml_tag_eq(xcone, _s_dae_radius) && xcone->val) {
          cone->radius = xml_float(xcone->val, 0.0f);
        } else if (xml_tag_eq(xcone, _s_dae_angle) && xcone->val) {
          cone->angle = xml_float(xcone->val, 0.0f);
        } else if (xml_tag_eq(xcone, _s_dae_extra)) {
          cone->extra = tree_fromxml(heap, obj, xcone);
        }
        xcone = xcone->next;
      }

      surf->surface = obj;
    } else if (xml_tag_eq(xml, _s_dae_plane)) {
      AkPlane *plane;
      xml_t   *xplane;
      
      obj   = ak_objAlloc(heap, surf, sizeof(*plane), AK_SURFACE_PLANE, true);
      plane = ak_objGet(obj);
      
      xplane = xml->val;
      while (xplane) {
        if (xml_tag_eq(xplane, _s_dae_equation) && (sval = xmls(xplane))) {
          xml_strtof_fast(sval, (AkFloat *)&plane->equation, 4);
        } else if (xml_tag_eq(xplane, _s_dae_extra)) {
          plane->extra = tree_fromxml(heap, obj, xplane);
        }
        xplane = xplane->next;
      }
      
      surf->surface = obj;
    } else if (xml_tag_eq(xml, _s_dae_cylinder)) {
      AkCylinder *clyn;
      xml_t      *xclyn;
      
      obj  = ak_objAlloc(heap, surf, sizeof(*clyn), AK_SURFACE_CYLINDER, true);
      clyn = ak_objGet(obj);
      
      xclyn = xml->val;
      while (xclyn) {
        if (xml_tag_eq(xclyn, _s_dae_radius) && (sval = xmls(xclyn))) {
          xml_strtof_fast(sval, (AkFloat *)&clyn->radius, 2);
        } else if (xml_tag_eq(xclyn, _s_dae_extra)) {
          clyn->extra = tree_fromxml(heap, obj, xclyn);
        }
        xclyn = xclyn->next;
      }
      
      surf->surface = obj;
    } else if (xml_tag_eq(xml, _s_dae_nurbs_surface)) {
      surf->surface = dae_nurbs_surface(dst, xml, surf);
    } else if (xml_tag_eq(xml, _s_dae_sphere)) {
      AkSphere *sphere;
      xml_t    *xsphere;
      
      obj = ak_objAlloc(heap, surf, sizeof(*sphere), AK_SURFACE_SPHERE, true);
      sphere = ak_objGet(obj);
      
      xsphere = xml->val;
      while (xsphere) {
        if (xml_tag_eq(xsphere, _s_dae_radius) && xsphere->val) {
          sphere->radius = xml_float(xsphere->val, 0.0f);
        } else if (xml_tag_eq(xsphere, _s_dae_extra)) {
          sphere->extra = tree_fromxml(heap, obj, xsphere);
        }
        xsphere = xsphere->next;
      }

      surf->surface = obj;
    } else if (xml_tag_eq(xml, _s_dae_torus)) {
      AkTorus *torus;
      xml_t   *xtorus;

      obj   = ak_objAlloc(heap, surf, sizeof(*xtorus), AK_SURFACE_TORUS, true);
      torus = ak_objGet(obj);

      xtorus = xml->val;
      while (xtorus) {
        if (xml_tag_eq(xtorus, _s_dae_radius) && (sval = xmls(xtorus))) {
          xml_strtof_fast(sval, (AkFloat *)&torus->radius, 2);
        } else if (xml_tag_eq(xtorus, _s_dae_extra)) {
          torus->extra = tree_fromxml(heap, obj, xtorus);
        }
        xtorus = xtorus->next;
      }

      surf->surface = obj;
    } else if (xml_tag_eq(xml, _s_dae_swept_surface)) {
      AkObject       *obj;
      AkSweptSurface *sweptSurface;
      xml_t          *xswept;
      
      obj = ak_objAlloc(heap,
                        surf,
                        sizeof(*sweptSurface),
                        AK_SURFACE_SWEPT_SURFACE,
                        true);
      
      sweptSurface = ak_objGet(obj);
      
      xswept = xml->val;
      while (xswept) {
        if (xml_tag_eq(xswept, _s_dae_curve)) {
          sweptSurface->curve = dae_curve(dst, xswept, obj);
        } else if (xml_tag_eq(xswept, _s_dae_direction) && (sval = xmls(xml))) {
          xml_strtof_fast(sval, (AkFloat *)&sweptSurface->direction, 3);
        } else if (xml_tag_eq(xswept, _s_dae_origin) && (sval = xmls(xml))) {
          xml_strtof_fast(sval, (AkFloat *)&sweptSurface->origin, 3);
        } else if (xml_tag_eq(xswept, _s_dae_axis) && (sval = xmls(xml))) {
          xml_strtof_fast(sval, (AkFloat *)&sweptSurface->axis, 3);
        } else if (xml_tag_eq(xswept, _s_dae_extra)) {
           sweptSurface->extra = tree_fromxml(heap, obj, xswept);
        }
        xswept = xswept->next;
      }
    } else if (xml_tag_eq(xml, _s_dae_orient) && (sval =xmls(xml))) {
      AkFloatArrayL *orient;
      AkResult       ret;
      
      ret = xml_strtof_arrayL(heap, surf, sval, &orient);
      if (ret == AK_OK) {
        orient->next = surf->orient;
        surf->orient = orient;
      }
    } else if (xml_tag_eq(xml, _s_dae_origin) && (sval = xmls(xml))) {
      xml_strtof_fast(sval, surf->origin, 3);
    }
    xml = xml->next;
  }

  return surf;
}

AkSurfaces* _assetkit_hide
dae_surfaces(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp) {
  AkHeap     *heap;
  AkSurfaces *surfaces;
  AkSurface  *surface;

  heap     = dst->heap;
  surfaces = ak_heap_calloc(heap, memp, sizeof(*surfaces));

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_surface)) {
      if ((surface = dae_surface(dst, xml, memp))) {
        surface->next     = surfaces->surface;
        surfaces->surface = surface;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      surfaces->extra = tree_fromxml(heap, surfaces, xml);
    }
    xml = xml->next;
  }

  return surfaces;
}
