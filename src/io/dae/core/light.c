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
#include "color.h"
#include "../../../string_fast.h"

#define AK_DEFAULT_LIGHT_DIR {0.0f, 0.0f, -1.0f}
/* DAE attenuation has no finite cutoff; use 1% intensity as a range hint. */
#define AK_DAE_ATTENUATION_RANGE_EPSILON 0.01f

static
float
dae_lightRangeFromAttenuation(AkLightAttenuation * __restrict attenuation);

static
bool
dae_lightProfileEq(const xml_attr_t * __restrict attr,
                   const char       * __restrict value,
                   size_t                        len) {
  return attr && ak_str_eq_fast(attr->val, attr->valsize, value, len);
}

static
float
dae_lightVendorAngle(xml_t * __restrict xml, float fallback) {
  float angle;

  angle = glm_rad(xml_float(xml, glm_deg(fallback)));
  return glm_clamp(angle, 0.0f, GLM_PI_2f);
}

/* profile_COMMON falloff_angle is the full spot cone angle in degrees.
   AssetKit stores the conventional half angle used by glTF and its public
   light API, so normalize at the DAE boundary. */
static
float
dae_lightCommonOuterHalfAngle(xml_t * __restrict xml, float fallback) {
  float fullAngle;

  fullAngle = xml_float(xml, glm_deg(fallback) * 2.0f);
  return glm_clamp(glm_rad(fullAngle) * 0.5f, 0.0f, GLM_PI_2f);
}

static
void
dae_lightVendorTechnique(xml_t    * __restrict technique,
                         AkLight  * __restrict light) {
  AkLightBase        *base;
  AkPointLight       *point;
  AkSpotLight        *spot;
  AkLightAttenuation *attenuation;
  xml_attr_t         *profile;
  xml_t              *item;
  float               innerAngle;
  float               outerAngle;
  float               penumbraAngle;
  bool                hasInnerAngle;
  bool                hasOuterAngle;
  bool                hasPenumbraAngle;

  if (!technique || !light || !(base = light->data))
    return;

  profile = DAE_XMLA8(technique, profile);
  if (!dae_lightProfileEq(profile, _s_dae_fcollada, _s_dae_fcollada_len)
      && !dae_lightProfileEq(profile, _s_dae_max3d, _s_dae_max3d_len)
      && !dae_lightProfileEq(profile, _s_dae_opencollada3dsmax,
                             _s_dae_opencollada3dsmax_len))
    return;

  point = base->type == AK_LIGHT_TYPE_POINT ? (AkPointLight *)base : NULL;
  spot = base->type == AK_LIGHT_TYPE_SPOT ? (AkSpotLight *)base : NULL;
  attenuation = point ? &point->attenuation
                      : (spot ? &spot->attenuation : NULL);
  innerAngle = spot ? spot->innerConeAngle : 0.0f;
  outerAngle = spot ? spot->outerConeAngle : 0.0f;
  penumbraAngle = 0.0f;
  hasInnerAngle = false;
  hasOuterAngle = false;
  hasPenumbraAngle = false;

  for (item = technique->val; item; item = item->next) {
    if (DAE_XML_TAG_EQ(item, intensity)) {
      base->intensity = xml_float(item, 1.0f);
    } else if (attenuation && DAE_XML_TAG_EQ(item, const_attn)) {
      attenuation->constant = xml_float(item, attenuation->constant);
    } else if (attenuation && DAE_XML_TAG_EQ(item, linear_attn)) {
      attenuation->linear = xml_float(item, attenuation->linear);
    } else if (attenuation && DAE_XML_TAG_EQ(item, quad_attn)) {
      attenuation->quadratic = xml_float(item, attenuation->quadratic);
    } else if (spot && DAE_XML_TAG_EQ(item, falloff_angle)) {
      innerAngle = dae_lightVendorAngle(item, innerAngle);
      hasInnerAngle = true;
    } else if (spot && DAE_XML_TAG_EQ(item, falloff_exp)) {
      spot->coneFalloffExponent = xml_float(item,
                                             spot->coneFalloffExponent);
    } else if (spot && DAE_XML_TAG_EQ(item, hotspot_beam)) {
      innerAngle = dae_lightVendorAngle(item, innerAngle);
      hasInnerAngle = true;
    } else if (spot && (DAE_XML_TAG_EQ(item, outer_cone)
                        || DAE_XML_TAG_EQ(item, falloff)
                        || DAE_XML_TAG_EQ(item, decay_falloff))) {
      outerAngle = dae_lightVendorAngle(item, outerAngle);
      hasOuterAngle = true;
    } else if (spot && DAE_XML_TAG_EQ(item, penumbra_angle)) {
      penumbraAngle = glm_rad(xml_float(item, 0.0f));
      hasPenumbraAngle = true;
    }
  }

  if (spot) {
    if (hasInnerAngle)
      spot->innerConeAngle = innerAngle;
    if (hasOuterAngle)
      spot->outerConeAngle = outerAngle;
    else if (hasPenumbraAngle)
      spot->outerConeAngle = spot->innerConeAngle + penumbraAngle;

    if (hasOuterAngle || hasPenumbraAngle) {
      spot->outerConeAngle = glm_clamp(spot->outerConeAngle,
                                       spot->innerConeAngle,
                                       GLM_PI_2f);
    }
  }

  if (attenuation)
    base->range = dae_lightRangeFromAttenuation(attenuation);
}

static
void
dae_lightVendorExtra(xml_t   * __restrict extra,
                     AkLight * __restrict light) {
  xml_t *technique;

  if (!extra || !light)
    return;

  for (technique = extra->val; technique; technique = technique->next) {
    if (DAE_XML_TAG_EQ(technique, technique))
      dae_lightVendorTechnique(technique, light);
  }
}

static
float
dae_lightRangeFromAttenuation(AkLightAttenuation * __restrict attenuation) {
  float constant, linear, quadratic, target, disc, range;

  if (!attenuation)
    return 0.0f;

  constant  = attenuation->constant;
  linear    = attenuation->linear;
  quadratic = attenuation->quadratic;
  target    = 1.0f / AK_DAE_ATTENUATION_RANGE_EPSILON;

  if (quadratic > 1e-6f) {
    disc = linear * linear + 4.0f * quadratic * (target - constant);
    if (disc > 0.0f) {
      range = (-linear + sqrtf(disc)) / (2.0f * quadratic);
      return isfinite(range) && range > 0.0f ? range : 0.0f;
    }
  }

  if (linear > 1e-6f && target > constant) {
    range = (target - constant) / linear;
    return isfinite(range) && range > 0.0f ? range : 0.0f;
  }

  return 0.0f;
}

AK_HIDE
void*
dae_light(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkLight     *light;
  AkHeap      *heap;
  xml_t       *items;
  bool         pendingVendor;

  heap        = dst->heap;
  light       = ak_heap_calloc(heap, memp, sizeof(*light));
  ak_setypeid(light, AKT_LIGHT);
  light->name = DAE_XMLA_STRDUP8(xml, heap, name, light);
  
  xmla_setid(xml, heap, light);

  items = xml->val;
  xml   = items;
  pendingVendor = false;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, light, NULL);
    } else if (DAE_XML_TAG_EQ(xml, techniquec)) {
      xml_t          *xtech, *xtechv, *xcolor;
      AkAmbientLight *lightb;
      AkCoordSys     *optCoordSys;
      
      xtech = xml->val;
  
      if (DAE_XML_TAG_EQ8(xtech, ambient)) {
        lightb       = ak_heap_aligned_calloc(
          heap,
          light,
          AK_ALIGNOF(AkAmbientLight),
          sizeof(AkAmbientLight));
        lightb->type = AK_LIGHT_TYPE_AMBIENT;
      } else if (DAE_XML_TAG_EQ(xtech, directional)) {
        lightb       = ak_heap_aligned_calloc(
          heap,
          light,
          AK_ALIGNOF(AkDirectionalLight),
          sizeof(AkDirectionalLight));
        lightb->type = AK_LIGHT_TYPE_DIRECTIONAL;
      } else if (DAE_XML_TAG_EQ8(xtech, point)) {
        AkPointLight *point;

        point                  = ak_heap_aligned_calloc(
          heap,
          light,
          AK_ALIGNOF(AkPointLight),
          sizeof(*point));
        point->base.type       = AK_LIGHT_TYPE_POINT;
        point->attenuation.constant = 1.0f;
        lightb                 = &point->base;

        xtechv = xtech->val;
        while (xtechv) {
          if (DAE_XML_TAG_EQ(xtechv, const_attn)) {
            sid_seta(xtechv, heap, point, &point->attenuation.constant);
            point->attenuation.constant = xml_float(xtechv, 1.0f);
          } else if (DAE_XML_TAG_EQ(xtechv, linear_attn)) {
            sid_seta(xtechv, heap, point, &point->attenuation.linear);
            point->attenuation.linear = xml_float(xtechv, 0.0f);
          } else if (DAE_XML_TAG_EQ(xtechv, quad_attn)) {
            sid_seta(xtechv, heap, point, &point->attenuation.quadratic);
            point->attenuation.quadratic = xml_float(xtechv, 0.0f);
          }
          xtechv = xtechv->next;
        }
        point->base.range = dae_lightRangeFromAttenuation(&point->attenuation);
      } else if (DAE_XML_TAG_EQ8(xtech, spot)) {
        AkSpotLight *spot;

        spot            = ak_heap_aligned_calloc(heap,
                                                 light,
                                                 AK_ALIGNOF(AkSpotLight),
                                                 sizeof(*spot));
        spot->base.type = AK_LIGHT_TYPE_SPOT;
        lightb          = &spot->base;

        spot->attenuation.constant = 1.0f;
        spot->innerConeAngle = 0.0f;
        spot->outerConeAngle = GLM_PI_2f;
        spot->coneFalloffExponent = 1.0f;
        
        xtechv = xtech->val;
        while (xtechv) {
          if (DAE_XML_TAG_EQ(xtechv, const_attn)) {
            sid_seta(xtechv, heap, spot, &spot->attenuation.constant);
            spot->attenuation.constant = xml_float(xtechv, 1.0f);
          } else if (DAE_XML_TAG_EQ(xtechv, linear_attn)) {
            sid_seta(xtechv, heap, spot, &spot->attenuation.linear);
            spot->attenuation.linear = xml_float(xtechv, 0.0f);
          } else if (DAE_XML_TAG_EQ(xtechv, quad_attn)) {
            sid_seta(xtechv, heap, spot, &spot->attenuation.quadratic);
            spot->attenuation.quadratic = xml_float(xtechv, 0.0f);
          } else if (DAE_XML_TAG_EQ(xtechv, falloff_angle)) {
            sid_seta(xtechv, heap, spot, &spot->outerConeAngle);
            spot->outerConeAngle = dae_lightCommonOuterHalfAngle(
              xtechv,
              GLM_PI_2f);
          } else if (DAE_XML_TAG_EQ(xtechv, falloff_exp)) {
            sid_seta(xtechv, heap, spot, &spot->coneFalloffExponent);
            spot->coneFalloffExponent = xml_float(xtechv, 1.0f);
          }
          xtechv = xtechv->next;
        }
        spot->base.range = dae_lightRangeFromAttenuation(&spot->attenuation);
      } else {
        goto nxt;
      }

      lightb->intensity = 1.0f;

      if ((xcolor = DAE_XML_ELEM8(xtech, color))) {
        dae_color(xcolor, lightb, true, true, &lightb->color);
      } else {
        glm_vec4_one(lightb->color.vec);
      }
      
      if ((light->data = lightb)) {
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
    } else if (DAE_XML_TAG_EQ(xml, technique)) {
      if (light->data)
        dae_lightVendorTechnique(xml, light);
      else
        pendingVendor = true;
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      if (light->data)
        dae_lightVendorExtra(xml, light);
      else
        pendingVendor = true;
      light->extra = tree_fromxml(heap, light, xml);
    }

  nxt:
    xml = xml->next;
  }

  if (pendingVendor && light->data) {
    for (xml = items; xml; xml = xml->next) {
      if (DAE_XML_TAG_EQ(xml, technique))
        dae_lightVendorTechnique(xml, light);
      else if (DAE_XML_TAG_EQ8(xml, extra))
        dae_lightVendorExtra(xml, light);
    }
  }
  
  return light;
}
