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

  techc = ak_heap_calloc(xst->heap, memParent, sizeof(*techc));

  last_instanceMaterial = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_techniquec))
      break;

    /* optics -> perspective */
    if (ak_xml_eqelm(xst, _s_dae_perspective)) {
      AkPerspective * perspective;
      perspective = ak_heap_calloc(xst->heap,
                                   techc,
                                   sizeof(*perspective));
      
      do {
        if (ak_xml_beginelm(xst, _s_dae_perspective))
          break;

        if (ak_xml_eqelm(xst, _s_dae_xfov)) {
          const char *sid;

          sid = ak_xml_attr(xst, perspective, _s_dae_sid);
          perspective->xfov = glm_rad(ak_xml_valf(xst));
          
          ak_sid_seta(perspective,
                      &perspective->xfov,
                      sid);

        } else if (ak_xml_eqelm(xst, _s_dae_yfov)) {
          perspective->yfov = glm_rad(ak_xml_valf(xst));

          ak_sid_seta(perspective,
                      &perspective->yfov,
                      ak_xml_attr(xst, perspective, _s_dae_sid));

        } else if (ak_xml_eqelm(xst, _s_dae_aspect_ratio)) {
          perspective->aspectRatio = ak_xml_valf(xst);

          ak_sid_seta(perspective,
                      &perspective->aspectRatio,
                      ak_xml_attr(xst, perspective, _s_dae_sid));

        } else if (ak_xml_eqelm(xst, _s_dae_znear)) {
          perspective->znear = ak_xml_valf(xst);

          ak_sid_seta(perspective,
                      &perspective->znear,
                      ak_xml_attr(xst, perspective, _s_dae_sid));

        } else if (ak_xml_eqelm(xst, _s_dae_zfar)) {
          perspective->zfar = ak_xml_valf(xst);

          ak_sid_seta(perspective,
                      &perspective->zfar,
                      ak_xml_attr(xst, perspective, _s_dae_sid));
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
        perspective->aspectRatio = perspective->xfov / perspective->yfov;
      } else if (!perspective->yfov
                 && perspective->aspectRatio
                 && perspective->xfov) {
        perspective->yfov = perspective->xfov / perspective->aspectRatio;
      } else if (!perspective->xfov
                 && perspective->aspectRatio
                 && perspective->yfov) {
        perspective->xfov = perspective->yfov * perspective->aspectRatio;
      }
    }

    /* optics -> orthographic */
    else if (ak_xml_eqelm(xst, _s_dae_orthographic)) {
      AkOrthographic * orthographic;
      orthographic = ak_heap_calloc(xst->heap,
                                    techc,
                                    sizeof(*orthographic));

      do {
        if (ak_xml_beginelm(xst, _s_dae_orthographic))
          break;

        if (ak_xml_eqelm(xst, _s_dae_xmag)) {
          ak_basic_attrf * xmag;
          xmag = ak_heap_calloc(xst->heap,
                                orthographic,
                                sizeof(*xmag));

          xmag->sid = ak_xml_attr(xst, xmag, _s_dae_sid);
          xmag->val = ak_xml_valf(xst);

          orthographic->xmag = xmag;
        } else if (ak_xml_eqelm(xst, _s_dae_ymag)) {
          ak_basic_attrf * ymag;
          ymag = ak_heap_calloc(xst->heap,
                                orthographic,
                                sizeof(*ymag));

          ymag->sid = ak_xml_attr(xst, ymag, _s_dae_sid);
          ymag->val = ak_xml_valf(xst);

          orthographic->ymag = ymag;
        } else if (ak_xml_eqelm(xst, _s_dae_aspect_ratio)) {
          ak_basic_attrf * aspectRatio;
          aspectRatio = ak_heap_calloc(xst->heap,
                                       orthographic,
                                       sizeof(*aspectRatio));

          aspectRatio->sid = ak_xml_attr(xst, aspectRatio, _s_dae_sid);
          aspectRatio->val = ak_xml_valf(xst);

          orthographic->aspectRatio = aspectRatio;
        } else if (ak_xml_eqelm(xst, _s_dae_znear)) {
          ak_basic_attrf * znear;
          znear = ak_heap_calloc(xst->heap,
                                 orthographic,
                                 sizeof(*znear));

          znear->sid = ak_xml_attr(xst, znear, _s_dae_sid);
          znear->val = ak_xml_valf(xst);

          orthographic->znear = znear;
        } else if (ak_xml_eqelm(xst, _s_dae_zfar)) {
          ak_basic_attrf * zfar;
          zfar = ak_heap_calloc(xst->heap,
                                orthographic,
                                sizeof(*zfar));

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
                               sizeof(*ambient));

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
                                   sizeof(*directional));

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
      point = ak_heap_calloc(xst->heap, techc, sizeof(*point));

      do {
        if (ak_xml_beginelm(xst, _s_dae_point))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          ak_dae_color(xst, true, &point->color);
        } else if (ak_xml_eqelm(xst, _s_dae_constant_attenuation)) {
          ak_basic_attrd * constantAttenuation;

          constantAttenuation = ak_heap_calloc(xst->heap,
                                               point,
                                               sizeof(*constantAttenuation));

          constantAttenuation->val = ak_xml_vald(xst);
          constantAttenuation->sid = ak_xml_attr(xst,
                                                 constantAttenuation,
                                                 _s_dae_sid);

          point->constantAttenuation = constantAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_linear_attenuation)) {
          ak_basic_attrd * linearAttenuation;

          linearAttenuation = ak_heap_calloc(xst->heap,
                                             point,
                                             sizeof(*linearAttenuation));

          linearAttenuation->val = ak_xml_vald(xst);
          linearAttenuation->sid = ak_xml_attr(xst,
                                               linearAttenuation,
                                               _s_dae_sid);

          point->linearAttenuation = linearAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_quadratic_attenuation)) {
          ak_basic_attrd * quadraticAttenuation;

          quadraticAttenuation = ak_heap_calloc(xst->heap,
                                                point,
                                                sizeof(*quadraticAttenuation));

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
      spot = ak_heap_calloc(xst->heap, techc, sizeof(*spot));

      do {
        if (ak_xml_beginelm(xst, _s_dae_spot))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          ak_dae_color(xst, true, &spot->color);
        } else if (ak_xml_eqelm(xst, _s_dae_constant_attenuation)) {
          ak_basic_attrd * constantAttenuation;

          constantAttenuation = ak_heap_calloc(xst->heap,
                                               spot,
                                               sizeof(*constantAttenuation));

          constantAttenuation->val = ak_xml_vald(xst);
          constantAttenuation->sid = ak_xml_attr(xst,
                                                 constantAttenuation,
                                                 _s_dae_sid);

          spot->constantAttenuation = constantAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_linear_attenuation)) {
          ak_basic_attrd * linearAttenuation;

          linearAttenuation = ak_heap_calloc(xst->heap,
                                             spot,
                                             sizeof(*linearAttenuation));

          linearAttenuation->val = ak_xml_vald(xst);
          linearAttenuation->sid = ak_xml_attr(xst,
                                               linearAttenuation,
                                               _s_dae_sid);

          spot->linearAttenuation = linearAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_quadratic_attenuation)) {
          ak_basic_attrd * quadraticAttenuation;

          quadraticAttenuation = ak_heap_calloc(xst->heap,
                                                spot,
                                                sizeof(*quadraticAttenuation));

          quadraticAttenuation->val = ak_xml_vald(xst);
          quadraticAttenuation->sid = ak_xml_attr(xst,
                                                  quadraticAttenuation,
                                                  _s_dae_sid);

          spot->quadraticAttenuation = quadraticAttenuation;
        } else if (ak_xml_eqelm(xst, _s_dae_falloff_angle)) {
          ak_basic_attrd * falloffAngle;

          falloffAngle = ak_heap_calloc(xst->heap,
                                        spot,
                                        sizeof(*falloffAngle));

          falloffAngle->val = ak_xml_vald(xst);
          falloffAngle->sid = ak_xml_attr(xst, falloffAngle, _s_dae_sid);

          spot->falloffAngle = falloffAngle;
        } else if (ak_xml_eqelm(xst, _s_dae_falloff_exponent)) {
          ak_basic_attrd * falloffExponent;

          falloffExponent = ak_heap_calloc(xst->heap,
                                           spot,
                                           sizeof(*falloffExponent));

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
