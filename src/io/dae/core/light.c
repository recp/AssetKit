/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "light.h"
#include "asset.h"
#include "techn.h"
#include "color.h"

#define AK_DEFAULT_LIGHT_DIR {0.0f, 0.0f, -1.0f}

_assetkit_hide
void*
dae_light(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkLight     *light;
  AkHeap      *heap;
  AkTechnique *tq;

  heap        = dst->heap;
  light       = ak_heap_calloc(heap, memp, sizeof(*light));
  light->name = xmla_strdup_by(xml, heap, _s_dae_name, light);
  
  xmla_setid(xml, heap, light);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, light, NULL);
    } else if (xml_tag_eq(xml, _s_dae_techniquec)) {
      xml_t          *xtech, *xtechv, *xcolor;
      AkAmbientLight *lightb;
      AkCoordSys     *optCoordSys;
      
      xtech = xml->val;
  
      if (xml_tag_eq(xtech, _s_dae_ambient)) {
        lightb       = ak_heap_calloc(heap, light, sizeof(AkAmbientLight));
        lightb->type = AK_LIGHT_TYPE_AMBIENT;
      } else if (xml_tag_eq(xtech, _s_dae_directional)) {
        lightb       = ak_heap_calloc(heap, light, sizeof(AkDirectionalLight));
        lightb->type = AK_LIGHT_TYPE_DIRECTIONAL;
      } else if (xml_tag_eq(xtech, _s_dae_point)) {
        AkPointLight *point;
        
        point            = ak_heap_calloc(heap, light, sizeof(*point));
        point->base.type = AK_LIGHT_TYPE_POINT;
        lightb           = &point->base;
        
        /* default values */
        point->constAttn = 1.0f;
        
        xtechv = xtech->val;
        while (xtechv) {
          if (xml_tag_eq(xtechv, _s_dae_color)) {
            sid_seta(xtechv, heap, point, &point->base.color);
          } else if (xml_tag_eq(xtechv, _s_dae_const_attn)) {
            sid_seta(xtechv, heap, point, &point->constAttn);
            point->constAttn = xml_float(xtechv, 1.0f);
          } else if (xml_tag_eq(xtechv, _s_dae_linear_attn)) {
            sid_seta(xtechv, heap, point, &point->linearAttn);
            point->linearAttn = xml_float(xtechv, 0.0f);
          } else if (xml_tag_eq(xtechv, _s_dae_quad_attn)) {
            sid_seta(xtechv, heap, point, &point->quadAttn);
            point->quadAttn = xml_float(xtechv, 0.0f);
          }
          xtechv = xtechv->next;
        }
      } else if (xml_tag_eq(xtech, _s_dae_spot)) {
        AkSpotLight *spot;

        spot            = ak_heap_calloc(heap, light, sizeof(*spot));
        spot->base.type = AK_LIGHT_TYPE_SPOT;
        lightb          = &spot->base;

        /* default values */
        spot->constAttn    = 1.0f;
        spot->falloffAngle = glm_rad(180.0f);
        
        xtechv = xtech->val;
        while (xtechv) {
          if (xml_tag_eq(xtechv, _s_dae_color)) {
            sid_seta(xtechv, heap, spot, &spot->base.color);
          } else if (xml_tag_eq(xtechv, _s_dae_const_attn)) {
            sid_seta(xtechv, heap, spot, &spot->constAttn);
            spot->constAttn = xml_float(xtechv, 0.0f);
          } else if (xml_tag_eq(xtechv, _s_dae_linear_attn)) {
            sid_seta(xtechv, heap, spot, &spot->linearAttn);
            spot->linearAttn = xml_float(xtechv, 0.0f);
          } else if (xml_tag_eq(xtechv, _s_dae_quad_attn)) {
            sid_seta(xtechv, heap, spot, &spot->quadAttn);
            spot->quadAttn = xml_float(xtechv, 0.0f);
          } else if (xml_tag_eq(xtechv, _s_dae_falloff_angle)) {
            sid_seta(xtechv, heap, spot, &spot->falloffAngle);
            spot->falloffAngle = xml_float(xtechv, 0.0f);
            glm_make_rad(&spot->falloffAngle);
          } else if (xml_tag_eq(xtechv, _s_dae_falloff_exp)) {
            sid_seta(xtechv, heap, spot, &spot->falloffExp);
            spot->falloffExp = xml_float(xtechv, 0.0f);
          }
          xtechv = xtechv->next;
        }
      } else {
        goto nxt;
      }

      if ((xcolor = xml_elem(xtech, _s_dae_color))) {
        dae_color(xcolor, lightb, true, true, &lightb->color);
      } else {
        glm_vec4_one(lightb->color.vec);
      }
      
      if ((light->tcommon = lightb)) {
        /* fix coord sys  */
        optCoordSys = (void *)ak_opt_get(AK_OPT_COORD);
        if (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE) == AK_COORD_CVT_ALL
            && optCoordSys != dst->doc->coordSys) {
          /* convert default cone direction to new coord sys */
          ak_coordCvtVectorTo(dst->doc->coordSys,
                              (vec3)AK_DEFAULT_LIGHT_DIR,
                              optCoordSys,
                              lightb->direction);
        } else {
          glm_vec3_copy((vec3)AK_DEFAULT_LIGHT_DIR,
                        lightb->direction);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_technique)) {
      tq               = dae_techn(xml, heap, light);
      tq->next         = light->technique;
      light->technique = tq;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      light->extra = tree_fromxml(heap, light, xml);
    }

  nxt:
    xml = xml->next;
  }
  
  return light;
}
