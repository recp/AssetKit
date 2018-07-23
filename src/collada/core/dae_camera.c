/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_camera.h"
#include "dae_asset.h"
#include "dae_technique.h"
#include <cglm/cglm.h>

AkResult _assetkit_hide
dae_camera(AkXmlState * __restrict xst,
              void * __restrict memParent,
              void ** __restrict  dest) {
  AkCamera     *camera;
  AkXmlElmState xest;

  camera = ak_heap_calloc(xst->heap, memParent, sizeof(*camera));

  ak_xml_readid(xst, camera);
  camera->name = ak_xml_attr(xst, camera, _s_dae_name);

  ak_xest_init(xest, _s_dae_camera)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)dae_assetInf(xst, camera, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_optics)) {
      AkOptics     *optics;
      AkTechnique  *last_tq;
      AkXmlElmState xest2;

      optics  = ak_heap_calloc(xst->heap,
                               camera,
                               sizeof(*optics));
      last_tq = optics->technique;

      ak_xest_init(xest2, _s_dae_optics)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_techniquec)) {
          AkProjection *tcommon;
          AkResult      ret;

          tcommon = NULL;
          ret     = dae_camera_tcommon(xst, optics, &tcommon);
          if (ret == AK_OK)
            optics->tcommon = tcommon;
        } else if (ak_xml_eqelm(xst, _s_dae_technique)) {
          AkTechnique *tq;
          AkResult ret;

          tq = NULL;
          ret = dae_technique(xst, optics, &tq);
          if (ret == AK_OK) {
            if (last_tq)
              last_tq->next = tq;
            else
              optics->technique = tq;

            last_tq = tq;
          }
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      camera->optics = optics;
    } else if (ak_xml_eqelm(xst, _s_dae_imager)) {
      AkImager     *imager;
      AkTechnique  *last_tq;
      AkXmlElmState xest2;

      imager  = ak_heap_calloc(xst->heap, camera, sizeof(*imager));
      last_tq = imager->technique;

      ak_xest_init(xest2, _s_dae_imager)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_technique)) {
          AkTechnique *tq;
          AkResult ret;

          tq = NULL;
          ret = dae_technique(xst, imager, &tq);
          if (ret == AK_OK) {
            if (last_tq)
              last_tq->next = tq;
            else
              imager->technique = tq;

            last_tq = tq;
          }
        } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(xst->reader);
          tree = NULL;

          ak_tree_fromXmlNode(xst->heap,
                              imager,
                              nodePtr,
                              &tree,
                              NULL);
          imager->extra = tree;

          ak_xml_skipelm(xst);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      camera->imager = imager;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          camera,
                          nodePtr,
                          &tree,
                          NULL);
      camera->extra = tree;

      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = camera;

  return AK_OK;
}

AkResult _assetkit_hide
dae_camera_tcommon(AkXmlState    * __restrict xst,
                   void          * __restrict memParent,
                   AkProjection ** __restrict dest) {
  AkXmlElmState xest;

  ak_xest_init(xest, _s_dae_techniquec)

  do {
    if (ak_xml_begin(&xest))
      break;

    /* optics -> perspective */
    if (ak_xml_eqelm(xst, _s_dae_perspective)) {
      AkPerspective *perspective;
      AkXmlElmState  xest2;

      perspective = ak_heap_calloc(xst->heap,
                                   memParent,
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

      perspective->base.type = AK_PROJECTION_PERSPECTIVE;
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

      *dest = &perspective->base;
    }

    /* optics -> orthographic */
    else if (ak_xml_eqelm(xst, _s_dae_orthographic)) {
      AkOrthographic *ortho;
      AkXmlElmState   xest2;

      ortho = ak_heap_calloc(xst->heap,
                             memParent,
                             sizeof(*ortho));

      ak_xest_init(xest2, _s_dae_orthographic)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_xmag)) {
          ak_xml_sid_seta(xst,
                          ortho,
                          &ortho->xmag);

          ortho->xmag = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_ymag)) {
          ak_xml_sid_seta(xst,
                          ortho,
                          &ortho->ymag);

          ortho->ymag = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_aspect_ratio)) {
          ak_xml_sid_seta(xst,
                          ortho,
                          &ortho->aspectRatio);

          ortho->aspectRatio = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_znear)) {
          ak_xml_sid_seta(xst,
                          ortho,
                          &ortho->znear);

          ortho->znear = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_zfar)) {
          ak_xml_sid_seta(xst,
                          ortho,
                          &ortho->zfar);

          ortho->zfar = ak_xml_valf(xst);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      ortho->base.type = AK_PROJECTION_ORTHOGRAPHIC;
      if (!ortho->aspectRatio
          && ortho->ymag
          && ortho->xmag) {
        ortho->aspectRatio = ortho->xmag / ortho->ymag;
      } else if (!ortho->ymag
                 && ortho->aspectRatio
                 && ortho->xmag) {
        ortho->ymag = ortho->xmag / ortho->aspectRatio;
      } else if (!ortho->xmag
                 && ortho->aspectRatio
                 && ortho->ymag) {
        ortho->xmag = ortho->ymag * ortho->aspectRatio;
      }
      *dest = &ortho->base;
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  return AK_OK;
}
