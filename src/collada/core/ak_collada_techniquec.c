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
  AkXmlElmState       xest;

  techc = ak_heap_calloc(xst->heap, memParent, sizeof(*techc));

  last_instanceMaterial = NULL;

  ak_xest_init(xest, _s_dae_techniquec)

  do {
    if (ak_xml_begin(&xest))
      break;

    /* optics -> perspective */
    if (ak_xml_eqelm(xst, _s_dae_perspective)) {
      AkPerspective *perspective;
      AkXmlElmState  xest2;

      perspective = ak_heap_calloc(xst->heap,
                                   techc,
                                   sizeof(*perspective));

      ak_xest_init(xest2, _s_dae_perspective)

      do {
        if (ak_xml_begin(&xest2))
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
        if (ak_xml_end(&xest2))
          break;
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
      AkOrthographic *orthographic;
      AkXmlElmState   xest2;

      orthographic = ak_heap_calloc(xst->heap,
                                    techc,
                                    sizeof(*orthographic));

      ak_xest_init(xest2, _s_dae_orthographic)

      do {
        if (ak_xml_begin(&xest2))
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
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      techc->technique = orthographic;
      techc->techniqueType = AK_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC;
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
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = techc;

  return AK_OK;
}
