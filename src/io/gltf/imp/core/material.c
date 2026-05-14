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

typedef struct AkGLTFColorDescBlock {
  AkColorDesc desc;
  AK_ALIGN(16) AkColor color;
} AkGLTFColorDescBlock;

static
AkColorDesc*
gltf_materialColorDesc(AkGLTFState * __restrict gst,
                       void        * __restrict parent,
                       float                    r,
                       float                    g,
                       float                    b,
                       float                    a);

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

  cmnTechn->albedo        = gltf_materialColorDesc(gst, cmnTechn,
                                                   1.0f, 1.0f, 1.0f, 1.0f);

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
  AkGLTFColorDescBlock *block;

  block       = ak_heap_calloc(gst->heap, parent, sizeof(*block));
  desc        = &block->desc;
  desc->color = &block->color;

  desc->color->vec[0] = r;
  desc->color->vec[1] = g;
  desc->color->vec[2] = b;
  desc->color->vec[3] = a;

  return desc;
}

static
AkTextureRef*
gltf_materialTexRef(AkGLTFState        * __restrict gst,
                    void               * __restrict parent,
                    json_t             * __restrict jtexinfo,
                    AkTextureColorSpace             colorSpace,
                    AkTextureChannels               channels) {
  return ak_texref_usage(gltf_texref(gst, parent, jtexinfo),
                         colorSpace,
                         channels);
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
    if (GLTF_JSON_KEY_EQ(jval, specularFactor)) {
      specularProp->strength = json_float(jval, 1.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, specularTexture)) {
      specularProp->specularTex = gltf_materialTexRef(gst,
                                                       cmnTechn,
                                                       jval,
                                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                                       AK_TEXTURE_CHANNEL_A);
      specularProp->textureChannels = AK_TEXTURE_CHANNEL_A;
    } else if (GLTF_JSON_KEY_EQ(jval, specularColorFactor)) {
      json_array_float(specularProp->color->color->vec, jval, 0.0f, 3, true);
      specularProp->color->color->vec[3] = 1.0f;
    } else if (GLTF_JSON_KEY_EQ(jval, specularColorTexture)) {
      specularProp->color->texture = gltf_materialTexRef(gst,
                                                          cmnTechn,
                                                          jval,
                                                          AK_TEXTURE_COLORSPACE_SRGB,
                                                          AK_TEXTURE_CHANNEL_RGB);
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
    if (GLTF_JSON_KEY_EQ(jval, clearcoatFactor)) {
      clearcoat->intensity = json_float(jval, 0.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, clearcoatTexture)) {
      clearcoat->texture = gltf_materialTexRef(gst,
                                                cmnTechn,
                                                jval,
                                                AK_TEXTURE_COLORSPACE_LINEAR,
                                                AK_TEXTURE_CHANNEL_R);
      clearcoat->textureChannels = AK_TEXTURE_CHANNEL_R;
    } else if (GLTF_JSON_KEY_EQ(jval, clearcoatRoughnessFactor)) {
      clearcoat->roughness = json_float(jval, 0.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, clearcoatRoughnessTexture)) {
      clearcoat->roughnessTexture = gltf_materialTexRef(gst,
                                                         cmnTechn,
                                                         jval,
                                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                                         AK_TEXTURE_CHANNEL_G);
      clearcoat->roughnessTextureChannels = AK_TEXTURE_CHANNEL_G;
    } else if (GLTF_JSON_KEY_EQ(jval, clearcoatNormalTexture)) {
      clearcoat->normalTexture = gltf_materialTexRef(gst,
                                                      cmnTechn,
                                                      jval,
                                                      AK_TEXTURE_COLORSPACE_LINEAR,
                                                      AK_TEXTURE_CHANNEL_RGB);
      clearcoat->normalScale   = json_float(GLTF_JSON_GET8(jval, scale),
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
    if (GLTF_JSON_KEY_EQ(jval, transmissionFactor)) {
      transmissionProp->factor = json_float(jval, 0.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, transmissionTexture)) {
      transmissionProp->texture = gltf_materialTexRef(gst,
                                                       cmnTechn,
                                                       jval,
                                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                                       AK_TEXTURE_CHANNEL_R);
      transmissionProp->textureChannels = AK_TEXTURE_CHANNEL_R;
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
    if (GLTF_JSON_KEY_EQ(jval, sheenColorFactor)) {
      json_array_float(sheen->color->color->vec, jval, 0.0f, 3, true);
      sheen->color->color->vec[3] = 1.0f;
    } else if (GLTF_JSON_KEY_EQ(jval, sheenColorTexture)) {
      sheen->color->texture = gltf_materialTexRef(gst,
                                                   sheen,
                                                   jval,
                                                   AK_TEXTURE_COLORSPACE_SRGB,
                                                   AK_TEXTURE_CHANNEL_RGB);
    } else if (GLTF_JSON_KEY_EQ(jval, sheenRoughnessFactor)) {
      sheen->roughness = json_float(jval, 0.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, sheenRoughnessTexture)) {
      sheen->roughnessTexture = gltf_materialTexRef(gst,
                                                     sheen,
                                                     jval,
                                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                                     AK_TEXTURE_CHANNEL_A);
      sheen->roughnessTextureChannels = AK_TEXTURE_CHANNEL_A;
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
    if (GLTF_JSON_KEY_EQ(jval, iridescenceFactor)) {
      iri->factor = json_float(jval, 0.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceTexture)) {
      iri->texture = gltf_materialTexRef(gst,
                                          iri,
                                          jval,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_R);
      iri->textureChannels = AK_TEXTURE_CHANNEL_R;
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceIor)) {
      iri->ior = json_float(jval, 1.3f);
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceThicknessMinimum)) {
      iri->thicknessMinimum = json_float(jval, 100.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceThicknessMaximum)) {
      iri->thicknessMaximum = json_float(jval, 400.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceThicknessTexture)) {
      iri->thicknessTexture = gltf_materialTexRef(gst,
                                                   iri,
                                                   jval,
                                                   AK_TEXTURE_COLORSPACE_LINEAR,
                                                   AK_TEXTURE_CHANNEL_G);
      iri->thicknessTextureChannels = AK_TEXTURE_CHANNEL_G;
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
    if (GLTF_JSON_KEY_EQ(jval, thicknessFactor)) {
      vol->thicknessFactor = json_float(jval, 0.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, thicknessTexture)) {
      vol->thicknessTexture = gltf_materialTexRef(gst,
                                                  vol,
                                                  jval,
                                                  AK_TEXTURE_COLORSPACE_LINEAR,
                                                  AK_TEXTURE_CHANNEL_G);
      vol->thicknessTextureChannels = AK_TEXTURE_CHANNEL_G;
    } else if (GLTF_JSON_KEY_EQ(jval, attenuationDistance)) {
      vol->attenuationDistance = json_float(jval, INFINITY);
    } else if (GLTF_JSON_KEY_EQ(jval, attenuationColor)) {
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
    if (GLTF_JSON_KEY_EQ(jval, anisotropyStrength)) {
      aniso->strength = json_float(jval, 0.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, anisotropyRotation)) {
      aniso->rotation = json_float(jval, 0.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, anisotropyTexture)) {
      aniso->texture = gltf_materialTexRef(gst,
                                            aniso,
                                            jval,
                                            AK_TEXTURE_COLORSPACE_LINEAR,
                                            AK_TEXTURE_CHANNEL_RGB);
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

  disp->dispersion = json_float(GLTF_JSON_GET(jspec, dispersion), 0.0f);
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
    if (GLTF_JSON_KEY_EQ(jval, diffuseTransmissionFactor)) {
      dt->factor = json_float(jval, 0.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, diffuseTransmissionTexture)) {
      dt->texture = gltf_materialTexRef(gst,
                                         dt,
                                         jval,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_A);
      dt->textureChannels = AK_TEXTURE_CHANNEL_A;
    } else if (GLTF_JSON_KEY_EQ(jval, diffuseTransmissionColorFactor)) {
      json_array_float(dt->color->color->vec, jval, 1.0f, 3, true);
      dt->color->color->vec[3] = 1.0f;
    } else if (GLTF_JSON_KEY_EQ(jval, diffuseTransmissionColorTexture)) {
      dt->color->texture = gltf_materialTexRef(gst,
                                                dt,
                                                jval,
                                                AK_TEXTURE_COLORSPACE_SRGB,
                                                AK_TEXTURE_CHANNEL_RGB);
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
  size_t              materialIndex;

  gst          = userdata;
  heap         = gst->heap;
  doc          = gst->doc;
  libmat       = ak_heap_calloc(heap, doc, sizeof(*libmat));
  doc->lib.materials = libmat;

  gst->defaultMaterial = gltf_default_mat(gst, libmat);

  if (!(jmaterials = json_array(jmaterial)))
    return;

  gst->materialsCount   = jmaterials->count;
  gst->materialsByIndex = ak_heap_calloc(heap,
                                         gst->tmpParent,
                                         sizeof(*gst->materialsByIndex)
                                         * gst->materialsCount);
  materialIndex = gst->materialsCount;
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
    jext    = gltf_jsonGetLen(jmaterial, _s_gltf_extensions, 10);
    gltf_extra(gst,
               mat,
               GLTF_JSON_GET8(jmaterial, extras),
               jext);

    if (jext) {
      json_t *jspec, *jval;

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_specular)))
        gltf_materialParseSpecular(gst, cmnTechn, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_clearcoat)))
        gltf_materialParseClearcoat(gst, cmnTechn, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_unlit)))
        cmnTechn->type = AK_MATERIAL_CONSTANT;

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_emissive_strength))) {
        AkMaterialEmissionProp *emission;

        if (!(emission = cmnTechn->emission)) {
          emission           = ak_heap_calloc(heap, cmnTechn, sizeof(*emission));
          cmnTechn->emission = emission;
        }

        emission->strength = json_float(GLTF_JSON_GET(jspec, emissiveStrength), 1.0f);
      }

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_ior)))
        cmnTechn->ior = json_float(GLTF_JSON_GET8(jspec, ior), 1.5f);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_transmission)))
        gltf_materialParseTransmission(gst, cmnTechn, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_sheen)))
        gltf_materialParseSheen(gst, cmnTechn, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_iridescence)))
        gltf_materialParseIridescence(gst, cmnTechn, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_volume)))
        gltf_materialParseVolume(gst, cmnTechn, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_anisotropy)))
        gltf_materialParseAnisotropy(gst, cmnTechn, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_dispersion)))
        gltf_materialParseDispersion(gst, cmnTechn, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_diffuse_transmission)))
        gltf_materialParseDiffuseTransmission(gst, cmnTechn, jspec);

      /* ARCHIVED: Superseded by KHR_materials_specular */
      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_pbrSpecularGlossiness))) {
        AkMaterialSpecularProp *specularProp;
        AkColorDesc            *specularColor;

        specularProp           = ak_heap_calloc(heap, cmnTechn, sizeof(*specularProp));
        specularProp->strength = 1.0f;
        cmnTechn->specular     = specularProp;
        cmnTechn->type         = AK_MATERIAL_SPECULAR_GLOSSINES;
        specularColor        = gltf_materialColorDesc(gst,
                                                       specularProp,
                                                       1.0f, 1.0f, 1.0f, 1.0f);
        specularProp->color  = specularColor;

        if (!cmnTechn->albedo) {
          cmnTechn->diffuse = gltf_materialColorDesc(gst,
                                                      cmnTechn,
                                                      1.0f, 1.0f, 1.0f, 1.0f);
        }
        jval = jspec->value;
        while (jval) {
          if (gltf_jsonKeyEqLen(jval, _s_gltf_diffuseFactor, 13)) {
            json_array_float(cmnTechn->diffuse->color->vec, jval, 0.0f, 4, true);
          } else if (gltf_jsonKeyEqLen(jval, _s_gltf_specFactor, 14)) {
            json_array_float(specularColor->color->vec, jval, 0.0f, 3, true);
            specularColor->color->vec[3] = 1.0f;
          } else if (gltf_jsonKeyEqLen(jval, _s_gltf_glossFactor, 16)) {
            specularProp->strength = json_float(jval, 1.0f);
          } else if (gltf_jsonKeyEqLen(jval, _s_gltf_diffuseTexture, 14)) {
            cmnTechn->diffuse->texture = gltf_materialTexRef(gst,
                                                              cmnTechn,
                                                              jval,
                                                              AK_TEXTURE_COLORSPACE_SRGB,
                                                              AK_TEXTURE_CHANNEL_RGBA);
          } else if (gltf_jsonKeyEqLen(jval, _s_gltf_specGlossTex, 25)) {
            specularProp->specularTex = gltf_materialTexRef(gst,
                                                             cmnTechn,
                                                             jval,
                                                             AK_TEXTURE_COLORSPACE_SRGB,
                                                             AK_TEXTURE_CHANNEL_RGBA);
          }
          jval = jval->next;
        }
      }
    } /* _s_gltf_extensions */

    while (jmatVal) {
      /* Metallic Roughness */
      if (GLTF_JSON_KEY_EQ8(jmatVal, name)) {
        mat->name = json_strdup(jmatVal, heap, mat);
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_pbrMetalRough, 20)) {
        AkMaterialMetallicProp *metalness, *roughness;
        json_t *jmrVal;

        metalness               = ak_heap_calloc(heap, cmnTechn, sizeof(*metalness));
        roughness               = ak_heap_calloc(heap, cmnTechn, sizeof(*roughness));

        metalness->intensity    = 1.0f;
        roughness->intensity    = 1.0f;

        cmnTechn->metalness     = metalness;
        cmnTechn->roughness     = roughness;

        if (!cmnTechn->albedo) {
          cmnTechn->albedo = gltf_materialColorDesc(gst,
                                                     cmnTechn,
                                                     1.0f, 1.0f, 1.0f, 1.0f);
        }

        jmrVal = jmatVal->value;
        while (jmrVal) {
          if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_baseColor, 15)) {
            json_array_float(cmnTechn->albedo->color->vec, jmrVal,  0.0f, 4, true);
          } else if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_metalFac, 14)) {
            metalness->intensity = json_float(jmrVal, 0.0f);
          } else if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_roughFac, 15)) {
            roughness->intensity = json_float(jmrVal, 0.0f);
          } else if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_metalRoughTex, 24)) {
            metalness->tex = gltf_materialTexRef(gst,
                                                  metalness,
                                                  jmrVal,
                                                  AK_TEXTURE_COLORSPACE_LINEAR,
                                                  AK_TEXTURE_CHANNEL_GB);
            metalness->textureChannels = AK_TEXTURE_CHANNEL_B;
            roughness->tex = metalness->tex;
            roughness->textureChannels = AK_TEXTURE_CHANNEL_G;
          } else if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_baseColorTex, 16)) {
            cmnTechn->albedo->texture = gltf_materialTexRef(gst,
                                                             cmnTechn->albedo,
                                                             jmrVal,
                                                             AK_TEXTURE_COLORSPACE_SRGB,
                                                             AK_TEXTURE_CHANNEL_RGBA);
          }

          jmrVal = jmrVal->next;
        }
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_emissiveFac, 14)) {
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
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_emissiveTex, 15)) {
        AkMaterialEmissionProp *emission;

        if (!(emission = cmnTechn->emission)) {
          emission           = ak_heap_calloc(heap, technfx, sizeof(*emission));
          emission->strength = 1.0f;
          cmnTechn->emission = emission;
        }

        emission->color.texture = gltf_materialTexRef(gst,
                                                       emission,
                                                       jmatVal,
                                                       AK_TEXTURE_COLORSPACE_SRGB,
                                                       AK_TEXTURE_CHANNEL_RGB);
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_occlusionTex, 16)) {
        /* Occlusion Map */
        AkOcclusion *occl;

        occl           = ak_heap_calloc(heap, technfx, sizeof(*occl));
        occl->tex      = gltf_materialTexRef(gst,
                                              occl,
                                              jmatVal,
                                              AK_TEXTURE_COLORSPACE_LINEAR,
                                              AK_TEXTURE_CHANNEL_R);
        occl->strength = json_float(GLTF_JSON_GET8(jmatVal, strength), 1.0f);
        occl->textureChannels = AK_TEXTURE_CHANNEL_R;

        cmnTechn->occlusion = occl;

      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_normalTex, 13)) {
        /* Normap Map */
        AkNormalMap *normal;

        normal        = ak_heap_calloc(heap, technfx, sizeof(*normal));
        normal->tex   = gltf_materialTexRef(gst,
                                             normal,
                                             jmatVal,
                                             AK_TEXTURE_COLORSPACE_LINEAR,
                                             AK_TEXTURE_CHANNEL_RGB);
        normal->scale = json_float(GLTF_JSON_GET8(jmatVal, scale), 1.0f);

        cmnTechn->normal = normal;

      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_doubleSided, 11)) {
        /* doubleSided */
        cmnTechn->doubleSided = json_bool(jmatVal, 0);
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_alphaMode, 9)) {
        AkTransparent *transp;

        if (!(transp = cmnTechn->transparent)) {
          transp                = ak_heap_calloc(heap, cmnTechn, sizeof(*transp));
          transp->amount        = 1.0f;
          transp->cutoff        = 0.5f;
          transp->opaque        = AK_OPAQUE_OPAQUE;
          cmnTechn->transparent = transp;
        }

        transp->opaque = gltf_alphaMode(jmatVal);
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_alphaCutoff, 11)) {
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
    if (materialIndex > 0)
      gst->materialsByIndex[--materialIndex] = mat;

    jmaterial = jmaterial->next;
  }
}
