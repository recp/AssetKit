/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_light.h"
#include "ak_collada_asset.h"
#include "ak_collada_technique.h"
#include "ak_collada_color.h"

AkResult _assetkit_hide
ak_dae_light(AkXmlState * __restrict xst,
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
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, light, &assetInf);
      if (ret == AK_OK)
        light->inf = assetInf;

    } else if (ak_xml_eqelm(xst, _s_dae_techniquec)) {
      AkLightBase *tcommon;
      AkResult     ret;

      tcommon = NULL;
      ret     = ak_dae_light_tcommon(xst, light, &tcommon);
      if (ret == AK_OK)
        light->tcommon = tcommon;
    } else if (ak_xml_eqelm(xst, _s_dae_technique)) {
      AkTechnique *tq;
      AkResult ret;

      tq = NULL;
      ret = ak_dae_technique(xst, light, &tq);
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
ak_dae_light_tcommon(AkXmlState   * __restrict xst,
                     void         * __restrict memParent,
                     AkLightBase ** __restrict dest) {
  AkXmlElmState xest;

  ak_xest_init(xest, _s_dae_techniquec)

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
          char *colorStr;

          ak_xml_readsid(xst, ambient);
          colorStr = ak_xml_rawcval(xst);

          if (colorStr)
            ak_strtof4(&colorStr, &ambient->color.vec);
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
          char *colorStr;

          ak_xml_readsid(xst, directional);
          colorStr = ak_xml_rawcval(xst);

          if (colorStr)
            ak_strtof4(&colorStr, &directional->color.vec);
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

      ak_xest_init(xest2, _s_dae_point)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          ak_xml_sid_seta(xst,
                          point,
                          &point->base.color);

          ak_dae_color(xst, false, &point->base.color);
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

      ak_xest_init(xest2, _s_dae_spot)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_color)) {
          ak_xml_sid_seta(xst,
                          spot,
                          &spot->base.color);

          ak_dae_color(xst, false, &spot->base.color);
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

          spot->falloffAngle = ak_xml_valf(xst);
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

  return AK_OK;
}
