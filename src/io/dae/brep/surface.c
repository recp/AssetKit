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

#include "surface.h"
#include "nurb.h"
#include "curve.h"
#include "../../../array.h"

AK_HIDE
AkSurface*
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

  surf->name = DAE_XMLA_STRDUP8(xml, heap, name, surf);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, cone)) {
      AkCone *cone;
      xml_t  *xcone;
      
      obj  = ak_objAlloc(heap, surf, sizeof(*cone), AK_SURFACE_CONE, true);
      cone = ak_objGet(obj);
      
      xcone = xml->val;
      while (xcone) {
        if (DAE_XML_TAG_EQ(xcone, radius) && xcone->val) {
          cone->radius = xml_float(xcone->val, 0.0f);
        } else if (DAE_XML_TAG_EQ(xcone, angle) && xcone->val) {
          cone->angle = xml_float(xcone->val, 0.0f);
        } else if (DAE_XML_TAG_EQ(xcone, extra)) {
          cone->extra = tree_fromxml(heap, obj, xcone);
        }
        xcone = xcone->next;
      }

      surf->surface = obj;
    } else if (DAE_XML_TAG_EQ(xml, plane)) {
      AkPlane *plane;
      xml_t   *xplane;
      
      obj   = ak_objAlloc(heap, surf, sizeof(*plane), AK_SURFACE_PLANE, true);
      plane = ak_objGet(obj);
      
      xplane = xml->val;
      while (xplane) {
        if (DAE_XML_TAG_EQ(xplane, equation) && (sval = xmls(xplane))) {
          xml_strtof_fast(sval, (AkFloat *)&plane->equation, 4);
        } else if (DAE_XML_TAG_EQ(xplane, extra)) {
          plane->extra = tree_fromxml(heap, obj, xplane);
        }
        xplane = xplane->next;
      }
      
      surf->surface = obj;
    } else if (DAE_XML_TAG_EQ(xml, cylinder)) {
      AkCylinder *clyn;
      xml_t      *xclyn;
      
      obj  = ak_objAlloc(heap, surf, sizeof(*clyn), AK_SURFACE_CYLINDER, true);
      clyn = ak_objGet(obj);
      
      xclyn = xml->val;
      while (xclyn) {
        if (DAE_XML_TAG_EQ(xclyn, radius) && (sval = xmls(xclyn))) {
          xml_strtof_fast(sval, (AkFloat *)&clyn->radius, 2);
        } else if (DAE_XML_TAG_EQ(xclyn, extra)) {
          clyn->extra = tree_fromxml(heap, obj, xclyn);
        }
        xclyn = xclyn->next;
      }
      
      surf->surface = obj;
    } else if (DAE_XML_TAG_EQ(xml, nurbs_surface)) {
      surf->surface = dae_nurbs_surface(dst, xml, surf);
    } else if (DAE_XML_TAG_EQ(xml, sphere)) {
      AkSphere *sphere;
      xml_t    *xsphere;
      
      obj = ak_objAlloc(heap, surf, sizeof(*sphere), AK_SURFACE_SPHERE, true);
      sphere = ak_objGet(obj);
      
      xsphere = xml->val;
      while (xsphere) {
        if (DAE_XML_TAG_EQ(xsphere, radius) && xsphere->val) {
          sphere->radius = xml_float(xsphere->val, 0.0f);
        } else if (DAE_XML_TAG_EQ(xsphere, extra)) {
          sphere->extra = tree_fromxml(heap, obj, xsphere);
        }
        xsphere = xsphere->next;
      }

      surf->surface = obj;
    } else if (DAE_XML_TAG_EQ(xml, torus)) {
      AkTorus *torus;
      xml_t   *xtorus;

      obj   = ak_objAlloc(heap, surf, sizeof(*xtorus), AK_SURFACE_TORUS, true);
      torus = ak_objGet(obj);

      xtorus = xml->val;
      while (xtorus) {
        if (DAE_XML_TAG_EQ(xtorus, radius) && (sval = xmls(xtorus))) {
          xml_strtof_fast(sval, (AkFloat *)&torus->radius, 2);
        } else if (DAE_XML_TAG_EQ(xtorus, extra)) {
          torus->extra = tree_fromxml(heap, obj, xtorus);
        }
        xtorus = xtorus->next;
      }

      surf->surface = obj;
    } else if (DAE_XML_TAG_EQ(xml, swept_surface)) {
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
        if (DAE_XML_TAG_EQ(xswept, curve)) {
          sweptSurface->curve = dae_curve(dst, xswept, obj);
        } else if (DAE_XML_TAG_EQ(xswept, direction) && (sval = xmls(xml))) {
          xml_strtof_fast(sval, (AkFloat *)&sweptSurface->direction, 3);
        } else if (DAE_XML_TAG_EQ(xswept, origin) && (sval = xmls(xml))) {
          xml_strtof_fast(sval, (AkFloat *)&sweptSurface->origin, 3);
        } else if (DAE_XML_TAG_EQ(xswept, axis) && (sval = xmls(xml))) {
          xml_strtof_fast(sval, (AkFloat *)&sweptSurface->axis, 3);
        } else if (DAE_XML_TAG_EQ(xswept, extra)) {
           sweptSurface->extra = tree_fromxml(heap, obj, xswept);
        }
        xswept = xswept->next;
      }
    } else if (DAE_XML_TAG_EQ(xml, orient) && (sval =xmls(xml))) {
      AkFloatArrayL *orient;
      AkResult       ret;
      
      ret = xml_strtof_arrayL(heap, surf, sval, &orient);
      if (ret == AK_OK) {
        orient->next = surf->orient;
        surf->orient = orient;
      }
    } else if (DAE_XML_TAG_EQ(xml, origin) && (sval = xmls(xml))) {
      xml_strtof_fast(sval, surf->origin, 3);
    }
    xml = xml->next;
  }

  return surf;
}

AK_HIDE
AkSurfaces*
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
    if (DAE_XML_TAG_EQ(xml, surface)) {
      if ((surface = dae_surface(dst, xml, memp))) {
        surface->next     = surfaces->surface;
        surfaces->surface = surface;
      }
    } else if (DAE_XML_TAG_EQ(xml, extra)) {
      surfaces->extra = tree_fromxml(heap, surfaces, xml);
    }
    xml = xml->next;
  }

  return surfaces;
}
