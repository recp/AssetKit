/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_technique.h"
#include "ak_collada_common.h"
#include "ak_collada_color.h"
#include "fx/ak_collada_fx_material.h"

AkResult _assetkit_hide
ak_dae_techniquec(void * __restrict memParent,
                   xmlTextReaderPtr reader,
                   AkTechniqueCommon ** __restrict dest) {

  AkTechniqueCommon *techc;
  AkInstanceMaterial  *last_instanceMaterial;

  const xmlChar * nodeName;
  int             nodeType;
  int             nodeRet;

  techc = ak_calloc(memParent, sizeof(*techc), 1);

  last_instanceMaterial = NULL;

  do {
    _xml_beginElement(_s_dae_techniquec);

    /* optics -> perspective */
    if (_xml_eqElm(_s_dae_perspective)) {
      AkPerspective * perspective;
      perspective = ak_calloc(techc, sizeof(*perspective), 1);
      
      do {
        _xml_beginElement(_s_dae_perspective);

        if (_xml_eqElm(_s_dae_xfov)) {
          ak_basic_attrd * xfov;
          xfov = ak_calloc(perspective, sizeof(*xfov), 1);

          _xml_readAttr(xfov, xfov->sid, _s_dae_sid);
          _xml_readTextUsingFn(xfov->val, strtod, NULL);

          perspective->xfov = xfov;
        } else if (_xml_eqElm(_s_dae_yfov)) {
          ak_basic_attrd * yfov;
          yfov = ak_calloc(perspective, sizeof(*yfov), 1);

          _xml_readAttr(yfov, yfov->sid, _s_dae_sid);
          _xml_readTextUsingFn(yfov->val, strtod, NULL);

          perspective->yfov = yfov;
        } else if (_xml_eqElm(_s_dae_aspect_ratio)) {
          ak_basic_attrd * aspect_ratio;
          aspect_ratio = ak_calloc(perspective, sizeof(*aspect_ratio), 1);

          _xml_readAttr(aspect_ratio, aspect_ratio->sid, _s_dae_sid);
          _xml_readTextUsingFn(aspect_ratio->val, strtod, NULL);

          perspective->aspect_ratio = aspect_ratio;
        } else if (_xml_eqElm(_s_dae_znear)) {
          ak_basic_attrd * znear;
          znear = ak_calloc(perspective, sizeof(*znear), 1);

          _xml_readAttr(znear, znear->sid, _s_dae_sid);
          _xml_readTextUsingFn(znear->val, strtod, NULL);

          perspective->znear = znear;
        } else if (_xml_eqElm(_s_dae_zfar)) {
          ak_basic_attrd * zfar;
          zfar = ak_calloc(perspective, sizeof(*zfar), 1);

          _xml_readAttr(zfar, zfar->sid, _s_dae_sid);
          _xml_readTextUsingFn(zfar->val, strtod, NULL);

          perspective->zfar = zfar;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = perspective;
      techc->technique_type = AK_TECHNIQUE_COMMON_CAMERA_PERSPECTIVE;
    }

    /* optics -> orthographic */
    else if (_xml_eqElm(_s_dae_orthographic)) {
      AkOrthographic * orthographic;
      orthographic = ak_calloc(techc, sizeof(*orthographic), 1);

      do {
        _xml_beginElement(_s_dae_orthographic);

        if (_xml_eqElm(_s_dae_xmag)) {
          ak_basic_attrd * xmag;
          xmag = ak_calloc(orthographic, sizeof(*xmag), 1);

          _xml_readAttr(xmag, xmag->sid, _s_dae_sid);
          _xml_readTextUsingFn(xmag->val, strtod, NULL);

          orthographic->xmag = xmag;
        } else if (_xml_eqElm(_s_dae_ymag)) {
          ak_basic_attrd * ymag;
          ymag = ak_calloc(orthographic, sizeof(*ymag), 1);

          _xml_readAttr(ymag, ymag->sid, _s_dae_sid);
          _xml_readTextUsingFn(ymag->val, strtod, NULL);

          orthographic->ymag = ymag;
        } else if (_xml_eqElm(_s_dae_aspect_ratio)) {
          ak_basic_attrd * aspect_ratio;
          aspect_ratio = ak_calloc(orthographic, sizeof(*aspect_ratio), 1);

          _xml_readAttr(aspect_ratio, aspect_ratio->sid, _s_dae_sid);
          _xml_readTextUsingFn(aspect_ratio->val, strtod, NULL);

          orthographic->aspect_ratio = aspect_ratio;
        } else if (_xml_eqElm(_s_dae_znear)) {
          ak_basic_attrd * znear;
          znear = ak_calloc(orthographic, sizeof(*znear), 1);

          _xml_readAttr(znear, znear->sid, _s_dae_sid);
          _xml_readTextUsingFn(znear->val, strtod, NULL);

          orthographic->znear = znear;
        } else if (_xml_eqElm(_s_dae_zfar)) {
          ak_basic_attrd * zfar;
          zfar = ak_calloc(orthographic, sizeof(*zfar), 1);

          _xml_readAttr(zfar, zfar->sid, _s_dae_sid);
          _xml_readTextUsingFn(zfar->val, strtod, NULL);

          orthographic->zfar = zfar;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = orthographic;
      techc->technique_type = AK_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC;
    }

    /* light -> ambient */
    else if (_xml_eqElm(_s_dae_ambient)) {
      AkAmbient * ambient;
      ambient = ak_calloc(techc, sizeof(*ambient), 1);

      do {
        _xml_beginElement(_s_dae_ambient);

        if (_xml_eqElm(_s_dae_color)) {
          char *colorStr;

          _xml_readAttr(ambient, ambient->color.sid, _s_dae_sid);
          _xml_readMutText(colorStr);

          if (colorStr) {
            ak_strtof4(&colorStr, &ambient->color.vec);
            xmlFree(colorStr);
          }
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = ambient;
      techc->technique_type = AK_TECHNIQUE_COMMON_LIGHT_AMBIENT;
    }

    /* light -> directional */
    else if (_xml_eqElm(_s_dae_directional)) {
      AkDirectional * directional;
      directional = ak_calloc(techc, sizeof(*directional), 1);

      do {
        _xml_beginElement(_s_dae_directional);

        if (_xml_eqElm(_s_dae_color)) {
          ak_dae_color(reader, true, &directional->color);
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = directional;
      techc->technique_type = AK_TECHNIQUE_COMMON_LIGHT_DIRECTIONAL;
    }

    /* light -> point */
    else if (_xml_eqElm(_s_dae_point)) {
      AkPoint * point;
      point = ak_calloc(techc, sizeof(*point), 1);

      do {
        _xml_beginElement(_s_dae_point);

        if (_xml_eqElm(_s_dae_color)) {
          ak_dae_color(reader, true, &point->color);
        } else if (_xml_eqElm(_s_dae_constant_attenuation)) {
          ak_basic_attrd * constant_attenuation;

          constant_attenuation = ak_calloc(point,
                                            sizeof(*constant_attenuation),
                                            1);

          _xml_readTextUsingFn(constant_attenuation->val, strtod, NULL);
          _xml_readAttr(constant_attenuation,
                        constant_attenuation->sid,
                        _s_dae_sid);

          point->constant_attenuation = constant_attenuation;
        } else if (_xml_eqElm(_s_dae_linear_attenuation)) {
          ak_basic_attrd * linear_attenuation;

          linear_attenuation = ak_calloc(point,
                                          sizeof(*linear_attenuation),
                                          1);

          _xml_readTextUsingFn(linear_attenuation->val, strtod, NULL);
          _xml_readAttr(linear_attenuation,
                        linear_attenuation->sid,
                        _s_dae_sid);

          point->linear_attenuation = linear_attenuation;
        } else if (_xml_eqElm(_s_dae_quadratic_attenuation)) {
          ak_basic_attrd * quadratic_attenuation;

          quadratic_attenuation = ak_calloc(point,
                                             sizeof(*quadratic_attenuation),
                                             1);

          _xml_readTextUsingFn(quadratic_attenuation->val, strtod, NULL);
          _xml_readAttr(quadratic_attenuation,
                        quadratic_attenuation->sid,
                        _s_dae_sid);

          point->quadratic_attenuation = quadratic_attenuation;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);
      
      techc->technique = point;
      techc->technique_type = AK_TECHNIQUE_COMMON_LIGHT_POINT;
    }

    /* light -> spot */
    else if (_xml_eqElm(_s_dae_point)) {
      AkSpot * spot;
      spot = ak_calloc(techc, sizeof(*spot), 1);

      do {
        _xml_beginElement(_s_dae_spot);

        if (_xml_eqElm(_s_dae_color)) {
          ak_dae_color(reader, true, &spot->color);
        } else if (_xml_eqElm(_s_dae_constant_attenuation)) {
          ak_basic_attrd * constant_attenuation;

          constant_attenuation = ak_calloc(spot,
                                            sizeof(*constant_attenuation), 1);

          _xml_readTextUsingFn(constant_attenuation->val, strtod, NULL);
          _xml_readAttr(constant_attenuation,
                        constant_attenuation->sid,
                        _s_dae_sid);

          spot->constant_attenuation = constant_attenuation;
        } else if (_xml_eqElm(_s_dae_linear_attenuation)) {
          ak_basic_attrd * linear_attenuation;

          linear_attenuation = ak_calloc(spot,
                                          sizeof(*linear_attenuation), 1);

          _xml_readTextUsingFn(linear_attenuation->val, strtod, NULL);
          _xml_readAttr(linear_attenuation,
                        linear_attenuation->sid,
                        _s_dae_sid);

          spot->linear_attenuation = linear_attenuation;
        } else if (_xml_eqElm(_s_dae_quadratic_attenuation)) {
          ak_basic_attrd * quadratic_attenuation;

          quadratic_attenuation = ak_calloc(spot,
                                             sizeof(*quadratic_attenuation), 1);

          _xml_readTextUsingFn(quadratic_attenuation->val, strtod, NULL);
          _xml_readAttr(quadratic_attenuation,
                        quadratic_attenuation->sid,
                        _s_dae_sid);

          spot->quadratic_attenuation = quadratic_attenuation;
        } else if (_xml_eqElm(_s_dae_falloff_angle)) {
          ak_basic_attrd * falloff_angle;

          falloff_angle = ak_calloc(spot, sizeof(*falloff_angle), 1);

          _xml_readTextUsingFn(falloff_angle->val, strtod, NULL);
          _xml_readAttr(falloff_angle, falloff_angle->sid, _s_dae_sid);

          spot->falloff_angle = falloff_angle;
        } else if (_xml_eqElm(_s_dae_falloff_exponent)) {
          ak_basic_attrd * falloff_exponent;

          falloff_exponent = ak_calloc(spot, sizeof(*falloff_exponent), 1);

          _xml_readTextUsingFn(falloff_exponent->val, strtod, NULL);
          _xml_readAttr(falloff_exponent, falloff_exponent->sid, _s_dae_sid);

          spot->falloff_exponent = falloff_exponent;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
      
      techc->technique = spot;
      techc->technique_type = AK_TECHNIQUE_COMMON_LIGHT_SPOT;
    } else if (_xml_eqElm(_s_dae_instance_material)) {
      AkInstanceMaterial *instanceMaterial;
      AkResult ret;
      ret = ak_dae_fxInstanceMaterial(memParent, reader, &instanceMaterial);

      if (ret == AK_OK) {
        if (last_instanceMaterial)
          last_instanceMaterial->next = instanceMaterial;
        else {
          techc->technique = instanceMaterial;
          techc->technique_type = AK_TECHNIQUE_COMMON_INSTANCE_MATERIAL;
        }

        last_instanceMaterial = instanceMaterial;
      }
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = techc;

  return AK_OK;
}
