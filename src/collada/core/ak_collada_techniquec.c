/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_technique.h"
#include "ak_collada_color.h"
#include "../ak_collada_common.h"
#include "../fx/ak_collada_fx_material.h"
#include <cglm.h>

AkResult _assetkit_hide
ak_dae_techniquec(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  AkTechniqueCommon ** __restrict dest) {

  AkTechniqueCommon  *techc;
  AkInstanceMaterial *last_instanceMaterial;

  techc = ak_heap_calloc(xst->heap, memParent, sizeof(*techc), false);

  last_instanceMaterial = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_techniquec))
      break;

    /* optics -> perspective */
    if (ak_xml_eqelm(xst, _s_dae_perspective)) {
      AkPerspective * perspective;
      perspective = ak_heap_calloc(xst->heap,
                                   techc,
                                   sizeof(*perspective),
                                   false);
      
      do {
        if (ak_xml_beginelm(xst, _s_dae_perspective))
          break;

        if (ak_xml_eqelm(xst, _s_dae_xfov)) {
          ak_basic_attrf * xfov;
          xfov = ak_heap_calloc(xst->heap,
                                perspective,
                                sizeof(*xfov),
                                false);

          xfov->sid = ak_xml_attr(xst, xfov, _s_dae_sid);
          xfov->val = ak_xml_valf(xst);
          xfov->val = glm_rad(xfov->val);

          perspective->xfov = xfov;
        } else if (ak_xml_eqelm(xst, _s_dae_yfov)) {
          ak_basic_attrf * yfov;
          yfov = ak_heap_calloc(xst->heap,
                                perspective,
                                sizeof(*yfov),
                                false);

          yfov->sid = ak_xml_attr(xst, yfov, _s_dae_sid);
          yfov->val = ak_xml_valf(xst);
          yfov->val = glm_rad(yfov->val);

          perspective->yfov = yfov;
        } else if (ak_xml_eqelm(xst, _s_dae_aspect_ratio)) {
          ak_basic_attrf * aspectRatio;
          aspectRatio = ak_heap_calloc(xst->heap,
                                       perspective,
                                       sizeof(*aspectRatio),
                                       false);

          aspectRatio->sid = ak_xml_attr(xst, aspectRatio, _s_dae_sid);
          aspectRatio->val = ak_xml_valf(xst);

          perspective->aspectRatio = aspectRatio;
        } else if (ak_xml_eqelm(xst, _s_dae_znear)) {
          ak_basic_attrf * znear;
          znear = ak_heap_calloc(xst->heap,
                                 perspective,
                                 sizeof(*znear),
                                 false);

          znear->sid = ak_xml_attr(xst, znear, _s_dae_sid);
          znear->val = ak_xml_valf(xst);

          perspective->znear = znear;
        } else if (ak_xml_eqelm(xst, _s_dae_zfar)) {
          ak_basic_attrf * zfar;
          zfar = ak_heap_calloc(xst->heap,
                                perspective,
                                sizeof(*zfar),
                                false);

          zfar->sid = ak_xml_attr(xst, zfar, _s_dae_sid);
          zfar->val = ak_xml_valf(xst);

          perspective->zfar = zfar;
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      techc->technique = perspective;
      techc->techniqueType = AK_TECHNIQUE_COMMON_CAMERA_PERSPECTIVE;

      if (!perspective->aspectRatio
          && perspective->yfov
          && perspective->xfov) {
        ak_basic_attrf * aspectRatio;
        aspectRatio = ak_heap_calloc(xst->heap,
                                     perspective,
                                     sizeof(*aspectRatio),
                                     false);

        aspectRatio->val = perspective->xfov->val / perspective->yfov->val;
        perspective->aspectRatio = aspectRatio;
      } else if (!perspective->yfov
                 && perspective->aspectRatio
                 && perspective->xfov) {
        ak_basic_attrf * yfov;
        yfov = ak_heap_calloc(xst->heap,
                              perspective,
                              sizeof(*yfov),
                              false);

        yfov->val = perspective->xfov->val / perspective->aspectRatio->val;
        perspective->yfov = yfov;
      } else if (!perspective->xfov
                 && perspective->aspectRatio
                 && perspective->yfov) {
        ak_basic_attrf * xfov;
        xfov = ak_heap_calloc(xst->heap,
                              perspective,
                              sizeof(*xfov),
                              false);

        xfov->val = perspective->yfov->val * perspective->aspectRatio->val;
        perspective->xfov = xfov;
      }
    }

    /* optics -> orthographic */
    else if (ak_xml_eqelm(xst, _s_dae_orthographic)) {
      AkOrthographic * orthographic;
      orthographic = ak_heap_calloc(xst->heap,
                                    techc,
                                    sizeof(*orthographic),
                                    false);

      do {
        if (ak_xml_beginelm(xst, _s_dae_orthographic))
          break;

        if (ak_xml_eqelm(xst, _s_dae_xmag)) {
          ak_basic_attrf * xmag;
          xmag = ak_heap_calloc(xst->heap,
                                orthographic,
                                sizeof(*xmag),
                                false);

          xmag->sid = ak_xml_attr(xst, xmag, _s_dae_sid);
          xmag->val = ak_xml_valf(xst);

          orthographic->xmag = xmag;
        } else if (ak_xml_eqelm(xst, _s_dae_ymag)) {
          ak_basic_attrf * ymag;
          ymag = ak_heap_calloc(xst->heap,
                                orthographic,
                                sizeof(*ymag),
                                false);

          ymag->sid = ak_xml_attr(xst, ymag, _s_dae_sid);
          ymag->val = ak_xml_valf(xst);

          orthographic->ymag = ymag;
        } else if (ak_xml_eqelm(xst, _s_dae_aspect_ratio)) {
          ak_basic_attrf * aspectRatio;
          aspectRatio = ak_heap_calloc(xst->heap,
                                       orthographic,
                                       sizeof(*aspectRatio),
                                       false);

          aspectRatio->sid = ak_xml_attr(xst, aspectRatio, _s_dae_sid);
          aspectRatio->val = ak_xml_valf(xst);

          orthographic->aspectRatio = aspectRatio;
        } else if (ak_xml_eqelm(xst, _s_dae_znear)) {
          ak_basic_attrf * znear;
          znear = ak_heap_calloc(xst->heap,
                                 orthographic,
                                 sizeof(*znear),
                                 false);

          znear->sid = ak_xml_attr(xst, znear, _s_dae_sid);
          znear->val = ak_xml_valf(xst);

          orthographic->znear = znear;
        } else if (ak_xml_eqelm(xst, _s_dae_zfar)) {
          ak_basic_attrf * zfar;
          zfar = ak_heap_calloc(xst->heap,
                                orthographic,
                                sizeof(*zfar),
                                false);

          zfar->sid = ak_xml_attr(xst, zfar, _s_dae_sid);
          zfar->val = ak_xml_valf(xst);

          orthographic->zfar = zfar;
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      techc->technique = orthographic;
      techc->techniqueType = AK_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC;
    }

    /* light -> ambient */
    else if (ak_xml_eqelm(xst, _s_dae_ambient)) {
      AkAmbient * ambient;
      ambient = ak_heap_calloc(xst->heap,
                               techc,
                               sizeof(*ambient),
                               false);

      do {
        if (ak_xml_beginelm(xst, _s_dae_ambient))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          char *colorStr;

          ambient->color.sid = ak_xml_attr(xst, ambient, _s_dae_sid);
          colorStr = ak_xml_rawval(xst);

          if (colorStr) {
            ak_strtof4(&colorStr, &ambient->color.color.vec);
            xmlFree(colorStr);
          }
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      techc->technique = ambient;
      techc->techniqueType = AK_TECHNIQUE_COMMON_LIGHT_AMBIENT;
    }

    /* light -> directional */
    else if (ak_xml_eqelm(xst, _s_dae_directional)) {
      AkDirectional * directional;
      directional = ak_heap_calloc(xst->heap,
                                   techc,
                                   sizeof(*directional),
                                   false);

      do {
        if (ak_xml_beginelm(xst, _s_dae_directional))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          ak_dae_color(xst, true, &directional->color);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      techc->technique = directional;
      techc->techniqueType = AK_TECHNIQUE_COMMON_LIGHT_DIRECTIONAL;
    }

    /* light -> point */
    else if (ak_xml_eqelm(xst, _s_dae_point)) {
      AkPoint * point;
      point = ak_heap_calloc(xst->heap, techc, sizeof(*point), false);

      do {
        if (ak_xml_beginelm(xst, _s_dae_point))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          ak_dae_color(xst, true, &point->color);
        } else if (ak_xml_eqelm(xst, _s_dae_constant_attenuation)) {
          ak_basic_attrd * constantAttenuation;

          constantAttenuation = ak_heap_calloc(xst->heap,
                                               point,
                                               sizeof(*constantAttenuation),
                                               false);

          constantAttenuation->val = ak_xml_vald(xst);
          constantAttenuation->sid = ak_xml_attr(xst,
                                                 constantAttenuation,
                                                 _s_dae_sid);

          point->constantAttenuation = constantAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_linear_attenuation)) {
          ak_basic_attrd * linearAttenuation;

          linearAttenuation = ak_heap_calloc(xst->heap,
                                             point,
                                             sizeof(*linearAttenuation),
                                             false);

          linearAttenuation->val = ak_xml_vald(xst);
          linearAttenuation->sid = ak_xml_attr(xst,
                                               linearAttenuation,
                                               _s_dae_sid);

          point->linearAttenuation = linearAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_quadratic_attenuation)) {
          ak_basic_attrd * quadraticAttenuation;

          quadraticAttenuation = ak_heap_calloc(xst->heap,
                                                point,
                                                sizeof(*quadraticAttenuation),
                                                false);

          quadraticAttenuation->val = ak_xml_vald(xst);
          quadraticAttenuation->sid = ak_xml_attr(xst,
                                                  quadraticAttenuation,
                                                  _s_dae_sid);

          point->quadraticAttenuation = quadraticAttenuation;
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);
      
      techc->technique = point;
      techc->techniqueType = AK_TECHNIQUE_COMMON_LIGHT_POINT;
    }

    /* light -> spot */
    else if (ak_xml_eqelm(xst, _s_dae_point)) {
      AkSpot * spot;
      spot = ak_heap_calloc(xst->heap, techc, sizeof(*spot), false);

      do {
        if (ak_xml_beginelm(xst, _s_dae_spot))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          ak_dae_color(xst, true, &spot->color);
        } else if (ak_xml_eqelm(xst, _s_dae_constant_attenuation)) {
          ak_basic_attrd * constantAttenuation;

          constantAttenuation = ak_heap_calloc(xst->heap,
                                               spot,
                                               sizeof(*constantAttenuation),
                                               false);

          constantAttenuation->val = ak_xml_vald(xst);
          constantAttenuation->sid = ak_xml_attr(xst,
                                                 constantAttenuation,
                                                 _s_dae_sid);

          spot->constantAttenuation = constantAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_linear_attenuation)) {
          ak_basic_attrd * linearAttenuation;

          linearAttenuation = ak_heap_calloc(xst->heap,
                                             spot,
                                             sizeof(*linearAttenuation),
                                             false);

          linearAttenuation->val = ak_xml_vald(xst);
          linearAttenuation->sid = ak_xml_attr(xst,
                                               linearAttenuation,
                                               _s_dae_sid);

          spot->linearAttenuation = linearAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_quadratic_attenuation)) {
          ak_basic_attrd * quadraticAttenuation;

          quadraticAttenuation = ak_heap_calloc(xst->heap,
                                                spot,
                                                sizeof(*quadraticAttenuation),
                                                false);

          quadraticAttenuation->val = ak_xml_vald(xst);
          quadraticAttenuation->sid = ak_xml_attr(xst,
                                                  quadraticAttenuation,
                                                  _s_dae_sid);

          spot->quadraticAttenuation = quadraticAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_falloff_angle)) {
          ak_basic_attrd * falloffAngle;

          falloffAngle = ak_heap_calloc(xst->heap,
                                        spot,
                                        sizeof(*falloffAngle),
                                        false);

          falloffAngle->val = ak_xml_vald(xst);
          falloffAngle->sid = ak_xml_attr(xst, falloffAngle, _s_dae_sid);

          spot->falloffAngle = falloffAngle;
        } else if (ak_xml_eqelm(xst, _s_dae_falloff_exponent)) {
          ak_basic_attrd * falloffExponent;

          falloffExponent = ak_heap_calloc(xst->heap,
                                           spot,
                                           sizeof(*falloffExponent),
                                           false);

          falloffExponent->val = ak_xml_vald(xst);
          falloffExponent->sid = ak_xml_attr(xst, falloffExponent, _s_dae_sid);

          spot->falloffExponent = falloffExponent;
        } else {
          ak_xml_skipelm(xst);
        }
        
        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);
      
      techc->technique = spot;
      techc->techniqueType = AK_TECHNIQUE_COMMON_LIGHT_SPOT;
    } else if (ak_xml_eqelm(xst, _s_dae_instance_material)) {
      AkInstanceMaterial *instanceMaterial;
      AkResult ret;
      ret = ak_dae_fxInstanceMaterial(xst,
                                      memParent,
                                      &instanceMaterial);

      if (ret == AK_OK) {
        if (last_instanceMaterial)
          last_instanceMaterial->next = instanceMaterial;
        else {
          techc->technique = instanceMaterial;
          techc->techniqueType = AK_TECHNIQUE_COMMON_INSTANCE_MATERIAL;
        }

        last_instanceMaterial = instanceMaterial;
      }
    }
    
    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  *dest = techc;

  return AK_OK;
}
