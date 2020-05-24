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

static
_assetkit_hide
AkTechniqueFxCommon*
dae_techniqueFxCmn(DAEState * __restrict dst,
                   xml_t    * __restrict xml,
                   void     * __restrict memp,
                   AkMaterialType        mattype);

_assetkit_hide
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
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, techn, NULL);
    } else if ((xml_tag_eq(xml, _s_dae_phong)   && (m = AK_MATERIAL_PHONG))
           || (xml_tag_eq(xml, _s_dae_blinn)    && (m = AK_MATERIAL_BLINN))
           || (xml_tag_eq(xml, _s_dae_lambert)  && (m = AK_MATERIAL_LAMBERT))
           || (xml_tag_eq(xml, _s_dae_constant) && (m = AK_MATERIAL_CONSTANT))) {
      techn->common = dae_techniqueFxCmn(dst, xml, techn, m);
    } else if (dst->version < AK_COLLADA_VERSION_150
               && xml_tag_eq(xml, _s_dae_image)) {
      /* migration from 1.4 */
      dae14_fxMigrateImg(dst, xml, NULL);
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      techn->extra = tree_fromxml(heap, techn, xml);
    }
    xml = xml->next;
  }

  return techn;
}

static
_assetkit_hide
AkTechniqueFxCommon*
dae_techniqueFxCmn(DAEState * __restrict dst,
                   xml_t    * __restrict xml,
                   void     * __restrict memp,
                   AkMaterialType        mattype) {
  AkHeap              *heap;
  AkTechniqueFxCommon *techn;
  xml_attr_t          *att;
  AkReflective        *refl;
  AkTransparent       *transp;
  AkOpaque             opaque;

  heap        = dst->heap;
  techn       = ak_heap_calloc(heap, memp, sizeof(*techn));
  techn->type = mattype;
  xml         = xml->val;

  while (xml) {
    if (xml_tag_eq(xml, _s_dae_emission)) {
      techn->emission = dae_colorOrTex(dst, xml, techn);
    } else if (xml_tag_eq(xml, _s_dae_ambient)) {
      techn->ambient = dae_colorOrTex(dst, xml, techn);
    } else if (xml_tag_eq(xml, _s_dae_diffuse)) {
      techn->diffuse = dae_colorOrTex(dst, xml, techn);
    } else if (xml_tag_eq(xml, _s_dae_specular)) {
      techn->specular = dae_colorOrTex(dst, xml, techn);
    } else if (xml_tag_eq(xml, _s_dae_reflective)) {
      if (!techn->reflective)
        techn->reflective = ak_heap_calloc(heap, techn, sizeof(*refl));

      techn->reflective->color = dae_colorOrTex(dst, xml, techn);
    } else if (xml_tag_eq(xml, _s_dae_transparent)) {
      if (!techn->transparent) {
        transp             = ak_heap_calloc(heap, techn, sizeof(*transp));
        transp->amount     = ak_def_transparency();
        techn->transparent = transp;
      }
      
      if ((att = xmla(xml, _s_dae_opaque)))
        opaque = dae_opaque(att);
      else
        opaque = AK_OPAQUE_A_ONE;
      
      techn->transparent->color  = dae_colorOrTex(dst, xml, techn);
      techn->transparent->opaque = opaque;
    } else if (xml_tag_eq(xml, _s_dae_shininess)) {
      techn->shininess = dae_floatOrParam(dst, xml, techn);
    } else if (xml_tag_eq(xml, _s_dae_reflectivity)) {
      if (!techn->reflective)
        techn->reflective = ak_heap_calloc(heap, techn, sizeof(*refl));
      techn->reflective->amount = dae_floatOrParam(dst, xml, techn);
    } else if (xml_tag_eq(xml, _s_dae_transparency)) {
      if (!techn->transparent) {
        transp             = ak_heap_calloc(heap, techn, sizeof(*transp));
        transp->amount     = ak_def_transparency();
        techn->transparent = transp;
      }
      techn->transparent->amount = dae_floatOrParam(dst, xml, techn);
      
      /* some old version of tools e.g. SketchUp exports incorrect */
      if (ak_opt_get(AK_OPT_BUGFIXES))
        dae_bugfix_transp(techn->transparent);
    } else if (xml_tag_eq(xml, _s_dae_index_of_refraction)) {
      techn->indexOfRefraction = dae_floatOrParam(dst, xml, techn);
    }
    xml = xml->next;
  }

  return techn;
}
