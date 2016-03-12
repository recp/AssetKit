/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_technique.h"
#include "aio_collada_common.h"
#include "aio_collada_color.h"

int _assetio_hide
aio_dae_techniquec(xmlTextReaderPtr __restrict reader,
                   aio_technique_common ** __restrict dest) {

  aio_technique_common *techc;
  const xmlChar * nodeName;
  int             nodeType;
  int             nodeRet;

  techc = aio_calloc(sizeof(*techc), 1);

  do {
    _xml_beginElement(_s_dae_techniquec);

    /* optics -> perspective */
    if (_xml_eqElm(_s_dae_perspective)) {
      aio_perspective * perspective;
      perspective = aio_calloc(sizeof(*perspective), 1);
      
      do {
        _xml_beginElement(_s_dae_perspective);

        if (_xml_eqElm(_s_dae_xfov)) {
          aio_basic_attrd * xfov;
          xfov = aio_calloc(sizeof(*xfov), 1);

          _xml_readAttr(xfov->sid, _s_dae_sid);
          _xml_readTextUsingFn(xfov->val, strtod, NULL);

          perspective->xfov = xfov;
        } else if (_xml_eqElm(_s_dae_yfov)) {
          aio_basic_attrd * yfov;
          yfov = aio_calloc(sizeof(*yfov), 1);

          _xml_readAttr(yfov->sid, _s_dae_sid);
          _xml_readTextUsingFn(yfov->val, strtod, NULL);

          perspective->yfov = yfov;
        } else if (_xml_eqElm(_s_dae_aspect_ratio)) {
          aio_basic_attrd * aspect_ratio;
          aspect_ratio = aio_calloc(sizeof(*aspect_ratio), 1);

          _xml_readAttr(aspect_ratio->sid, _s_dae_sid);
          _xml_readTextUsingFn(aspect_ratio->val, strtod, NULL);

          perspective->aspect_ratio = aspect_ratio;
        } else if (_xml_eqElm(_s_dae_znear)) {
          aio_basic_attrd * znear;
          znear = aio_calloc(sizeof(*znear), 1);

          _xml_readAttr(znear->sid, _s_dae_sid);
          _xml_readTextUsingFn(znear->val, strtod, NULL);

          perspective->znear = znear;
        } else if (_xml_eqElm(_s_dae_zfar)) {
          aio_basic_attrd * zfar;
          zfar = aio_calloc(sizeof(*zfar), 1);

          _xml_readAttr(zfar->sid, _s_dae_sid);
          _xml_readTextUsingFn(zfar->val, strtod, NULL);

          perspective->zfar = zfar;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = perspective;
      techc->technique_type = AIO_TECHNIQUE_COMMON_CAMERA_PERSPECTIVE;
    }

    /* optics -> orthographic */
    else if (_xml_eqElm(_s_dae_orthographic)) {
      aio_orthographic * orthographic;
      orthographic = aio_calloc(sizeof(*orthographic), 1);

      do {
        _xml_beginElement(_s_dae_orthographic);

        if (_xml_eqElm(_s_dae_xmag)) {
          aio_basic_attrd * xmag;
          xmag = aio_calloc(sizeof(*xmag), 1);

          _xml_readAttr(xmag->sid, _s_dae_sid);
          _xml_readTextUsingFn(xmag->val, strtod, NULL);

          orthographic->xmag = xmag;
        } else if (_xml_eqElm(_s_dae_ymag)) {
          aio_basic_attrd * ymag;
          ymag = aio_calloc(sizeof(*ymag), 1);

          _xml_readAttr(ymag->sid, _s_dae_sid);
          _xml_readTextUsingFn(ymag->val, strtod, NULL);

          orthographic->ymag = ymag;
        } else if (_xml_eqElm(_s_dae_aspect_ratio)) {
          aio_basic_attrd * aspect_ratio;
          aspect_ratio = aio_calloc(sizeof(*aspect_ratio), 1);

          _xml_readAttr(aspect_ratio->sid, _s_dae_sid);
          _xml_readTextUsingFn(aspect_ratio->val, strtod, NULL);

          orthographic->aspect_ratio = aspect_ratio;
        } else if (_xml_eqElm(_s_dae_znear)) {
          aio_basic_attrd * znear;
          znear = aio_calloc(sizeof(*znear), 1);

          _xml_readAttr(znear->sid, _s_dae_sid);
          _xml_readTextUsingFn(znear->val, strtod, NULL);

          orthographic->znear = znear;
        } else if (_xml_eqElm(_s_dae_zfar)) {
          aio_basic_attrd * zfar;
          zfar = aio_calloc(sizeof(*zfar), 1);

          _xml_readAttr(zfar->sid, _s_dae_sid);
          _xml_readTextUsingFn(zfar->val, strtod, NULL);

          orthographic->zfar = zfar;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = orthographic;
      techc->technique_type = AIO_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC;
    }

    /* light -> ambient */
    else if (_xml_eqElm(_s_dae_ambient)) {
      aio_ambient * ambient;
      ambient = aio_calloc(sizeof(*ambient), 1);

      do {
        _xml_beginElement(_s_dae_ambient);

        if (_xml_eqElm(_s_dae_color)) {
          char *colorStr;

          _xml_readAttr(ambient->color.sid, _s_dae_sid);
          _xml_readMutText(colorStr);

          if (colorStr) {
            aio_strtof4(&colorStr, &ambient->color.vec);
            xmlFree(colorStr);
          }
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = ambient;
      techc->technique_type = AIO_TECHNIQUE_COMMON_LIGHT_AMBIENT;
    }

    /* light -> directional */
    else if (_xml_eqElm(_s_dae_directional)) {
      aio_directional * directional;
      directional = aio_calloc(sizeof(*directional), 1);

      do {
        _xml_beginElement(_s_dae_directional);

        if (_xml_eqElm(_s_dae_color)) {
          aio_dae_color(reader, true, &directional->color);
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      techc->technique = directional;
      techc->technique_type = AIO_TECHNIQUE_COMMON_LIGHT_DIRECTIONAL;
    }

    /* light -> point */
    else if (_xml_eqElm(_s_dae_point)) {
      aio_point * point;
      point = aio_calloc(sizeof(*point), 1);

      do {
        _xml_beginElement(_s_dae_point);

        if (_xml_eqElm(_s_dae_color)) {
          aio_dae_color(reader, true, &point->color);
        } else if (_xml_eqElm(_s_dae_constant_attenuation)) {
          aio_basic_attrd * constant_attenuation;

          constant_attenuation = aio_calloc(sizeof(*constant_attenuation), 1);

          _xml_readTextUsingFn(constant_attenuation->val, strtod, NULL);
          _xml_readAttr(constant_attenuation->sid, _s_dae_sid);

          point->constant_attenuation = constant_attenuation;
        } else if (_xml_eqElm(_s_dae_linear_attenuation)) {
          aio_basic_attrd * linear_attenuation;

          linear_attenuation = aio_calloc(sizeof(*linear_attenuation), 1);

          _xml_readTextUsingFn(linear_attenuation->val, strtod, NULL);
          _xml_readAttr(linear_attenuation->sid, _s_dae_sid);

          point->linear_attenuation = linear_attenuation;
        } else if (_xml_eqElm(_s_dae_quadratic_attenuation)) {
          aio_basic_attrd * quadratic_attenuation;

          quadratic_attenuation = aio_calloc(sizeof(*quadratic_attenuation), 1);

          _xml_readTextUsingFn(quadratic_attenuation->val, strtod, NULL);
          _xml_readAttr(quadratic_attenuation->sid, _s_dae_sid);

          point->quadratic_attenuation = quadratic_attenuation;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);
      
      techc->technique = point;
      techc->technique_type = AIO_TECHNIQUE_COMMON_LIGHT_POINT;
    }

    /* light -> spot */
    else if (_xml_eqElm(_s_dae_point)) {
      aio_spot * spot;
      spot = aio_calloc(sizeof(*spot), 1);

      do {
        _xml_beginElement(_s_dae_spot);

        if (_xml_eqElm(_s_dae_color)) {
          aio_dae_color(reader, true, &spot->color);
        } else if (_xml_eqElm(_s_dae_constant_attenuation)) {
          aio_basic_attrd * constant_attenuation;

          constant_attenuation = aio_calloc(sizeof(*constant_attenuation), 1);

          _xml_readTextUsingFn(constant_attenuation->val, strtod, NULL);
          _xml_readAttr(constant_attenuation->sid, _s_dae_sid);

          spot->constant_attenuation = constant_attenuation;
        } else if (_xml_eqElm(_s_dae_linear_attenuation)) {
          aio_basic_attrd * linear_attenuation;

          linear_attenuation = aio_calloc(sizeof(*linear_attenuation), 1);

          _xml_readTextUsingFn(linear_attenuation->val, strtod, NULL);
          _xml_readAttr(linear_attenuation->sid, _s_dae_sid);

          spot->linear_attenuation = linear_attenuation;
        } else if (_xml_eqElm(_s_dae_quadratic_attenuation)) {
          aio_basic_attrd * quadratic_attenuation;

          quadratic_attenuation = aio_calloc(sizeof(*quadratic_attenuation), 1);

          _xml_readTextUsingFn(quadratic_attenuation->val, strtod, NULL);
          _xml_readAttr(quadratic_attenuation->sid, _s_dae_sid);

          spot->quadratic_attenuation = quadratic_attenuation;
        } else if (_xml_eqElm(_s_dae_falloff_angle)) {
          aio_basic_attrd * falloff_angle;

          falloff_angle = aio_calloc(sizeof(*falloff_angle), 1);

          _xml_readTextUsingFn(falloff_angle->val, strtod, NULL);
          _xml_readAttr(falloff_angle->sid, _s_dae_sid);

          spot->falloff_angle = falloff_angle;
        } else if (_xml_eqElm(_s_dae_falloff_exponent)) {
          aio_basic_attrd * falloff_exponent;

          falloff_exponent = aio_calloc(sizeof(*falloff_exponent), 1);

          _xml_readTextUsingFn(falloff_exponent->val, strtod, NULL);
          _xml_readAttr(falloff_exponent->sid, _s_dae_sid);

          spot->falloff_exponent = falloff_exponent;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
      
      techc->technique = spot;
      techc->technique_type = AIO_TECHNIQUE_COMMON_LIGHT_SPOT;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = techc;

  return 0;
}
