/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "techn_cmn.h"
#include "colortex.h"
#include "fltprm.h"
#include "enum.h"
#include "../bugfix/transp.h"
#include "../../default/def_material.h"

AkTechniqueFxCommon* _assetkit_hide
dae_phong(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap              *heap;
  AkTechniqueFxCommon *techn;
  xml_attr_t          *att;
  AkReflective        *refl;
  AkTransparent       *transp;
  AkOpaque             opaque;

  heap  = dst->heap;
  techn = ak_heap_calloc(heap, memp, sizeof(*techn));
  xml   = xml->val;

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
    xml = xml->val;
  }

  return techn;
}
