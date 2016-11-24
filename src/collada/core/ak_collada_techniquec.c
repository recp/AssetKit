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
          ak_xml_sid_seta(xst,
                          perspective,
                          &perspective->xfov);

          perspective->xfov = glm_rad(ak_xml_valf(xst));
        } else if (ak_xml_eqelm(xst, _s_dae_yfov)) {
          ak_xml_sid_seta(xst,
                          perspective,
                          &perspective->yfov);

          perspective->yfov = glm_rad(ak_xml_valf(xst));
        } else if (ak_xml_eqelm(xst, _s_dae_aspect_ratio)) {
          ak_xml_sid_seta(xst,
                          perspective,
                          &perspective->aspectRatio);

          perspective->aspectRatio = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_znear)) {
          ak_xml_sid_seta(xst,
                          perspective,
                          &perspective->znear);

          perspective->znear = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_zfar)) {
          ak_xml_sid_seta(xst,
                          perspective,
                          &perspective->zfar);

          perspective->zfar = ak_xml_valf(xst);
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
          ak_xml_sid_seta(xst,
                          orthographic,
                          &orthographic->xmag);

          orthographic->xmag = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_ymag)) {
          ak_xml_sid_seta(xst,
                          orthographic,
                          &orthographic->ymag);

          orthographic->ymag = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_aspect_ratio)) {
          ak_xml_sid_seta(xst,
                          orthographic,
                          &orthographic->aspectRatio);

          orthographic->aspectRatio = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_znear)) {
          ak_xml_sid_seta(xst,
                          orthographic,
                          &orthographic->znear);

          orthographic->znear = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_zfar)) {
          ak_xml_sid_seta(xst,
                          orthographic,
                          &orthographic->zfar);

          orthographic->zfar = ak_xml_valf(xst);
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

          ak_xml_readsid(xst, ambient);
          colorStr = ak_xml_rawcval(xst);

          if (colorStr)
            ak_strtof4(&colorStr, &ambient->vec);
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
          char *colorStr;

          ak_xml_readsid(xst, directional);
          colorStr = ak_xml_rawcval(xst);

          if (colorStr)
            ak_strtof4(&colorStr, &directional->vec);
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
          ak_xml_sid_seta(xst,
                          point,
                          &point->color);

          ak_dae_color(xst, false, &point->color);
        } else if (ak_xml_eqelm(xst, _s_dae_constant_attenuation)) {
          ak_xml_sid_seta(xst,
                          point,
                          &point->constantAttenuation);

          point->constantAttenuation = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_linear_attenuation)) {
          ak_xml_sid_seta(xst,
                          point,
                          &point->linearAttenuation);

          point->linearAttenuation = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_quadratic_attenuation)) {
          ak_xml_sid_seta(xst,
                          point,
                          &point->quadraticAttenuation);

          point->quadraticAttenuation = ak_xml_valf(xst);
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
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->color);

          ak_dae_color(xst, false, &spot->color);
        } else if (ak_xml_eqelm(xst, _s_dae_constant_attenuation)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->constantAttenuation);

          spot->constantAttenuation = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_linear_attenuation)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->linearAttenuation);

          spot->linearAttenuation = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_quadratic_attenuation)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->quadraticAttenuation);

          spot->quadraticAttenuation = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_falloff_angle)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->falloffAngle);

          spot->falloffAngle = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_falloff_exponent)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->falloffExponent);

          spot->falloffExponent = ak_xml_valf(xst);
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
