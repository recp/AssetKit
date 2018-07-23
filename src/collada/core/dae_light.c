/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_light.h"
#include "dae_asset.h"
#include "dae_technique.h"
#include "dae_color.h"
#include <cglm/cglm.h>

#define AK_DEFAULT_LIGHT_DIR {0.0f, 0.0f, -1.0f}

AkResult _assetkit_hide
dae_light(AkXmlState * __restrict xst,
          void       * __restrict memParent,
          void      ** __restrict dest) {
  AkLight       *light;
  AkTechnique   *last_tq;
  AkXmlElmState  xest;

  light = ak_heap_calloc(xst->heap, memParent, sizeof(*light));

  ak_xml_readid(xst, light);
  light->name = ak_xml_attr(xst, light, _s_dae_name);

  last_tq = light->technique;

  ak_xest_init(xest, _s_dae_light)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      (void)dae_assetInf(xst, light, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_techniquec)) {
      AkLightBase *tcommon;
      AkResult     ret;

      tcommon = NULL;
      ret     = dae_light_tcommon(xst, light, &tcommon);
      if (ret == AK_OK)
        light->tcommon = tcommon;
    } else if (ak_xml_eqelm(xst, _s_dae_technique)) {
      AkTechnique *tq;
      AkResult ret;

      tq = NULL;
      ret = dae_technique(xst, light, &tq);
      if (ret == AK_OK) {
        if (last_tq)
          last_tq->next = tq;
        else
          light->technique = tq;

        last_tq = tq;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          light,
                          nodePtr,
                          &tree,
                          NULL);
      light->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = light;

  return AK_OK;
}

AkResult _assetkit_hide
dae_light_tcommon(AkXmlState   * __restrict xst,
                  void         * __restrict memParent,
                  AkLightBase ** __restrict dest) {
  AkCoordSys   *optCoordSys;
  AkXmlElmState xest;

  ak_xest_init(xest, _s_dae_techniquec)

  *dest = NULL;

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_ambient)) {
      AkAmbientLight *ambient;
      AkXmlElmState   xest2;

      ambient = ak_heap_calloc(xst->heap,
                               memParent,
                               sizeof(*ambient));

      ak_xest_init(xest2, _s_dae_ambient)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          dae_color(xst, ambient, true, true, &ambient->color);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      ambient->type = AK_LIGHT_TYPE_AMBIENT;
      *dest = ambient;
    }

    else if (ak_xml_eqelm(xst, _s_dae_directional)) {
      AkDirectionalLight *directional;
      AkXmlElmState       xest2;

      directional = ak_heap_calloc(xst->heap,
                                   memParent,
                                   sizeof(*directional));

      ak_xest_init(xest2, _s_dae_directional)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          dae_color(xst, directional, true, true, &directional->color);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      directional->type = AK_LIGHT_TYPE_DIRECTIONAL;
      *dest = directional;
    }

    else if (ak_xml_eqelm(xst, _s_dae_point)) {
      AkPointLight *point;
      AkXmlElmState xest2;

      point = ak_heap_calloc(xst->heap,
                             memParent,
                             sizeof(*point));

      /* default values */
      point->constAttn = 1.0f;

      ak_xest_init(xest2, _s_dae_point)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          ak_xml_sid_seta(xst,
                          point,
                          &point->base.color);

          dae_color(xst, point, true, true, &point->base.color);
        } else if (ak_xml_eqelm(xst, _s_dae_const_attn)) {
          ak_xml_sid_seta(xst,
                          point,
                          &point->constAttn);

          point->constAttn = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_linear_attn)) {
          ak_xml_sid_seta(xst,
                          point,
                          &point->linearAttn);

          point->linearAttn = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_quad_attn)) {
          ak_xml_sid_seta(xst,
                          point,
                          &point->quadAttn);

          point->quadAttn = ak_xml_valf(xst);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      point->base.type = AK_LIGHT_TYPE_POINT;
      *dest = &point->base;
    }

    else if (ak_xml_eqelm(xst, _s_dae_spot)) {
      AkSpotLight  *spot;
      AkXmlElmState xest2;

      spot = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*spot));

      /* default values */
      spot->constAttn    = 1.0f;
      spot->falloffAngle = glm_rad(180.0f);

      ak_xest_init(xest2, _s_dae_spot)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->base.color);

          dae_color(xst, spot, true, true, &spot->base.color);
        } else if (ak_xml_eqelm(xst, _s_dae_const_attn)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->constAttn);

          spot->constAttn = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_linear_attn)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->linearAttn);

          spot->linearAttn = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_quad_attn)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->quadAttn);

          spot->quadAttn = ak_xml_valf(xst);
        } else if (ak_xml_eqelm(xst, _s_dae_falloff_angle)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->falloffAngle);

          spot->falloffAngle = glm_rad(ak_xml_valf(xst));
        } else if (ak_xml_eqelm(xst, _s_dae_falloff_exp)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->falloffExp);

          spot->falloffExp = ak_xml_valf(xst);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      spot->base.type = AK_LIGHT_TYPE_SPOT;
      *dest = &spot->base;
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  if (*dest) {
    optCoordSys = (void *)ak_opt_get(AK_OPT_COORD);
    if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) == AK_COORD_CVT_ALL
        && optCoordSys != xst->doc->coordSys) {
      /* convert default cone direction to new coord sys */
      ak_coordCvtVectorTo(xst->doc->coordSys,
                          (vec3)AK_DEFAULT_LIGHT_DIR,
                          optCoordSys,
                          (*dest)->direction);
    } else {
      glm_vec_copy((vec3)AK_DEFAULT_LIGHT_DIR,
                   (*dest)->direction);
    }
  }

  return AK_OK;
}
