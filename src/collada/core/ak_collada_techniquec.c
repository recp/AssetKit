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
ak_dae_techniquec(AkHeap * __restrict heap,
                  void * __restrict memParent,
                  xmlTextReaderPtr reader,
                  AkTechniqueCommon ** __restrict dest) {

  AkTechniqueCommon *techc;
  AkInstanceMaterial  *last_instanceMaterial;

  const xmlChar * nodeName;
  int             nodeType;
  int             nodeRet;

  techc = ak_heap_calloc(heap, memParent, sizeof(*techc), false);

  last_instanceMaterial = NULL;

  do {
    _xml_beginElement(_s_dae_techniquec);

    /* optics -> perspective */
    if (_xml_eqElm(_s_dae_perspective)) {
      AkPerspective * perspective;
      perspective = ak_heap_calloc(heap, techc, sizeof(*perspective), false);
      
      do {
        _xml_beginElement(_s_dae_perspective);

        if (_xml_eqElm(_s_dae_xfov)) {
          ak_basic_attrf * xfov;
          xfov = ak_heap_calloc(heap, perspective, sizeof(*xfov), false);

          _xml_readAttr(xfov, xfov->sid, _s_dae_sid);
          _xml_readTextUsingFn(xfov->val, strtof, NULL);
          xfov->val = glm_rad(xfov->val);

          perspective->xfov = xfov;
        } else if (_xml_eqElm(_s_dae_yfov)) {
          ak_basic_attrf * yfov;
          yfov = ak_heap_calloc(heap, perspective, sizeof(*yfov), false);

          _xml_readAttr(yfov, yfov->sid, _s_dae_sid);
          _xml_readTextUsingFn(yfov->val, strtof, NULL);
          yfov->val = glm_rad(yfov->val);

          perspective->yfov = yfov;
        } else if (_xml_eqElm(_s_dae_aspect_ratio)) {
          ak_basic_attrf * aspectRatio;
          aspectRatio = ak_heap_calloc(heap,
                                       perspective,
                                       sizeof(*aspectRatio),
                                       false);

          _xml_readAttr(aspectRatio, aspectRatio->sid, _s_dae_sid);
          _xml_readTextUsingFn(aspectRatio->val, strtof, NULL);

          perspective->aspectRatio = aspectRatio;
        } else if (_xml_eqElm(_s_dae_znear)) {
          ak_basic_attrf * znear;
          znear = ak_heap_calloc(heap, perspective, sizeof(*znear), false);

          _xml_readAttr(znear, znear->sid, _s_dae_sid);
          _xml_readTextUsingFn(znear->val, strtof, NULL);

          perspective->znear = znear;
        } else if (_xml_eqElm(_s_dae_zfar)) {
          ak_basic_attrf * zfar;
          zfar = ak_heap_calloc(heap, perspective, sizeof(*zfar), false);

          _xml_readAttr(zfar, zfar->sid, _s_dae_sid);
          _xml_readTextUsingFn(zfar->val, strtof, NULL);

          perspective->zfar = zfar;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = perspective;
      techc->techniqueType = AK_TECHNIQUE_COMMON_CAMERA_PERSPECTIVE;

      if (!perspective->aspectRatio
          && perspective->yfov
          && perspective->xfov) {
        ak_basic_attrf * aspectRatio;
        aspectRatio = ak_heap_calloc(heap,
                                     perspective,
                                     sizeof(*aspectRatio),
                                     false);

        aspectRatio->val = perspective->xfov->val / perspective->yfov->val;
        perspective->aspectRatio = aspectRatio;
      } else if (!perspective->yfov
                 && perspective->aspectRatio
                 && perspective->xfov) {
        ak_basic_attrf * yfov;
        yfov = ak_heap_calloc(heap,
                              perspective,
                              sizeof(*yfov),
                              false);

        yfov->val = perspective->xfov->val / perspective->aspectRatio->val;
        perspective->yfov = yfov;
      } else if (!perspective->xfov
                 && perspective->aspectRatio
                 && perspective->yfov) {
        ak_basic_attrf * xfov;
        xfov = ak_heap_calloc(heap,
                              perspective,
                              sizeof(*xfov),
                              false);

        xfov->val = perspective->yfov->val * perspective->aspectRatio->val;
        perspective->xfov = xfov;
      }
    }

    /* optics -> orthographic */
    else if (_xml_eqElm(_s_dae_orthographic)) {
      AkOrthographic * orthographic;
      orthographic = ak_heap_calloc(heap, techc, sizeof(*orthographic), false);

      do {
        _xml_beginElement(_s_dae_orthographic);

        if (_xml_eqElm(_s_dae_xmag)) {
          ak_basic_attrf * xmag;
          xmag = ak_heap_calloc(heap, orthographic, sizeof(*xmag), false);

          _xml_readAttr(xmag, xmag->sid, _s_dae_sid);
          _xml_readTextUsingFn(xmag->val, strtof, NULL);

          orthographic->xmag = xmag;
        } else if (_xml_eqElm(_s_dae_ymag)) {
          ak_basic_attrf * ymag;
          ymag = ak_heap_calloc(heap, orthographic, sizeof(*ymag), false);

          _xml_readAttr(ymag, ymag->sid, _s_dae_sid);
          _xml_readTextUsingFn(ymag->val, strtof, NULL);

          orthographic->ymag = ymag;
        } else if (_xml_eqElm(_s_dae_aspect_ratio)) {
          ak_basic_attrf * aspectRatio;
          aspectRatio = ak_heap_calloc(heap,
                                       orthographic,
                                       sizeof(*aspectRatio),
                                       false);

          _xml_readAttr(aspectRatio, aspectRatio->sid, _s_dae_sid);
          _xml_readTextUsingFn(aspectRatio->val, strtof, NULL);

          orthographic->aspectRatio = aspectRatio;
        } else if (_xml_eqElm(_s_dae_znear)) {
          ak_basic_attrf * znear;
          znear = ak_heap_calloc(heap, orthographic, sizeof(*znear), false);

          _xml_readAttr(znear, znear->sid, _s_dae_sid);
          _xml_readTextUsingFn(znear->val, strtof, NULL);

          orthographic->znear = znear;
        } else if (_xml_eqElm(_s_dae_zfar)) {
          ak_basic_attrf * zfar;
          zfar = ak_heap_calloc(heap, orthographic, sizeof(*zfar), false);

          _xml_readAttr(zfar, zfar->sid, _s_dae_sid);
          _xml_readTextUsingFn(zfar->val, strtof, NULL);

          orthographic->zfar = zfar;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = orthographic;
      techc->techniqueType = AK_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC;
    }

    /* light -> ambient */
    else if (_xml_eqElm(_s_dae_ambient)) {
      AkAmbient * ambient;
      ambient = ak_heap_calloc(heap, techc, sizeof(*ambient), false);

      do {
        _xml_beginElement(_s_dae_ambient);

        if (_xml_eqElm(_s_dae_color)) {
          char *colorStr;

          _xml_readAttr(ambient, ambient->color.sid, _s_dae_sid);
          _xml_readMutText(colorStr);

          if (colorStr) {
            ak_strtof4(&colorStr, &ambient->color.color.vec);
            xmlFree(colorStr);
          }
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = ambient;
      techc->techniqueType = AK_TECHNIQUE_COMMON_LIGHT_AMBIENT;
    }

    /* light -> directional */
    else if (_xml_eqElm(_s_dae_directional)) {
      AkDirectional * directional;
      directional = ak_heap_calloc(heap, techc, sizeof(*directional), false);

      do {
        _xml_beginElement(_s_dae_directional);

        if (_xml_eqElm(_s_dae_color)) {
          ak_dae_color(heap, reader, true, &directional->color);
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = directional;
      techc->techniqueType = AK_TECHNIQUE_COMMON_LIGHT_DIRECTIONAL;
    }

    /* light -> point */
    else if (_xml_eqElm(_s_dae_point)) {
      AkPoint * point;
      point = ak_heap_calloc(heap, techc, sizeof(*point), false);

      do {
        _xml_beginElement(_s_dae_point);

        if (_xml_eqElm(_s_dae_color)) {
          ak_dae_color(heap, reader, true, &point->color);
        } else if (_xml_eqElm(_s_dae_constant_attenuation)) {
          ak_basic_attrd * constantAttenuation;

          constantAttenuation = ak_heap_calloc(heap, point,
                                          sizeof(*constantAttenuation),
                                          false);

          _xml_readTextUsingFn(constantAttenuation->val, strtod, NULL);
          _xml_readAttr(constantAttenuation,
                        constantAttenuation->sid,
                        _s_dae_sid);

          point->constantAttenuation = constantAttenuation;
        } else if (_xml_eqElm(_s_dae_linear_attenuation)) {
          ak_basic_attrd * linearAttenuation;

          linearAttenuation = ak_heap_calloc(heap, point,
                                        sizeof(*linearAttenuation),
                                        false);

          _xml_readTextUsingFn(linearAttenuation->val, strtod, NULL);
          _xml_readAttr(linearAttenuation,
                        linearAttenuation->sid,
                        _s_dae_sid);

          point->linearAttenuation = linearAttenuation;
        } else if (_xml_eqElm(_s_dae_quadratic_attenuation)) {
          ak_basic_attrd * quadraticAttenuation;

          quadraticAttenuation = ak_heap_calloc(heap, point,
                                           sizeof(*quadraticAttenuation),
                                           false);

          _xml_readTextUsingFn(quadraticAttenuation->val, strtod, NULL);
          _xml_readAttr(quadraticAttenuation,
                        quadraticAttenuation->sid,
                        _s_dae_sid);

          point->quadraticAttenuation = quadraticAttenuation;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);
      
      techc->technique = point;
      techc->techniqueType = AK_TECHNIQUE_COMMON_LIGHT_POINT;
    }

    /* light -> spot */
    else if (_xml_eqElm(_s_dae_point)) {
      AkSpot * spot;
      spot = ak_heap_calloc(heap, techc, sizeof(*spot), false);

      do {
        _xml_beginElement(_s_dae_spot);

        if (_xml_eqElm(_s_dae_color)) {
          ak_dae_color(heap, reader, true, &spot->color);
        } else if (_xml_eqElm(_s_dae_constant_attenuation)) {
          ak_basic_attrd * constantAttenuation;

          constantAttenuation = ak_heap_calloc(heap, spot,
                                          sizeof(*constantAttenuation),
                                          false);

          _xml_readTextUsingFn(constantAttenuation->val, strtod, NULL);
          _xml_readAttr(constantAttenuation,
                        constantAttenuation->sid,
                        _s_dae_sid);

          spot->constantAttenuation = constantAttenuation;
        } else if (_xml_eqElm(_s_dae_linear_attenuation)) {
          ak_basic_attrd * linearAttenuation;

          linearAttenuation = ak_heap_calloc(heap, spot,
                                        sizeof(*linearAttenuation),
                                        false);

          _xml_readTextUsingFn(linearAttenuation->val, strtod, NULL);
          _xml_readAttr(linearAttenuation,
                        linearAttenuation->sid,
                        _s_dae_sid);

          spot->linearAttenuation = linearAttenuation;
        } else if (_xml_eqElm(_s_dae_quadratic_attenuation)) {
          ak_basic_attrd * quadraticAttenuation;

          quadraticAttenuation = ak_heap_calloc(heap, spot,
                                           sizeof(*quadraticAttenuation),
                                           false);

          _xml_readTextUsingFn(quadraticAttenuation->val, strtod, NULL);
          _xml_readAttr(quadraticAttenuation,
                        quadraticAttenuation->sid,
                        _s_dae_sid);

          spot->quadraticAttenuation = quadraticAttenuation;
        } else if (_xml_eqElm(_s_dae_falloff_angle)) {
          ak_basic_attrd * falloffAngle;

          falloffAngle = ak_heap_calloc(heap,
                                        spot,
                                        sizeof(*falloffAngle),
                                        false);

          _xml_readTextUsingFn(falloffAngle->val, strtod, NULL);
          _xml_readAttr(falloffAngle, falloffAngle->sid, _s_dae_sid);

          spot->falloffAngle = falloffAngle;
        } else if (_xml_eqElm(_s_dae_falloff_exponent)) {
          ak_basic_attrd * falloffExponent;

          falloffExponent = ak_heap_calloc(heap,
                                           spot,
                                           sizeof(*falloffExponent),
                                           false);

          _xml_readTextUsingFn(falloffExponent->val, strtod, NULL);
          _xml_readAttr(falloffExponent, falloffExponent->sid, _s_dae_sid);

          spot->falloffExponent = falloffExponent;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
      
      techc->technique = spot;
      techc->techniqueType = AK_TECHNIQUE_COMMON_LIGHT_SPOT;
    } else if (_xml_eqElm(_s_dae_instance_material)) {
      AkInstanceMaterial *instanceMaterial;
      AkResult ret;
      ret = ak_dae_fxInstanceMaterial(heap,
                                      memParent,
                                      reader,
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
    _xml_endElement;
  } while (nodeRet);

  *dest = techc;

  return AK_OK;
}
