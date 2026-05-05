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
#include "../extra.h"
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
  cmnTechn->emission           = ak_heap_calloc(heap, technfx, sizeof(*cmnTechn->emission));
  colorDesc                    = &cmnTechn->emission->color;
  colorDesc->color             = ak_heap_calloc(heap, colorDesc, sizeof(*colorDesc->color));
  colorDesc->color->vec[3]     = 1.0f;
  cmnTechn->emission->strength = 1.0f;

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

static
AkColorDesc*
gltf_materialColorDesc(AkGLTFState * __restrict gst,
                       void        * __restrict parent,
                       float                    r,
                       float                    g,
                       float                    b,
                       float                    a) {
  AkColorDesc *desc;

  desc        = ak_heap_calloc(gst->heap, parent, sizeof(*desc));
  desc->color = ak_heap_calloc(gst->heap, desc, sizeof(*desc->color));

  desc->color->vec[0] = r;
  desc->color->vec[1] = g;
  desc->color->vec[2] = b;
  desc->color->vec[3] = a;

  return desc;
}

static
void
gltf_materialParseSpecular(AkGLTFState         * __restrict gst,
                           AkTechniqueFxCommon * __restrict cmnTechn,
                           json_t              * __restrict jspec) {
  AkMaterialSpecularProp *specularProp;
  json_t                 *jval;

  if (!(specularProp = cmnTechn->specular)) {
    specularProp           = ak_heap_calloc(gst->heap, cmnTechn,
                                            sizeof(*specularProp));
    specularProp->strength = 1.0f;
    specularProp->color    = gltf_materialColorDesc(gst, specularProp,
                                                    1.0f, 1.0f, 1.0f, 1.0f);
    cmnTechn->specular     = specularProp;
  }

  jval = jspec->value;
  while (jval) {
    if (json_key_eq(jval, _s_gltf_specularFactor)) {
      specularProp->strength = json_float(jval, 1.0f);
    } else if (json_key_eq(jval, _s_gltf_specularTexture)) {
      specularProp->specularTex = gltf_texref(gst, cmnTechn, jval);
    } else if (json_key_eq(jval, _s_gltf_specularColorFactor)) {
      json_array_float(specularProp->color->color->vec, jval, 0.0f, 3, true);
      specularProp->color->color->vec[3] = 1.0f;
    } else if (json_key_eq(jval, _s_gltf_specularColorTexture)) {
      specularProp->color->texture = gltf_texref(gst, cmnTechn, jval);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseClearcoat(AkGLTFState         * __restrict gst,
                            AkTechniqueFxCommon * __restrict cmnTechn,
                            json_t              * __restrict jspec) {
  AkMaterialClearcoat *clearcoat;
  json_t              *jval;

  if (!(clearcoat = cmnTechn->clearcoat)) {
    clearcoat           = ak_heap_calloc(gst->heap, cmnTechn,
                                         sizeof(*clearcoat));
    clearcoat->normalScale = 1.0f;
    cmnTechn->clearcoat = clearcoat;
  }

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
      clearcoat->normalScale   = json_float(json_get(jval, _s_gltf_scale),
                                            1.0f);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseTransmission(AkGLTFState         * __restrict gst,
                               AkTechniqueFxCommon * __restrict cmnTechn,
                               json_t              * __restrict jspec) {
  AkMaterialTransmissionProp *transmissionProp;
  json_t                     *jval;

  if (!(transmissionProp = cmnTechn->transmission)) {
    transmissionProp       = ak_heap_calloc(gst->heap, cmnTechn,
                                            sizeof(*transmissionProp));
    cmnTechn->transmission = transmissionProp;
  }

  jval = jspec->value;
  while (jval) {
    if (json_key_eq(jval, _s_gltf_transmissionFactor)) {
      transmissionProp->factor = json_float(jval, 0.0f);
    } else if (json_key_eq(jval, _s_gltf_transmissionTexture)) {
      transmissionProp->texture = gltf_texref(gst, cmnTechn, jval);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseSheen(AkGLTFState         * __restrict gst,
                        AkTechniqueFxCommon * __restrict cmnTechn,
                        json_t              * __restrict jspec) {
  AkMaterialSheen *sheen;
  json_t          *jval;

  if (!(sheen = cmnTechn->sheen)) {
    sheen          = ak_heap_calloc(gst->heap, cmnTechn, sizeof(*sheen));
    sheen->color   = gltf_materialColorDesc(gst, sheen,
                                            0.0f, 0.0f, 0.0f, 1.0f);
    cmnTechn->sheen = sheen;
  }

  jval = jspec->value;
  while (jval) {
    if (json_key_eq(jval, _s_gltf_sheenColorFactor)) {
      json_array_float(sheen->color->color->vec, jval, 0.0f, 3, true);
      sheen->color->color->vec[3] = 1.0f;
    } else if (json_key_eq(jval, _s_gltf_sheenColorTexture)) {
      sheen->color->texture = gltf_texref(gst, sheen, jval);
    } else if (json_key_eq(jval, _s_gltf_sheenRoughnessFactor)) {
      sheen->roughness = json_float(jval, 0.0f);
    } else if (json_key_eq(jval, _s_gltf_sheenRoughnessTexture)) {
      sheen->roughnessTexture = gltf_texref(gst, sheen, jval);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseIridescence(AkGLTFState         * __restrict gst,
                              AkTechniqueFxCommon * __restrict cmnTechn,
                              json_t              * __restrict jspec) {
  AkMaterialIridescence *iri;
  json_t                *jval;

  if (!(iri = cmnTechn->iridescence)) {
    iri                   = ak_heap_calloc(gst->heap, cmnTechn, sizeof(*iri));
    iri->ior              = 1.3f;
    iri->thicknessMinimum = 100.0f;
    iri->thicknessMaximum = 400.0f;
    cmnTechn->iridescence = iri;
  }

  jval = jspec->value;
  while (jval) {
    if (json_key_eq(jval, _s_gltf_iridescenceFactor)) {
      iri->factor = json_float(jval, 0.0f);
    } else if (json_key_eq(jval, _s_gltf_iridescenceTexture)) {
      iri->texture = gltf_texref(gst, iri, jval);
    } else if (json_key_eq(jval, _s_gltf_iridescenceIor)) {
      iri->ior = json_float(jval, 1.3f);
    } else if (json_key_eq(jval, _s_gltf_iridescenceThicknessMinimum)) {
      iri->thicknessMinimum = json_float(jval, 100.0f);
    } else if (json_key_eq(jval, _s_gltf_iridescenceThicknessMaximum)) {
      iri->thicknessMaximum = json_float(jval, 400.0f);
    } else if (json_key_eq(jval, _s_gltf_iridescenceThicknessTexture)) {
      iri->thicknessTexture = gltf_texref(gst, iri, jval);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseVolume(AkGLTFState         * __restrict gst,
                         AkTechniqueFxCommon * __restrict cmnTechn,
                         json_t              * __restrict jspec) {
  AkMaterialVolume *vol;
  json_t           *jval;

  if (!(vol = cmnTechn->volume)) {
    vol = ak_heap_calloc(gst->heap, cmnTechn, sizeof(*vol));
    vol->attenuationColor.vec[0] = 1.0f;
    vol->attenuationColor.vec[1] = 1.0f;
    vol->attenuationColor.vec[2] = 1.0f;
    vol->attenuationColor.vec[3] = 1.0f;
    vol->attenuationDistance = INFINITY;
    cmnTechn->volume = vol;
  }

  jval = jspec->value;
  while (jval) {
    if (json_key_eq(jval, _s_gltf_thicknessFactor)) {
      vol->thicknessFactor = json_float(jval, 0.0f);
    } else if (json_key_eq(jval, _s_gltf_thicknessTexture)) {
      vol->thicknessTexture = gltf_texref(gst, vol, jval);
    } else if (json_key_eq(jval, _s_gltf_attenuationDistance)) {
      vol->attenuationDistance = json_float(jval, INFINITY);
    } else if (json_key_eq(jval, _s_gltf_attenuationColor)) {
      json_array_float(vol->attenuationColor.vec, jval, 1.0f, 3, true);
      vol->attenuationColor.vec[3] = 1.0f;
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseAnisotropy(AkGLTFState         * __restrict gst,
                             AkTechniqueFxCommon * __restrict cmnTechn,
                             json_t              * __restrict jspec) {
  AkMaterialAnisotropy *aniso;
  json_t               *jval;

  if (!(aniso = cmnTechn->anisotropy)) {
    aniso                 = ak_heap_calloc(gst->heap, cmnTechn,
                                           sizeof(*aniso));
    cmnTechn->anisotropy  = aniso;
  }

  jval = jspec->value;
  while (jval) {
    if (json_key_eq(jval, _s_gltf_anisotropyStrength)) {
      aniso->strength = json_float(jval, 0.0f);
    } else if (json_key_eq(jval, _s_gltf_anisotropyRotation)) {
      aniso->rotation = json_float(jval, 0.0f);
    } else if (json_key_eq(jval, _s_gltf_anisotropyTexture)) {
      aniso->texture = gltf_texref(gst, aniso, jval);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseDispersion(AkGLTFState         * __restrict gst,
                             AkTechniqueFxCommon * __restrict cmnTechn,
                             json_t              * __restrict jspec) {
  AkMaterialDispersion *disp;

  if (!(disp = cmnTechn->dispersion)) {
    disp                 = ak_heap_calloc(gst->heap, cmnTechn, sizeof(*disp));
    cmnTechn->dispersion = disp;
  }

  disp->dispersion = json_float(json_get(jspec, _s_gltf_dispersion), 0.0f);
}

static
void
gltf_materialParseDiffuseTransmission(AkGLTFState         * __restrict gst,
                                      AkTechniqueFxCommon * __restrict cmnTechn,
                                      json_t              * __restrict jspec) {
  AkMaterialDiffuseTransmission *dt;
  json_t                        *jval;

  if (!(dt = cmnTechn->diffuseTransmission)) {
    dt        = ak_heap_calloc(gst->heap, cmnTechn, sizeof(*dt));
    dt->color = gltf_materialColorDesc(gst, dt, 1.0f, 1.0f, 1.0f, 1.0f);
    cmnTechn->diffuseTransmission = dt;
  }

  jval = jspec->value;
  while (jval) {
    if (json_key_eq(jval, _s_gltf_diffuseTransmissionFactor)) {
      dt->factor = json_float(jval, 0.0f);
    } else if (json_key_eq(jval, _s_gltf_diffuseTransmissionTexture)) {
      dt->texture = gltf_texref(gst, dt, jval);
    } else if (json_key_eq(jval, _s_gltf_diffuseTransmissionColorFactor)) {
      json_array_float(dt->color->color->vec, jval, 1.0f, 3, true);
      dt->color->color->vec[3] = 1.0f;
    } else if (json_key_eq(jval, _s_gltf_diffuseTransmissionColorTexture)) {
      dt->color->texture = gltf_texref(gst, dt, jval);
    }
    jval = jval->next;
  }
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

  gst          = userdata;
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
    cmnTechn->ior  = 1.5f;

    jmatVal = jmaterial->value;
    gltf_extra(gst,
               mat,
               json_get(jmaterial, _s_gltf_extras),
               json_get(jmaterial, _s_gltf_extensions));

    if ((jext = json_get(jmaterial, _s_gltf_extensions))) {
      json_t *jspec, *jval;

      if ((jspec = json_get(jext, _s_gltf_ext_KHR_materials_specular)))
        gltf_materialParseSpecular(gst, cmnTechn, jspec);

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_clearcoat)))
        gltf_materialParseClearcoat(gst, cmnTechn, jspec);

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_unlit)))
        cmnTechn->type = AK_MATERIAL_CONSTANT;

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_emissive_strength))) {
        AkMaterialEmissionProp *emission;

        if (!(emission = cmnTechn->emission)) {
          emission           = ak_heap_calloc(heap, cmnTechn, sizeof(*emission));
          cmnTechn->emission = emission;
        }

        emission->strength = json_float(json_get(jspec, _s_gltf_emissiveStrength), 1.0f);
      }

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_ior)))
        cmnTechn->ior = json_float(json_get(jspec, _s_gltf_ior), 1.5f);

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_transmission)))
        gltf_materialParseTransmission(gst, cmnTechn, jspec);

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_sheen)))
        gltf_materialParseSheen(gst, cmnTechn, jspec);

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_iridescence)))
        gltf_materialParseIridescence(gst, cmnTechn, jspec);

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_volume)))
        gltf_materialParseVolume(gst, cmnTechn, jspec);

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_anisotropy)))
        gltf_materialParseAnisotropy(gst, cmnTechn, jspec);

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_dispersion)))
        gltf_materialParseDispersion(gst, cmnTechn, jspec);

      if ((jspec = json_get(jext, _s_gltf_KHR_materials_diffuse_transmission)))
        gltf_materialParseDiffuseTransmission(gst, cmnTechn, jspec);

      /* ARCHIVED: Superseded by KHR_materials_specular */
      if ((jspec = json_get(jext, _s_gltf_ext_pbrSpecGloss))) {
        AkMaterialSpecularProp *specularProp;
        AkColorDesc            *specularColor;

        specularProp         = ak_heap_calloc(heap, cmnTechn, sizeof(*specularProp));
        cmnTechn->specular   = specularProp;
        specularColor        = ak_heap_calloc(heap, specularProp, sizeof(*specularColor));
        specularColor->color = ak_heap_calloc(heap, specularColor, sizeof(*specularColor->color));
        specularProp->color  = specularColor;

        if (!cmnTechn->albedo) {
          cmnTechn->diffuse = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->diffuse));
        }
        cmnTechn->diffuse->color = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->diffuse->color));

        glm_vec4_copy(GLM_VEC4_ONE, cmnTechn->diffuse->color->vec);
        glm_vec4_copy(GLM_VEC4_ONE, specularColor->color->vec);

        jval = jspec->value;
        while (jval) {
          if (json_key_eq(jval, _s_gltf_diffuseFactor)) {
            json_array_float(cmnTechn->diffuse->color->vec, jval, 0.0f, 4, true);
          } else if (json_key_eq(jval, _s_gltf_specFactor)) {
            json_array_float(specularColor->color->vec, jval, 0.0f, 3, true);
            specularColor->color->vec[3] = 1.0f;
          } else if (json_key_eq(jval, _s_gltf_glossFactor)) {
            specularProp->strength = json_float(jval, 0.0f);
          } else if (json_key_eq(jval, _s_gltf_diffuseTexture)) {
            cmnTechn->diffuse->texture = gltf_texref(gst, cmnTechn, jval);
          } else if (json_key_eq(jval, _s_gltf_specGlossTex)) {
            specularProp->specularTex = gltf_texref(gst, cmnTechn, jval);
          }
          jval = jval->next;
        }
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
        AkMaterialEmissionProp *emission;
        AkColor                *color;

        if (!(emission = cmnTechn->emission)) {
          emission           = ak_heap_calloc(heap, technfx, sizeof(*emission));
          emission->strength = 1.0f;
          cmnTechn->emission = emission;
        }

        if (!(color = emission->color.color)) {
          emission->color.color = color = ak_heap_calloc(heap, emission, sizeof(*color));
        }

        json_array_float(color->vec, jmatVal, 0.0f, 3, true);
        color->vec[3] = 1.0f;
      } else if (json_key_eq(jmatVal, _s_gltf_emissiveTex)) {
        AkMaterialEmissionProp *emission;

        if (!(emission = cmnTechn->emission)) {
          emission           = ak_heap_calloc(heap, technfx, sizeof(*emission));
          emission->strength = 1.0f;
          cmnTechn->emission = emission;
        }

        emission->color.texture = gltf_texref(gst, emission, jmatVal);
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
