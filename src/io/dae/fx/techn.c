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

#include "techn.h"
#include "colortex.h"
#include "fltprm.h"

#include "../core/asset.h"
#include "../core/enum.h"
#include "../1.4/image.h"
#include "../bugfix/transp.h"
#include "../../../default/material.h"
#include "../../../string_fast.h"

#include <string.h>

static
AkTechniqueFxCommon*
dae_techniqueFxCmn(DAEState * __restrict dst,
                   xml_t    * __restrict xml,
                   void     * __restrict memp,
                   AkMaterialType        mattype);

static
void
dae_techniqueFxSceneKitExtra(DAEState            * __restrict dst,
                             xml_t               * __restrict xml,
                             AkTechniqueFxCommon * __restrict techn);

static
bool
dae_xmlAttrEq(const xml_attr_t * __restrict attr,
              const char       * __restrict value,
              size_t                        len) {
  return attr && ak_str_eq_fast(attr->val, attr->valsize, value, len);
}

AK_HIDE
AkTechniqueFx*
dae_techniqueFx(DAEState * __restrict dst,
                xml_t    * __restrict xml,
                void     * __restrict memp) {
  AkHeap              *heap;
  AkTechniqueFx       *techn;
  AkMaterialType       m;

  heap  = dst->heap;
  techn = ak_heap_calloc(heap, memp, sizeof(*techn));
  ak_setypeid(techn, AKT_TECHNIQUE_FX);

  xmla_setid(xml, heap, techn);
  sid_set(xml, heap, techn);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, techn, NULL);
    } else if ((DAE_XML_TAG_EQ8(xml, phong)    && (m = AK_MATERIAL_TYPE_PHONG))
           || (DAE_XML_TAG_EQ8(xml, blinn)     && (m = AK_MATERIAL_TYPE_BLINN))
           || (DAE_XML_TAG_EQ8(xml, lambert)   && (m = AK_MATERIAL_TYPE_LAMBERT))
           || (DAE_XML_TAG_EQ8(xml, constant)  && (m = AK_MATERIAL_TYPE_CONSTANT))) {
      techn->common = dae_techniqueFxCmn(dst, xml, techn, m);
    } else if (dst->version < AK_COLLADA_VERSION_150
               && DAE_XML_TAG_EQ8(xml, image)) {
      /* migration from 1.4 */
      dae14_fxMigrateImg(dst, xml, NULL);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      dae_techniqueFxSceneKitExtra(dst, xml, techn->common);
      techn->extra = tree_fromxml(heap, techn, xml);
    }
    xml = xml->next;
  }

  return techn;
}

static
void
dae_colorDescTextureUsage(DAEState            * __restrict dst,
                          AkColorDesc         * __restrict clr,
                          AkTextureColorSpace              colorSpace,
                          AkTextureChannels                channels) {
  AkDAETextureRef *tex;

  if (!clr || !dst->texmap)
    return;

  if ((tex = rb_find(dst->texmap, clr))) {
    tex->colorSpace = colorSpace;
    tex->channels   = channels;
  }
}

static
AkTextureChannels
dae_transparentTextureChannels(AkOpaque opaque) {
  switch (opaque) {
    case AK_OPAQUE_A_ONE:
    case AK_OPAQUE_A_ZERO:
    case AK_OPAQUE_MASK:
      return AK_TEXTURE_CHANNEL_A;
    case AK_OPAQUE_RGB_ONE:
    case AK_OPAQUE_RGB_ZERO:
      return AK_TEXTURE_CHANNEL_RGB;
    default:
      return AK_TEXTURE_CHANNEL_RGBA;
  }
}

static
void
dae_techniqueFxSceneKitExtra(DAEState            * __restrict dst,
                             xml_t               * __restrict xml,
                             AkTechniqueFxCommon * __restrict techn) {
  xml_t *technique, *item;

  if (!xml || !techn)
    return;

  for (technique = xml->val; technique; technique = technique->next) {
    if (!DAE_XML_TAG_EQ(technique, technique)
        || !dae_xmlAttrEq(DAE_XMLA8(technique, profile), "SceneKit", 8))
      continue;

    for (item = technique->val; item; item = item->next) {
      if (!xml_tag_eqsz(item, "constant_diffuse", sizeof("constant_diffuse") - 1))
        continue;

      if (!techn->constantDiffuse) {
        techn->constantDiffuse = dae_colorOrTex(dst, item, techn);
        dae_colorDescTextureUsage(dst, techn->constantDiffuse, AK_TEXTURE_COLORSPACE_SRGB,
                                  AK_TEXTURE_CHANNEL_RGBA);
      }
      return;
    }
  }
}

static
AkTechniqueFxCommon*
dae_techniqueFxCmn(DAEState * __restrict dst,
                   xml_t    * __restrict xml,
                   void     * __restrict memp,
                   AkMaterialType        mattype) {
  AkHeap              *heap;
  AkTechniqueFxCommon *techn;
  xml_attr_t          *att;
  AkTransparent       *transp;
  AkOpaque             opaque;

  heap        = dst->heap;
  techn       = ak_heap_calloc(heap, memp, sizeof(*techn));
  techn->type = mattype;
  xml         = xml->val;

  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, emission)) {
      AkMaterialEmissionProp *emission;

      if (!(emission = techn->emission)) {
        emission           = ak_heap_calloc(heap, techn, sizeof(*emission));
        techn->emission    = emission;
        emission->strength = 1.0f;
      }

      dae_colorOrTexSet(dst, xml, techn, &techn->emission->color);
      dae_colorDescTextureUsage(dst, &techn->emission->color,
                                AK_TEXTURE_COLORSPACE_SRGB,
                                AK_TEXTURE_CHANNEL_RGB);
    } else if (DAE_XML_TAG_EQ8(xml, ambient)) {
      techn->ambient = dae_colorOrTex(dst, xml, techn);
      dae_colorDescTextureUsage(dst, techn->ambient,
                                AK_TEXTURE_COLORSPACE_SRGB,
                                AK_TEXTURE_CHANNEL_RGB);
    } else if (DAE_XML_TAG_EQ8(xml, diffuse)) {
      techn->diffuse = dae_colorOrTex(dst, xml, techn);
      dae_colorDescTextureUsage(dst, techn->diffuse,
                                AK_TEXTURE_COLORSPACE_SRGB,
                                AK_TEXTURE_CHANNEL_RGBA);
    } else if (DAE_XML_TAG_EQ8(xml, specular)) {
      AkMaterialSpecularProp *specularProp;

      if (!(specularProp = techn->specular)) {
        specularProp    = ak_heap_calloc(heap, techn, sizeof(*specularProp));
        techn->specular = specularProp;
      }

      specularProp->color = dae_colorOrTex(dst, xml, specularProp);
      dae_colorDescTextureUsage(dst, specularProp->color,
                                AK_TEXTURE_COLORSPACE_SRGB,
                                AK_TEXTURE_CHANNEL_RGB);
    } else if (DAE_XML_TAG_EQ(xml, reflective)) {
      if (!techn->reflective)
        techn->reflective = ak_heap_calloc(heap, techn, sizeof(*techn->reflective));
      techn->reflective->color = dae_colorOrTex(dst, xml, techn);
      dae_colorDescTextureUsage(dst, techn->reflective->color,
                                AK_TEXTURE_COLORSPACE_SRGB,
                                AK_TEXTURE_CHANNEL_RGB);
    } else if (DAE_XML_TAG_EQ(xml, transparent)) {
      if (!techn->transparent) {
        transp             = ak_heap_calloc(heap, techn, sizeof(*transp));
        transp->amount     = 1.0f;
        techn->transparent = transp;
      }
      
      if ((att = DAE_XMLA8(xml, opaque)))
        opaque = dae_opaque(att);
      else
        opaque = AK_OPAQUE_A_ONE;
      
      techn->transparent->color  = dae_colorOrTex(dst, xml, techn);
      techn->transparent->opaque = opaque;
      dae_colorDescTextureUsage(dst, techn->transparent->color,
                                AK_TEXTURE_COLORSPACE_SRGB,
                                dae_transparentTextureChannels(opaque));
    } else if (DAE_XML_TAG_EQ(xml, shininess)) {
      AkMaterialSpecularProp *specularProp;

      if (!(specularProp = techn->specular)) {
        specularProp    = ak_heap_calloc(heap, techn, sizeof(*specularProp));
        techn->specular = specularProp;
      }

      specularProp->strength = dae_float(dst, xml, specularProp, 
                                         offsetof(AkMaterialSpecularProp, shininess), 1.0f);
    } else if (DAE_XML_TAG_EQ(xml, reflectivity)) {
      if (!techn->reflective)
        techn->reflective = ak_heap_calloc(heap, techn, sizeof(*techn->reflective));
      techn->reflective->amount = dae_float(dst, xml, techn->reflective, 
                                            offsetof(AkReflective, amount), 0.0f);
    } else if (DAE_XML_TAG_EQ(xml, transparency)) {
      if (!techn->transparent) {
        transp             = ak_heap_calloc(heap, techn, sizeof(*transp));
        transp->amount     = 1.0f;
        techn->transparent = transp;
      }
      techn->transparent->amount = dae_float(dst, xml, techn->transparent,
                                             offsetof(AkTransparent, amount), 1.0f);

      /* some old version of tools e.g. SketchUp exports incorrect */
      if (ak_opt_get(AK_OPT_BUGFIXES))
        dae_bugfix_transp(techn->transparent);
    } else if (DAE_XML_TAG_EQ(xml, index_of_refraction)) {
      /* TODO: assumed 0.0 for COLLADA */
      techn->ior = dae_float(dst, xml, techn,
                             offsetof(AkTechniqueFxCommon, ior), 0.0f);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      dae_techniqueFxSceneKitExtra(dst, xml, techn);
    }
    xml = xml->next;
  }

  return techn;
}
