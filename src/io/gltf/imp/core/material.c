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

#include "material.h"
#include "profile.h"
#include "sampler.h"
#include "texture.h"
#include "enum.h"
#include "../../../../default/material.h"

AK_HIDE
AkMaterial*
gltf_default_mat(AkGLTFState *gst, AkLibrary *libmat) {
  AkHeap                 *heap;
  AkInstanceEffect       *ieff;
  AkEffect               *effect;
  AkProfileCommon        *pcommon;
  AkTechniqueFx          *technfx;
  AkTechniqueFxCommon    *cmnTechn;
  AkMaterial             *mat;
  AkMaterialMetallicProp *metalness, *roughness;
  AkColorDesc            *colorDesc;
  AkTransparent          *transp;

  heap               = gst->heap;
  pcommon            = gltf_cmnEffect(gst);
  effect             = ak_mem_parent(pcommon);
  technfx            = ak_heap_calloc(heap, pcommon, sizeof(*technfx));
  mat                = ak_heap_calloc(heap, libmat,  sizeof(*mat));
  cmnTechn           = ak_heap_calloc(heap, technfx, sizeof(*cmnTechn));;
  pcommon->technique = technfx;

  ak_setypeid(technfx, AKT_TECHNIQUE_FX);

  cmnTechn->type = AK_MATERIAL_PBR;

  metalness               = ak_heap_calloc(heap, cmnTechn, sizeof(*metalness));
  roughness               = ak_heap_calloc(heap, cmnTechn, sizeof(*roughness));

  metalness->intensity    = 1.0f;
  roughness->intensity    = 1.0f;

  cmnTechn->metalness     = metalness;
  cmnTechn->roughness     = roughness;

  cmnTechn->albedo        = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->albedo));
  cmnTechn->albedo->color = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->albedo->color));

  /* DEFAULT value by spec */
  glm_vec4_copy(GLM_VEC4_ONE, cmnTechn->albedo->color->vec);

  /* emissive */
  colorDesc                = ak_heap_calloc(heap, technfx, sizeof(*colorDesc));
  colorDesc->color         = ak_heap_calloc(heap, colorDesc, sizeof(*colorDesc->color));
  colorDesc->color->vec[3] = 1.0f;
  cmnTechn->emission       = colorDesc;

  /* transparent */
  transp                = ak_heap_calloc(heap, cmnTechn, sizeof(*transp));
  transp->amount        = 1.0f;
  transp->opaque        = AK_OPAQUE_OPAQUE;
  transp->cutoff        = 0.5f;
  cmnTechn->transparent = transp;

  technfx->common    = cmnTechn;
  ieff               = ak_heap_calloc(heap, mat, sizeof(*ieff));
  ieff->base.type    = AK_INSTANCE_EFFECT;
  ieff->base.url.ptr = effect;
  mat->effect        = ieff;

  return mat;
}

AK_HIDE
void
gltf_materials(json_t * __restrict jmaterial,
               void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jmaterials;
  AkLibrary          *libmat;
  bool                specGlossExt;

  gst          = userdata;
  specGlossExt = ak_opt_get(AK_OPT_GLTF_EXT_SPEC_GLOSS);
  heap         = gst->heap;
  doc          = gst->doc;
  libmat       = ak_heap_calloc(heap, doc, sizeof(*libmat));
  doc->lib.materials = libmat;

  gst->defaultMaterial = gltf_default_mat(gst, libmat);

  if (!(jmaterials = json_array(jmaterial)))
    return;

  jmaterial = jmaterials->base.value;
  while (jmaterial) {
    json_t                 *jmatVal, *jext;
    AkProfileCommon        *pcommon;
    AkTechniqueFx          *technfx;
    AkTechniqueFxCommon    *cmnTechn;
    AkMaterial             *mat;
    AkEffect               *effect;
    AkInstanceEffect       *ieff;

    pcommon            = gltf_cmnEffect(gst);
    effect             = ak_mem_parent(pcommon);
    technfx            = ak_heap_calloc(heap, pcommon, sizeof(*technfx));
    mat                = ak_heap_calloc(heap, libmat,  sizeof(*mat));
    cmnTechn           = ak_heap_calloc(heap, technfx, sizeof(*cmnTechn));;
    pcommon->technique = technfx;

    ak_setypeid(technfx, AKT_TECHNIQUE_FX);

    cmnTechn->type = AK_MATERIAL_PBR;

    jmatVal = jmaterial->value;

    if (specGlossExt) {
      if ((jext = json_get(jmaterial, _s_gltf_extensions))) {
        json_t *jspecGloss, *jspecgVal;
        if ((jspecGloss = json_get(jext, _s_gltf_ext_pbrSpecGloss))) {
          AkMaterialSpecularProp *specularProp;
          AkColorDesc            *specularColor;

          specularProp             = ak_heap_calloc(heap, cmnTechn, sizeof(*specularProp));
          cmnTechn->specular       = specularProp;
          specularColor            = ak_heap_calloc(heap, specularProp, sizeof(*specularColor));
          specularColor->color     = ak_heap_calloc(heap, specularColor, sizeof(*specularColor->color));

          if (!cmnTechn->albedo) {
            cmnTechn->diffuse = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->diffuse));
          }
          cmnTechn->diffuse->color = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->diffuse->color));

          glm_vec4_copy(GLM_VEC4_ONE, cmnTechn->diffuse->color->vec);
          glm_vec4_copy(GLM_VEC4_ONE, specularColor->color->vec);

          jspecgVal = jspecGloss->value;
          while (jspecgVal) {
            if (json_key_eq(jspecgVal, _s_gltf_diffuseFactor)) {
              json_array_float(cmnTechn->diffuse->color->vec, jspecgVal, 0.0f, 4, true);
            } else if (json_key_eq(jspecgVal, _s_gltf_specFactor)) {
              json_array_float(specularColor->color->vec, jspecgVal, 0.0f, 3, true);
              specularColor->color->vec[3] = 1.0f;
            } else if (json_key_eq(jspecgVal, _s_gltf_glossFactor)) {
              specularProp->strength = json_float(jspecgVal, 0.0f);
            } else if (json_key_eq(jspecgVal, _s_gltf_diffuseTexture)) {
              cmnTechn->diffuse->texture = gltf_texref(gst, cmnTechn, jspecgVal);
            } else if (json_key_eq(jspecgVal, _s_gltf_specGlossTex)) {
              specularProp->specularTex = gltf_texref(gst, cmnTechn, jspecgVal);
            }
            jspecgVal = jspecgVal->next;
          } /* jspecGlossVal */
        } /* _s_gltf_ext_pbrSpecGloss */
      } /* _s_gltf_extensions */
    } /* specGlossExt */

    if ((jext = json_get(jmaterial, _s_gltf_extensions))) {
      json_t *jspec, *jval;
      if ((jspec = json_get(jext, _s_gltf_ext_KHR_materials_specular))) {
        AkMaterialSpecularProp *specularProp;
        AkColorDesc            *specularColor;

        specularProp         = ak_heap_calloc(heap, cmnTechn, sizeof(*specularProp));
        cmnTechn->specular   = specularProp;
        specularColor        = ak_heap_calloc(heap, specularProp, sizeof(*specularColor));
        specularColor->color = ak_heap_calloc(heap, specularColor, sizeof(*specularColor->color));

        glm_vec4_copy(GLM_VEC4_ONE, specularColor->color->vec);

        jval = jspec->value;
        while (jval) {
          if (json_key_eq(jval, _s_gltf_specularFactor)) {
            specularProp->strength = json_float(jval, 1.0f);
          } else if (json_key_eq(jval, _s_gltf_specularTexture)) {
            specularProp->specularTex = gltf_texref(gst, cmnTechn, jval);
          } else if (json_key_eq(jval, _s_gltf_specularColorFactor)) {
            json_array_float(specularColor->color->vec, jval, 0.0f, 3, true);
            specularColor->color->vec[3] = 1.0f;
          } else if (json_key_eq(jval, _s_gltf_specularColorTexture)) {
            specularColor->texture = gltf_texref(gst, cmnTechn, jval);
          }
          jval = jval->next;
        }
      } else if ((jspec = json_get(jext, _s_gltf_KHR_materials_clearcoat))) {
        AkMaterialClearcoat *clearcoat;
        clearcoat = ak_heap_calloc(heap, cmnTechn, sizeof(*clearcoat));

        jval = jspec->value;
        while (jval) {
          if (json_key_eq(jval, _s_gltf_clearcoatFactor)) {
            clearcoat->intensity = json_float(jval, 0.0f);
          } else if (json_key_eq(jval, _s_gltf_clearcoatTexture)) {
            clearcoat->texture = gltf_texref(gst, cmnTechn, jval);
          } else if (json_key_eq(jval, _s_gltf_clearcoatRoughnessFactor)) {
            clearcoat->roughness = json_float(jval, 0.0f);
          } else if (json_key_eq(jval, _s_gltf_clearcoatRoughnessTexture)) {
            clearcoat->roughnessTexture = gltf_texref(gst, cmnTechn, jval);
          } else if (json_key_eq(jval, _s_gltf_clearcoatNormalTexture)) {
            clearcoat->normalTexture = gltf_texref(gst, cmnTechn, jval);
          }
          jval = jval->next;
        }

        cmnTechn->clearcoat = clearcoat;
      } else if ((jspec = json_get(jext, _s_gltf_KHR_materials_unlit))) {
        cmnTechn->type = AK_MATERIAL_CONSTANT;
      }
    } /* _s_gltf_extensions */

    while (jmatVal) {
      /* Metallic Roughness */
      if (json_key_eq(jmatVal, _s_gltf_pbrMetalRough)) {
        AkMaterialMetallicProp *metalness, *roughness;
        json_t *jmrVal;

        metalness               = ak_heap_calloc(heap, cmnTechn, sizeof(*metalness));
        roughness               = ak_heap_calloc(heap, cmnTechn, sizeof(*roughness));

        metalness->intensity    = 1.0f;
        roughness->intensity    = 1.0f;

        cmnTechn->metalness     = metalness;
        cmnTechn->roughness     = roughness;

        if (!cmnTechn->albedo) {
          cmnTechn->albedo = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->albedo));
        }

        cmnTechn->albedo->color = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->albedo->color));

        /* DEFAULT value by spec */
        glm_vec4_copy(GLM_VEC4_ONE, cmnTechn->albedo->color->vec);

        jmrVal = jmatVal->value;
        while (jmrVal) {
          if (json_key_eq(jmrVal, _s_gltf_baseColor)) {
            json_array_float(cmnTechn->albedo->color->vec, jmrVal,  0.0f, 4, true);
          } else if (json_key_eq(jmrVal, _s_gltf_metalFac)) {
            metalness->intensity = json_float(jmrVal, 0.0f);
          } else if (json_key_eq(jmrVal, _s_gltf_roughFac)) {
            roughness->intensity = json_float(jmrVal, 0.0f);
          } else if (json_key_eq(jmrVal, _s_gltf_metalRoughTex)) {
            metalness->tex = gltf_texref(gst, metalness, jmrVal);
            roughness->tex = metalness->tex;
          } else if (json_key_eq(jmrVal, _s_gltf_baseColorTex)) {
            cmnTechn->albedo->texture = gltf_texref(gst, cmnTechn->albedo, jmrVal);
          }

          jmrVal = jmrVal->next;
        }
      } else if (json_key_eq(jmatVal, _s_gltf_emissiveFac)) {
        AkColorDesc *colorDesc;

        if (!(colorDesc = cmnTechn->emission)) {
          colorDesc          = ak_heap_calloc(heap, technfx, sizeof(*colorDesc));
          cmnTechn->emission = colorDesc;
        }

        colorDesc->color = ak_heap_calloc(heap, colorDesc, sizeof(*colorDesc->color));
        json_array_float(colorDesc->color->vec, jmatVal, 0.0f, 3, true);
        colorDesc->color->vec[3] = 1.0f;
      } else if (json_key_eq(jmatVal, _s_gltf_emissiveTex)) {
        /* Emission Map */
        AkColorDesc *colorDesc;

        if (!(colorDesc = cmnTechn->emission)) {
          colorDesc          = ak_heap_calloc(heap, technfx, sizeof(*colorDesc));
          cmnTechn->emission = colorDesc;
        }

        colorDesc->texture = gltf_texref(gst, colorDesc, jmatVal);
      } else if (json_key_eq(jmatVal, _s_gltf_occlusionTex)) {
        /* Occlusion Map */
        AkOcclusion *occl;

        occl           = ak_heap_calloc(heap, technfx, sizeof(*occl));
        occl->tex      = gltf_texref(gst, occl, jmatVal);
        occl->strength = json_float(json_get(jmatVal, _s_gltf_strength), 1.0f);

        cmnTechn->occlusion = occl;

      } else if (json_key_eq(jmatVal, _s_gltf_normalTex)) {
        /* Normap Map */
        AkNormalMap *normal;

        normal        = ak_heap_calloc(heap, technfx, sizeof(*normal));
        normal->tex   = gltf_texref(gst, normal, jmatVal);
        normal->scale = json_float(json_get(jmatVal, _s_gltf_scale), 1.0f);

        cmnTechn->normal = normal;

      } else if (json_key_eq(jmatVal, _s_gltf_doubleSided)) {
        /* doubleSided */
        cmnTechn->doubleSided = json_bool(jmatVal, 0);
      } else if (json_key_eq(jmatVal, _s_gltf_alphaMode)) {
        AkTransparent *transp;

        if (!(transp = cmnTechn->transparent)) {
          transp                = ak_heap_calloc(heap, cmnTechn, sizeof(*transp));
          transp->amount        = 1.0f;
          transp->cutoff        = 0.5f;
          transp->opaque        = AK_OPAQUE_OPAQUE;
          cmnTechn->transparent = transp;
        }

        transp->opaque = gltf_alphaMode(jmatVal);
      } else if (json_key_eq(jmatVal, _s_gltf_alphaCutoff)) {
        AkTransparent *transp;

        if (!(transp = cmnTechn->transparent)) {
          transp                = ak_heap_calloc(heap, cmnTechn, sizeof(*transp));
          transp->amount        = 1.0f;
          transp->cutoff        = 0.5f;
          transp->opaque        = AK_OPAQUE_OPAQUE;
          cmnTechn->transparent = transp;
        }

        transp->cutoff = json_float(jmatVal, 0.5f);
      }

      jmatVal = jmatVal->next;
    }

    technfx->common    = cmnTechn;
    ieff               = ak_heap_calloc(heap, mat, sizeof(*ieff));
    ieff->base.type    = AK_INSTANCE_EFFECT;
    ieff->base.url.ptr = effect;
    mat->effect        = ieff;

    mat->base.next     = libmat->chld;
    libmat->chld       = (void *)mat;
    libmat->count++;

    jmaterial = jmaterial->next;
  }
}
