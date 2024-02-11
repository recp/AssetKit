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
void
gltf_materials(json_t * __restrict jmaterial,
               void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *jmaterials;
  AkLibrary          *libmat;
  bool                specGlossExt;

  if (!(jmaterials = json_array(jmaterial)))
    return;

  gst          = userdata;
  jmaterial    = jmaterials->base.value;

  specGlossExt = ak_opt_get(AK_OPT_GLTF_EXT_SPEC_GLOSS);
  heap         = gst->heap;
  doc          = gst->doc;
  libmat       = ak_heap_calloc(heap, doc, sizeof(*libmat));

  while (jmaterial) {
    json_t                 *jmatVal, *jext;
    AkProfileCommon        *pcommon;
    AkTechniqueFx          *technfx;
    AkTechniqueFxCommon    *cmnTechn;
    AkMaterial             *mat;
    AkEffect               *effect;
    AkInstanceEffect       *ieff;
    float                   cutoff;
    bool                    hasSpecGlossExt;

    pcommon         = gltf_cmnEffect(gst);
    effect          = ak_mem_parent(pcommon);
    technfx         = ak_heap_calloc(heap, pcommon, sizeof(*technfx));
    mat             = ak_heap_calloc(heap, libmat,  sizeof(*mat));
    cmnTechn        = ak_heap_calloc(heap, technfx, sizeof(*cmnTechn));;
    cutoff          = 0.5f;
    hasSpecGlossExt = false;

    pcommon->technique = technfx;

    ak_setypeid(technfx, AKT_TECHNIQUE_FX);

    cmnTechn->type = AK_MATERIAL_PBR;

    jmatVal = jmaterial->value;

    if (specGlossExt) {
      if ((jext = json_get(jmaterial, _s_gltf_extensions))) {
        json_t *jspecGloss, *jspecgVal;
        if ((jspecGloss = json_get(jext, _s_gltf_ext_pbrSpecGloss))) {
          AkMaterialSpecularProp *specularProp;

          specularProp             = ak_heap_calloc(heap, cmnTechn, sizeof(*specularProp));
          cmnTechn->specular       = specularProp;

          if (!cmnTechn->albedo) {
            cmnTechn->diffuse = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->diffuse));
          }
          cmnTechn->diffuse->color = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->diffuse->color));

          glm_vec4_copy(GLM_VEC4_ONE, cmnTechn->diffuse->color->vec);
          glm_vec4_copy(GLM_VEC4_ONE, specularProp->colorFactor.vec);

          jspecgVal = jspecGloss->value;
          while (jspecgVal) {
            if (json_key_eq(jspecgVal, _s_gltf_diffuseFactor)) {
              json_array_float(cmnTechn->diffuse->color->vec, jspecgVal, 0.0f, 4, true);
            } else if (json_key_eq(jspecgVal, _s_gltf_specFactor)) {
              json_array_float(specularProp->colorFactor.vec, jspecgVal, 0.0f, 3, true);
              specularProp->colorFactor.vec[3] = 1.0f;
            } else if (json_key_eq(jspecgVal, _s_gltf_glossFactor)) {
              specularProp->strength = json_float(jspecgVal, 0.0f);
            } else if (json_key_eq(jspecgVal, _s_gltf_diffuseTexture)) {
              cmnTechn->diffuse->texture = gltf_texref(gst, cmnTechn, jspecGloss);
            } else if (json_key_eq(jspecgVal, _s_gltf_specGlossTex)) {
              specularProp->specularTex = gltf_texref(gst, cmnTechn, jspecGloss);
            }
            jspecgVal = jspecgVal->next;
          } /* jspecGlossVal */

          hasSpecGlossExt = true;
        } /* _s_gltf_ext_pbrSpecGloss */
      } /* _s_gltf_extensions */
    } /* specGlossExt */

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
      } else if (json_key_eq(jmatVal, _s_gltf_emissiveTex)) {
        /* Emission Map */
        AkColorDesc *colorDesc;

        colorDesc          = ak_heap_calloc(heap, technfx, sizeof(*colorDesc));
        colorDesc->texture = gltf_texref(gst, colorDesc, jmatVal);
        cmnTechn->emission = colorDesc;

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
        /* alphaMode */
        AkOpaque opaque;

        opaque = gltf_alphaMode(jmatVal);

        if (opaque != AK_OPAQUE_OPAQUE) {
          AkTransparent *transp;

          transp         = ak_heap_calloc(heap, cmnTechn, sizeof(*transp));
          transp->amount = ak_def_transparency();
          transp->opaque = opaque;
          transp->cutoff = 0.5f;

          cmnTechn->transparent = transp;
        }
      } else if (json_key_eq(jmatVal, _s_gltf_alphaCutoff)) {
        /* alphaCutoff */
        cutoff = json_float(jmatVal, 0.5f);
      }

      jmatVal = jmatVal->next;
    }
    
    /* alphaCutoff */
    if (cmnTechn->transparent)
      cmnTechn->transparent->cutoff = cutoff;

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

  doc->lib.materials = libmat;
}
