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
#include "sampler.h"
#include "texture.h"
#include "enum.h"
#include "../extra.h"
#include "../../../../strpool.h"

static
void
gltf_materialFeaturePush(AkMaterialSurface * __restrict surface,
                         AkMaterialFeature * __restrict feature) {
  if (!surface || !feature)
    return;

  feature->next     = surface->features;
  surface->features = feature;
  if ((uint32_t)feature->type < 32)
    surface->featureMask |= 1u << (uint32_t)feature->type;
}

static
AkMaterialFeature*
gltf_materialEnsureFeature(AkGLTFState          * __restrict gst,
                           AkMaterialSurface    * __restrict surface,
                           AkMaterialFeatureType             type,
                           size_t                            size) {
  AkMaterialFeature *feature;

  if ((feature = ak_materialFeature(surface, type)))
    return feature;

  feature       = ak_heap_calloc(gst->heap, surface, size);
  feature->type = type;
  gltf_materialFeaturePush(surface, feature);

  return feature;
}

static
AkMaterialClearcoatFeature*
gltf_materialEnsureClearcoat(AkGLTFState       * __restrict gst,
                             AkMaterialSurface * __restrict surface) {
  AkMaterialClearcoatFeature *feature;

  feature = (void*)ak_materialFeature(surface, AK_MATERIAL_FEATURE_CLEARCOAT);
  if (feature)
    return feature;

  feature = ak_heap_calloc(gst->heap, surface, sizeof(*feature));
  feature->base.type = AK_MATERIAL_FEATURE_CLEARCOAT;
  feature->normalScale = 1.0f;
  gltf_materialFeaturePush(surface, &feature->base);

  return feature;
}

static
AkMaterialIridescenceFeature*
gltf_materialEnsureIridescence(AkGLTFState       * __restrict gst,
                               AkMaterialSurface * __restrict surface) {
  AkMaterialIridescenceFeature *feature;

  feature = (void*)ak_materialFeature(surface,
                                      AK_MATERIAL_FEATURE_IRIDESCENCE);
  if (feature)
    return feature;

  feature = ak_heap_calloc(gst->heap, surface, sizeof(*feature));
  feature->base.type = AK_MATERIAL_FEATURE_IRIDESCENCE;
  feature->ior = 1.3f;
  feature->thicknessMinimum = 100.0f;
  feature->thicknessMaximum = 400.0f;
  gltf_materialFeaturePush(surface, &feature->base);

  return feature;
}

static
AkMaterialVolumeFeature*
gltf_materialEnsureVolume(AkGLTFState       * __restrict gst,
                          AkMaterialSurface * __restrict surface) {
  AkMaterialVolumeFeature *feature;

  feature = (void*)ak_materialFeature(surface, AK_MATERIAL_FEATURE_VOLUME);
  if (feature)
    return feature;

  feature = ak_heap_calloc(gst->heap, surface, sizeof(*feature));
  feature->base.type               = AK_MATERIAL_FEATURE_VOLUME;
  feature->attenuationColor.vec[0] = 1.0f;
  feature->attenuationColor.vec[1] = 1.0f;
  feature->attenuationColor.vec[2] = 1.0f;
  feature->attenuationColor.vec[3] = 1.0f;
  feature->attenuationDistance     = INFINITY;
  gltf_materialFeaturePush(surface, &feature->base);

  return feature;
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
AkMaterialInput*
gltf_materialInput(AkGLTFState          * __restrict gst,
                   void                 * __restrict parent,
                   AkMaterialInput     ** __restrict slot,
                   const char           * __restrict semantic,
                   AkMaterialInputValue              valueType,
                   AkTextureColorSpace               colorSpace,
                   AkTextureChannels                 channels) {
  AkMaterialInput *input;

  if (!(input = *slot)) {
    input             = ak_heap_calloc(gst->heap, parent, sizeof(*input));
    input->semantic   = semantic;
    input->source     = AK_MATERIAL_INPUT_CONSTANT;
    input->valueType  = valueType;
    input->colorSpace = colorSpace;
    input->channels   = channels;
    *slot = input;
  }

  input->colorSpace = colorSpace;
  if (channels != AK_TEXTURE_CHANNEL_NONE)
    input->channels = channels;

  return input;
}

static
AkMaterialInput*
gltf_materialScalar(AkGLTFState          * __restrict gst,
                    void                 * __restrict parent,
                    AkMaterialInput     ** __restrict slot,
                    const char           * __restrict semantic,
                    float                             value,
                    AkTextureRef         * __restrict texture,
                    AkTextureColorSpace               colorSpace,
                    AkTextureChannels                 channels) {
  AkMaterialInput *input;

  input = gltf_materialInput(gst,
                             parent,
                             slot,
                             semantic,
                             AK_MATERIAL_VALUE_FLOAT,
                             colorSpace,
                             channels);
  input->value[0] = value;
  if (texture) {
    input->texture = texture;
    input->source  = AK_MATERIAL_INPUT_TEXTURE;
  }

  return input;
}

static
AkMaterialInput*
gltf_materialColor(AkGLTFState          * __restrict gst,
                   void                 * __restrict parent,
                   AkMaterialInput     ** __restrict slot,
                   const char           * __restrict semantic,
                   float                             r,
                   float                             g,
                   float                             b,
                   float                             a,
                   AkTextureRef         * __restrict texture,
                   AkTextureColorSpace               colorSpace,
                   AkTextureChannels                 channels) {
  AkMaterialInput *input;

  input = gltf_materialInput(gst,
                             parent,
                             slot,
                             semantic,
                             AK_MATERIAL_VALUE_COLOR,
                             colorSpace,
                             channels);
  input->color.vec[0] = r;
  input->color.vec[1] = g;
  input->color.vec[2] = b;
  input->color.vec[3] = a;

  if (texture) {
    input->texture = texture;
    input->source  = AK_MATERIAL_INPUT_TEXTURE;
  }

  return input;
}

static
void
gltf_materialInitSurface(AkGLTFState       * __restrict gst,
                         AkMaterialSurface * __restrict surface) {
  surface->type = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->alphaCutoff = 0.5f;
  surface->ior = 1.5f;
  surface->emissiveStrength = 1.0f;

  gltf_materialColor(gst,
                     surface,
                     &surface->baseColor,
                     ak_materialSemanticName(AK_MATERIAL_SEMANTIC_BASE_COLOR),
                     1.0f, 1.0f, 1.0f, 1.0f,
                     NULL,
                     AK_TEXTURE_COLORSPACE_SRGB,
                     AK_TEXTURE_CHANNEL_RGBA);
  gltf_materialScalar(gst,
                      surface,
                      &surface->metallic,
                      ak_materialSemanticName(AK_MATERIAL_SEMANTIC_METALLIC),
                      1.0f,
                      NULL,
                      AK_TEXTURE_COLORSPACE_LINEAR,
                      AK_TEXTURE_CHANNEL_B);
  gltf_materialScalar(gst,
                      surface,
                      &surface->roughness,
                      ak_materialSemanticName(AK_MATERIAL_SEMANTIC_ROUGHNESS),
                      1.0f,
                      NULL,
                      AK_TEXTURE_COLORSPACE_LINEAR,
                      AK_TEXTURE_CHANNEL_G);
}

AK_HIDE
AkMaterial*
gltf_default_mat(AkGLTFState *gst) {
  AkMaterial        *mat;
  AkMaterialSurface *surface;

  mat     = ak_heap_calloc(gst->heap, gst->doc, sizeof(*mat));
  ak_setypeid(mat, AKT_MATERIAL);
  surface = ak_heap_calloc(gst->heap, mat,    sizeof(*surface));
  gltf_materialInitSurface(gst, surface);
  mat->surface = surface;

  return mat;
}

static
void
gltf_materialParseSpecular(AkGLTFState       * __restrict gst,
                           AkMaterialSurface * __restrict surface,
                           json_t            * __restrict jspec) {
  AkMaterialSpecularFeature *specular;
  json_t                    *jval;

  specular = (void*)gltf_materialEnsureFeature(gst,
                                               surface,
                                               AK_MATERIAL_FEATURE_SPECULAR,
                                               sizeof(*specular));

  jval = jspec->value;
  while (jval) {
    if (GLTF_JSON_KEY_EQ(jval, specularFactor)) {
      gltf_materialScalar(gst,
                          specular,
                          &specular->factor,
                          _s_ak_specularFactor,
                          json_float(jval, 1.0f),
                          specular->factor ? specular->factor->texture : NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_A);
    } else if (GLTF_JSON_KEY_EQ(jval, specularTexture)) {
      gltf_materialScalar(gst,
                          specular,
                          &specular->factor,
                          _s_ak_specularFactor,
                          specular->factor ? specular->factor->value[0] : 1.0f,
                          gltf_materialTexRef(gst,
                                               specular,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_A),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_A);
    } else if (GLTF_JSON_KEY_EQ(jval, specularColorFactor)) {
      AkMaterialInput *input;
      input = gltf_materialColor(gst,
                                 specular,
                                 &specular->color,
                                 _s_ak_specularColor,
                                 1.0f, 1.0f, 1.0f, 1.0f,
                                 specular->color ? specular->color->texture : NULL,
                                 AK_TEXTURE_COLORSPACE_SRGB,
                                 AK_TEXTURE_CHANNEL_RGB);
      json_array_float(input->color.vec, jval, 0.0f, 3, true);
      input->color.vec[3] = 1.0f;
    } else if (GLTF_JSON_KEY_EQ(jval, specularColorTexture)) {
      gltf_materialColor(gst,
                         specular,
                         &specular->color,
                         _s_ak_specularColor,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         gltf_materialTexRef(gst,
                                              specular,
                                              jval,
                                              AK_TEXTURE_COLORSPACE_SRGB,
                                              AK_TEXTURE_CHANNEL_RGB),
                         AK_TEXTURE_COLORSPACE_SRGB,
                         AK_TEXTURE_CHANNEL_RGB);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseClearcoat(AkGLTFState       * __restrict gst,
                            AkMaterialSurface * __restrict surface,
                            json_t            * __restrict jspec) {
  AkMaterialClearcoatFeature *clearcoat;
  json_t                     *jval;

  clearcoat = gltf_materialEnsureClearcoat(gst, surface);

  jval = jspec->value;
  while (jval) {
    if (GLTF_JSON_KEY_EQ(jval, clearcoatFactor)) {
      gltf_materialScalar(gst,
                          clearcoat,
                          &clearcoat->factor,
                          _s_ak_clearcoat,
                          json_float(jval, 0.0f),
                          clearcoat->factor ? clearcoat->factor->texture : NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_R);
    } else if (GLTF_JSON_KEY_EQ(jval, clearcoatTexture)) {
      gltf_materialScalar(gst,
                          clearcoat,
                          &clearcoat->factor,
                          _s_ak_clearcoat,
                          clearcoat->factor ? clearcoat->factor->value[0] : 0.0f,
                          gltf_materialTexRef(gst,
                                               clearcoat,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_R),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_R);
    } else if (GLTF_JSON_KEY_EQ(jval, clearcoatRoughnessFactor)) {
      gltf_materialScalar(gst,
                          clearcoat,
                          &clearcoat->roughness,
                          _s_ak_clearcoatRoughness,
                          json_float(jval, 0.0f),
                          clearcoat->roughness
                            ? clearcoat->roughness->texture
                            : NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_G);
    } else if (GLTF_JSON_KEY_EQ(jval, clearcoatRoughnessTexture)) {
      gltf_materialScalar(gst,
                          clearcoat,
                          &clearcoat->roughness,
                          _s_ak_clearcoatRoughness,
                          clearcoat->roughness
                            ? clearcoat->roughness->value[0]
                            : 0.0f,
                          gltf_materialTexRef(gst,
                                               clearcoat,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_G),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_G);
    } else if (GLTF_JSON_KEY_EQ(jval, clearcoatNormalTexture)) {
      clearcoat->normalScale = json_float(GLTF_JSON_GET8(jval, scale), 1.0f);
      gltf_materialScalar(gst,
                          clearcoat,
                          &clearcoat->normal,
                          _s_ak_clearcoatNormal,
                          clearcoat->normalScale,
                          gltf_materialTexRef(gst,
                                               clearcoat,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_RGB),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_RGB);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseTransmission(AkGLTFState       * __restrict gst,
                               AkMaterialSurface * __restrict surface,
                               json_t            * __restrict jspec) {
  AkMaterialTransmissionFeature *transmission;
  json_t                        *jval;

  transmission = (void*)gltf_materialEnsureFeature(
                         gst,
                         surface,
                         AK_MATERIAL_FEATURE_TRANSMISSION,
                         sizeof(*transmission));

  jval = jspec->value;
  while (jval) {
    if (GLTF_JSON_KEY_EQ(jval, transmissionFactor)) {
      gltf_materialScalar(gst,
                          transmission,
                          &transmission->factor,
                          _s_ak_transmission,
                          json_float(jval, 0.0f),
                          transmission->factor
                            ? transmission->factor->texture
                            : NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_R);
    } else if (GLTF_JSON_KEY_EQ(jval, transmissionTexture)) {
      gltf_materialScalar(gst,
                          transmission,
                          &transmission->factor,
                          _s_ak_transmission,
                          transmission->factor
                            ? transmission->factor->value[0]
                            : 0.0f,
                          gltf_materialTexRef(gst,
                                               transmission,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_R),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_R);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseSheen(AkGLTFState       * __restrict gst,
                        AkMaterialSurface * __restrict surface,
                        json_t            * __restrict jspec) {
  AkMaterialSheenFeature *sheen;
  json_t                 *jval;

  sheen = (void*)gltf_materialEnsureFeature(gst,
                                            surface,
                                            AK_MATERIAL_FEATURE_SHEEN,
                                            sizeof(*sheen));

  jval = jspec->value;
  while (jval) {
    if (GLTF_JSON_KEY_EQ(jval, sheenColorFactor)) {
      AkMaterialInput *input;
      input = gltf_materialColor(gst,
                                 sheen,
                                 &sheen->color,
                                 _s_ak_sheenColor,
                                 0.0f, 0.0f, 0.0f, 1.0f,
                                 sheen->color ? sheen->color->texture : NULL,
                                 AK_TEXTURE_COLORSPACE_SRGB,
                                 AK_TEXTURE_CHANNEL_RGB);
      json_array_float(input->color.vec, jval, 0.0f, 3, true);
      input->color.vec[3] = 1.0f;
    } else if (GLTF_JSON_KEY_EQ(jval, sheenColorTexture)) {
      gltf_materialColor(gst,
                         sheen,
                         &sheen->color,
                         _s_ak_sheenColor,
                         0.0f, 0.0f, 0.0f, 1.0f,
                         gltf_materialTexRef(gst,
                                              sheen,
                                              jval,
                                              AK_TEXTURE_COLORSPACE_SRGB,
                                              AK_TEXTURE_CHANNEL_RGB),
                         AK_TEXTURE_COLORSPACE_SRGB,
                         AK_TEXTURE_CHANNEL_RGB);
    } else if (GLTF_JSON_KEY_EQ(jval, sheenRoughnessFactor)) {
      gltf_materialScalar(gst,
                          sheen,
                          &sheen->roughness,
                          _s_ak_sheenRoughness,
                          json_float(jval, 0.0f),
                          sheen->roughness ? sheen->roughness->texture : NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_A);
    } else if (GLTF_JSON_KEY_EQ(jval, sheenRoughnessTexture)) {
      gltf_materialScalar(gst,
                          sheen,
                          &sheen->roughness,
                          _s_ak_sheenRoughness,
                          sheen->roughness ? sheen->roughness->value[0] : 0.0f,
                          gltf_materialTexRef(gst,
                                               sheen,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_A),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_A);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseIridescence(AkGLTFState       * __restrict gst,
                              AkMaterialSurface * __restrict surface,
                              json_t            * __restrict jspec) {
  AkMaterialIridescenceFeature *iri;
  json_t                       *jval;

  iri = gltf_materialEnsureIridescence(gst, surface);

  jval = jspec->value;
  while (jval) {
    if (GLTF_JSON_KEY_EQ(jval, iridescenceFactor)) {
      gltf_materialScalar(gst,
                          iri,
                          &iri->factor,
                          _s_ak_iridescence,
                          json_float(jval, 0.0f),
                          iri->factor ? iri->factor->texture : NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_R);
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceTexture)) {
      gltf_materialScalar(gst,
                          iri,
                          &iri->factor,
                          _s_ak_iridescence,
                          iri->factor ? iri->factor->value[0] : 0.0f,
                          gltf_materialTexRef(gst,
                                               iri,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_R),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_R);
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceIor)) {
      iri->ior = json_float(jval, 1.3f);
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceThicknessMinimum)) {
      iri->thicknessMinimum = json_float(jval, 100.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceThicknessMaximum)) {
      iri->thicknessMaximum = json_float(jval, 400.0f);
    } else if (GLTF_JSON_KEY_EQ(jval, iridescenceThicknessTexture)) {
      gltf_materialScalar(gst,
                          iri,
                          &iri->thickness,
                          _s_ak_iridescenceThickness,
                          iri->thicknessMaximum,
                          gltf_materialTexRef(gst,
                                               iri,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_G),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_G);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseVolume(AkGLTFState       * __restrict gst,
                         AkMaterialSurface * __restrict surface,
                         json_t            * __restrict jspec) {
  AkMaterialVolumeFeature *volume;
  json_t                  *jval;

  volume = gltf_materialEnsureVolume(gst, surface);

  jval = jspec->value;
  while (jval) {
    if (GLTF_JSON_KEY_EQ(jval, thicknessFactor)) {
      gltf_materialScalar(gst,
                          volume,
                          &volume->thickness,
                          _s_ak_thickness,
                          json_float(jval, 0.0f),
                          volume->thickness ? volume->thickness->texture : NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_G);
    } else if (GLTF_JSON_KEY_EQ(jval, thicknessTexture)) {
      gltf_materialScalar(gst,
                          volume,
                          &volume->thickness,
                          _s_ak_thickness,
                          volume->thickness ? volume->thickness->value[0] : 0.0f,
                          gltf_materialTexRef(gst,
                                               volume,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_G),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_G);
    } else if (GLTF_JSON_KEY_EQ(jval, attenuationDistance)) {
      volume->attenuationDistance = json_float(jval, INFINITY);
    } else if (GLTF_JSON_KEY_EQ(jval, attenuationColor)) {
      json_array_float(volume->attenuationColor.vec, jval, 1.0f, 3, true);
      volume->attenuationColor.vec[3] = 1.0f;
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseVolumeScatter(AkGLTFState       * __restrict gst,
                                AkMaterialSurface * __restrict surface,
                                json_t            * __restrict jspec) {
  AkMaterialSubsurfaceFeature *subsurface;
  json_t                      *jval;

  subsurface = (void*)gltf_materialEnsureFeature(gst,
                                                 surface,
                                                 AK_MATERIAL_FEATURE_SUBSURFACE,
                                                 sizeof(*subsurface));

  jval = jspec->value;
  while (jval) {
    if (GLTF_JSON_KEY_EQ(jval, multiscatterColor)
        || GLTF_JSON_KEY_EQ(jval, multiscatterColorFactor)) {
      AkMaterialInput *input;
      input = gltf_materialColor(gst,
                                 subsurface,
                                 &subsurface->color,
                                 _s_ak_multiscatterColor,
                                 0.0f, 0.0f, 0.0f, 1.0f,
                                 subsurface->color ? subsurface->color->texture : NULL,
                                 AK_TEXTURE_COLORSPACE_SRGB,
                                 AK_TEXTURE_CHANNEL_RGB);
      json_array_float(input->color.vec, jval, 0.0f, 3, true);
      input->color.vec[3] = 1.0f;
    } else if (GLTF_JSON_KEY_EQ(jval, scatterAnisotropy)) {
      subsurface->anisotropy = json_float(jval, 0.0f);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseAnisotropy(AkGLTFState       * __restrict gst,
                             AkMaterialSurface * __restrict surface,
                             json_t            * __restrict jspec) {
  AkMaterialAnisotropyFeature *aniso;
  json_t                      *jval;

  aniso = (void*)gltf_materialEnsureFeature(gst,
                                            surface,
                                            AK_MATERIAL_FEATURE_ANISOTROPY,
                                            sizeof(*aniso));

  jval = jspec->value;
  while (jval) {
    if (GLTF_JSON_KEY_EQ(jval, anisotropyStrength)) {
      gltf_materialScalar(gst,
                          aniso,
                          &aniso->strength,
                          _s_ak_anisotropyStrength,
                          json_float(jval, 0.0f),
                          aniso->strength ? aniso->strength->texture : NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_RGB);
    } else if (GLTF_JSON_KEY_EQ(jval, anisotropyRotation)) {
      gltf_materialScalar(gst,
                          aniso,
                          &aniso->rotation,
                          _s_ak_anisotropyRotation,
                          json_float(jval, 0.0f),
                          NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_NONE);
    } else if (GLTF_JSON_KEY_EQ(jval, anisotropyTexture)) {
      gltf_materialScalar(gst,
                          aniso,
                          &aniso->strength,
                          _s_ak_anisotropyStrength,
                          aniso->strength ? aniso->strength->value[0] : 0.0f,
                          gltf_materialTexRef(gst,
                                               aniso,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_RGB),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_RGB);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseDispersion(AkGLTFState       * __restrict gst,
                             AkMaterialSurface * __restrict surface,
                             json_t            * __restrict jspec) {
  AkMaterialDispersionFeature *disp;

  disp = (void*)gltf_materialEnsureFeature(gst,
                                           surface,
                                           AK_MATERIAL_FEATURE_DISPERSION,
                                           sizeof(*disp));
  disp->dispersion = json_float(GLTF_JSON_GET(jspec, dispersion), 0.0f);
}

static
void
gltf_materialParseDiffuseTransmission(AkGLTFState       * __restrict gst,
                                      AkMaterialSurface * __restrict surface,
                                      json_t            * __restrict jspec) {
  AkMaterialDiffuseTransmissionFeature *dt;
  json_t                               *jval;

  dt = (void*)gltf_materialEnsureFeature(
               gst,
               surface,
               AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION,
               sizeof(*dt));

  jval = jspec->value;
  while (jval) {
    if (GLTF_JSON_KEY_EQ(jval, diffuseTransmissionFactor)) {
      gltf_materialScalar(gst,
                          dt,
                          &dt->factor,
                          _s_ak_diffuseTransmission,
                          json_float(jval, 0.0f),
                          dt->factor ? dt->factor->texture : NULL,
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_A);
    } else if (GLTF_JSON_KEY_EQ(jval, diffuseTransmissionTexture)) {
      gltf_materialScalar(gst,
                          dt,
                          &dt->factor,
                          _s_ak_diffuseTransmission,
                          dt->factor ? dt->factor->value[0] : 0.0f,
                          gltf_materialTexRef(gst,
                                               dt,
                                               jval,
                                               AK_TEXTURE_COLORSPACE_LINEAR,
                                               AK_TEXTURE_CHANNEL_A),
                          AK_TEXTURE_COLORSPACE_LINEAR,
                          AK_TEXTURE_CHANNEL_A);
    } else if (GLTF_JSON_KEY_EQ(jval, diffuseTransmissionColorFactor)) {
      AkMaterialInput *input;
      input = gltf_materialColor(gst,
                                 dt,
                                 &dt->color,
                                 _s_ak_diffuseTransmissionColor,
                                 1.0f, 1.0f, 1.0f, 1.0f,
                                 dt->color ? dt->color->texture : NULL,
                                 AK_TEXTURE_COLORSPACE_SRGB,
                                 AK_TEXTURE_CHANNEL_RGB);
      json_array_float(input->color.vec, jval, 1.0f, 3, true);
      input->color.vec[3] = 1.0f;
    } else if (GLTF_JSON_KEY_EQ(jval, diffuseTransmissionColorTexture)) {
      gltf_materialColor(gst,
                         dt,
                         &dt->color,
                         _s_ak_diffuseTransmissionColor,
                         1.0f, 1.0f, 1.0f, 1.0f,
                         gltf_materialTexRef(gst,
                                              dt,
                                              jval,
                                              AK_TEXTURE_COLORSPACE_SRGB,
                                              AK_TEXTURE_CHANNEL_RGB),
                         AK_TEXTURE_COLORSPACE_SRGB,
                         AK_TEXTURE_CHANNEL_RGB);
    }
    jval = jval->next;
  }
}

static
void
gltf_materialParseSpecularGlossiness(AkGLTFState       * __restrict gst,
                                     AkMaterialSurface * __restrict surface,
                                     json_t            * __restrict jspec) {
  AkMaterialSpecularGlossinessFeature *sg;
  json_t                              *jval;

  surface->type = AK_MATERIAL_TYPE_PBR_SPECULAR_GLOSSINESS;
  sg = (void*)gltf_materialEnsureFeature(
               gst,
               surface,
               AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS,
               sizeof(*sg));

  gltf_materialColor(gst,
                     sg,
                     &sg->diffuse,
                     _s_ak_diffuse,
                     1.0f, 1.0f, 1.0f, 1.0f,
                     sg->diffuse ? sg->diffuse->texture : NULL,
                     AK_TEXTURE_COLORSPACE_SRGB,
                     AK_TEXTURE_CHANNEL_RGBA);
  gltf_materialColor(gst,
                     sg,
                     &sg->specular,
                     _s_ak_specular,
                     1.0f, 1.0f, 1.0f, 1.0f,
                     sg->specular ? sg->specular->texture : NULL,
                     AK_TEXTURE_COLORSPACE_SRGB,
                     AK_TEXTURE_CHANNEL_RGB);
  gltf_materialScalar(gst,
                      sg,
                      &sg->glossiness,
                      _s_ak_glossiness,
                      1.0f,
                      sg->glossiness ? sg->glossiness->texture : NULL,
                      AK_TEXTURE_COLORSPACE_LINEAR,
                      AK_TEXTURE_CHANNEL_A);

  jval = jspec->value;
  while (jval) {
    if (gltf_jsonKeyEqLen(jval, _s_gltf_diffuseFactor, 13)) {
      json_array_float(sg->diffuse->color.vec, jval, 0.0f, 4, true);
      surface->baseColor = sg->diffuse;
    } else if (gltf_jsonKeyEqLen(jval, _s_gltf_specFactor, 14)) {
      json_array_float(sg->specular->color.vec, jval, 0.0f, 3, true);
      sg->specular->color.vec[3] = 1.0f;
    } else if (gltf_jsonKeyEqLen(jval, _s_gltf_glossFactor, 16)) {
      sg->glossiness->value[0] = json_float(jval, 1.0f);
    } else if (gltf_jsonKeyEqLen(jval, _s_gltf_diffuseTexture, 14)) {
      gltf_materialColor(gst,
                         sg,
                         &sg->diffuse,
                         _s_ak_diffuse,
                         sg->diffuse->color.vec[0],
                         sg->diffuse->color.vec[1],
                         sg->diffuse->color.vec[2],
                         sg->diffuse->color.vec[3],
                         gltf_materialTexRef(gst,
                                              sg,
                                              jval,
                                              AK_TEXTURE_COLORSPACE_SRGB,
                                              AK_TEXTURE_CHANNEL_RGBA),
                         AK_TEXTURE_COLORSPACE_SRGB,
                         AK_TEXTURE_CHANNEL_RGBA);
      surface->baseColor = sg->diffuse;
    } else if (gltf_jsonKeyEqLen(jval, _s_gltf_specGlossTex, 25)) {
      AkTextureRef *tex;
      tex = gltf_materialTexRef(gst,
                                sg,
                                jval,
                                AK_TEXTURE_COLORSPACE_SRGB,
                                AK_TEXTURE_CHANNEL_RGBA);
      sg->specular->texture = tex;
      sg->specular->source  = AK_MATERIAL_INPUT_TEXTURE;
      sg->specular->channels = AK_TEXTURE_CHANNEL_RGB;
      sg->glossiness->texture = tex;
      sg->glossiness->source  = AK_MATERIAL_INPUT_TEXTURE;
      sg->glossiness->channels = AK_TEXTURE_CHANNEL_A;
    }
    jval = jval->next;
  }
}

static
void
gltf_materialApplyAlphaMode(AkMaterialSurface * __restrict surface,
                            const json_t      * __restrict json) {
  const char *value;

  if (!json || !json->value)
    return;

  value = json->value;
  switch (json->valsize) {
    case _s_gltf_MASK_len:
      if (ak_str_pack4_fast(value, _s_gltf_MASK_len) == _s_gltf_MASK_u32_exact)
        surface->flags |= AK_MATERIAL_FLAG_ALPHA_MASK;
      break;
    case _s_gltf_BLEND_len:
      if (ak_str_eq_packed_fast(value,
                                (size_t)json->valsize,
                                _s_gltf_BLEND_u64_exact,
                                _s_gltf_BLEND_len))
        surface->flags |= AK_MATERIAL_FLAG_ALPHA_BLEND;
      break;
    case _s_gltf_OPAQUE_len:
      if (ak_str_eq_packed_fast(value,
                                (size_t)json->valsize,
                                _s_gltf_OPAQUE_u64_exact,
                                _s_gltf_OPAQUE_len))
        surface->flags &= ~(AK_MATERIAL_FLAG_ALPHA_BLEND | AK_MATERIAL_FLAG_ALPHA_MASK);
      break;
    default:
      break;
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
  size_t              materialIndex;

  gst          = userdata;
  heap         = gst->heap;
  doc          = gst->doc;

  gst->defaultMaterial = gltf_default_mat(gst);

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
    json_t            *jmatVal, *jext;
    AkMaterial        *mat;
    AkMaterialSurface *surface;

    mat     = ak_heap_calloc(heap, doc, sizeof(*mat));
    ak_setypeid(mat, AKT_MATERIAL);
    surface = ak_heap_calloc(heap, mat,    sizeof(*surface));
    gltf_materialInitSurface(gst, surface);
    mat->surface = surface;

    jmatVal = jmaterial->value;
    jext    = gltf_jsonGetLen(jmaterial, _s_gltf_extensions, 10);
    gltf_extra(gst,
               mat,
               GLTF_JSON_GET8(jmaterial, extras),
               jext);

    if (jext) {
      json_t *jspec;

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_specular)))
        gltf_materialParseSpecular(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_clearcoat)))
        gltf_materialParseClearcoat(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_unlit))) {
        surface->type = AK_MATERIAL_TYPE_UNLIT;
        surface->flags |= AK_MATERIAL_FLAG_UNLIT;
      }

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_emissive_strength)))
        surface->emissiveStrength = json_float(GLTF_JSON_GET(jspec,
                                                             emissiveStrength),
                                               1.0f);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_ior)))
        surface->ior = json_float(GLTF_JSON_GET8(jspec, ior), 1.5f);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_transmission)))
        gltf_materialParseTransmission(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_sheen)))
        gltf_materialParseSheen(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_iridescence)))
        gltf_materialParseIridescence(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_volume)))
        gltf_materialParseVolume(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_volume_scatter)))
        gltf_materialParseVolumeScatter(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_anisotropy)))
        gltf_materialParseAnisotropy(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_dispersion)))
        gltf_materialParseDispersion(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_diffuse_transmission)))
        gltf_materialParseDiffuseTransmission(gst, surface, jspec);

      if ((jspec = GLTF_JSON_GET(jext, KHR_materials_pbrSpecularGlossiness)))
        gltf_materialParseSpecularGlossiness(gst, surface, jspec);
    }

    while (jmatVal) {
      if (GLTF_JSON_KEY_EQ8(jmatVal, name)) {
        mat->name = json_strdup(jmatVal, heap, mat);
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_pbrMetalRough, 20)) {
        json_t *jmrVal;

        jmrVal = jmatVal->value;
        while (jmrVal) {
          if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_baseColor, 15)) {
            json_array_float(surface->baseColor->color.vec,
                             jmrVal,
                             0.0f,
                             4,
                             true);
          } else if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_metalFac, 14)) {
            surface->metallic->value[0] = json_float(jmrVal, 0.0f);
          } else if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_roughFac, 15)) {
            surface->roughness->value[0] = json_float(jmrVal, 0.0f);
          } else if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_metalRoughTex, 24)) {
            AkTextureRef *tex;
            tex = gltf_materialTexRef(gst,
                                      surface,
                                      jmrVal,
                                      AK_TEXTURE_COLORSPACE_LINEAR,
                                      AK_TEXTURE_CHANNEL_GB);
            surface->metallic->texture = tex;
            surface->metallic->source  = AK_MATERIAL_INPUT_TEXTURE;
            surface->metallic->channels = AK_TEXTURE_CHANNEL_B;
            surface->roughness->texture = tex;
            surface->roughness->source  = AK_MATERIAL_INPUT_TEXTURE;
            surface->roughness->channels = AK_TEXTURE_CHANNEL_G;
          } else if (gltf_jsonKeyEqLen(jmrVal, _s_gltf_baseColorTex, 16)) {
            surface->baseColor->texture = gltf_materialTexRef(gst,
                                                              surface->baseColor,
                                                              jmrVal,
                                                              AK_TEXTURE_COLORSPACE_SRGB,
                                                              AK_TEXTURE_CHANNEL_RGBA);
            surface->baseColor->source = AK_MATERIAL_INPUT_TEXTURE;
          }

          jmrVal = jmrVal->next;
        }
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_emissiveFac, 14)) {
        AkMaterialInput *input;
        input = gltf_materialColor(gst,
                                   surface,
                                   &surface->emissive,
                                   ak_materialSemanticName(AK_MATERIAL_SEMANTIC_EMISSIVE),
                                   0.0f, 0.0f, 0.0f, 1.0f,
                                   surface->emissive
                                     ? surface->emissive->texture
                                     : NULL,
                                   AK_TEXTURE_COLORSPACE_SRGB,
                                   AK_TEXTURE_CHANNEL_RGB);
        json_array_float(input->color.vec, jmatVal, 0.0f, 3, true);
        input->color.vec[3] = 1.0f;
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_emissiveTex, 15)) {
        gltf_materialColor(gst,
                           surface,
                           &surface->emissive,
                           ak_materialSemanticName(AK_MATERIAL_SEMANTIC_EMISSIVE),
                           0.0f, 0.0f, 0.0f, 1.0f,
                           gltf_materialTexRef(gst,
                                                surface,
                                                jmatVal,
                                                AK_TEXTURE_COLORSPACE_SRGB,
                                                AK_TEXTURE_CHANNEL_RGB),
                           AK_TEXTURE_COLORSPACE_SRGB,
                           AK_TEXTURE_CHANNEL_RGB);
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_occlusionTex, 16)) {
        gltf_materialScalar(gst,
                            surface,
                            &surface->occlusion,
                            ak_materialSemanticName(AK_MATERIAL_SEMANTIC_OCCLUSION),
                            json_float(GLTF_JSON_GET8(jmatVal, strength), 1.0f),
                            gltf_materialTexRef(gst,
                                                 surface,
                                                 jmatVal,
                                                 AK_TEXTURE_COLORSPACE_LINEAR,
                                                 AK_TEXTURE_CHANNEL_R),
                            AK_TEXTURE_COLORSPACE_LINEAR,
                            AK_TEXTURE_CHANNEL_R);
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_normalTex, 13)) {
        gltf_materialScalar(gst,
                            surface,
                            &surface->normal,
                            ak_materialSemanticName(AK_MATERIAL_SEMANTIC_NORMAL),
                            json_float(GLTF_JSON_GET8(jmatVal, scale), 1.0f),
                            gltf_materialTexRef(gst,
                                                 surface,
                                                 jmatVal,
                                                 AK_TEXTURE_COLORSPACE_LINEAR,
                                                 AK_TEXTURE_CHANNEL_RGB),
                            AK_TEXTURE_COLORSPACE_LINEAR,
                            AK_TEXTURE_CHANNEL_RGB);
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_doubleSided, 11)) {
        if (json_bool(jmatVal, 0))
          surface->flags |= AK_MATERIAL_FLAG_DOUBLE_SIDED;
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_alphaMode, 9)) {
        gltf_materialApplyAlphaMode(surface, jmatVal);
      } else if (gltf_jsonKeyEqLen(jmatVal, _s_gltf_alphaCutoff, 11)) {
        surface->alphaCutoff = json_float(jmatVal, 0.5f);
      }

      jmatVal = jmatVal->next;
    }

    AK_LIB_PREPEND(doc->lib.materials, mat, next);
    if (materialIndex > 0)
      gst->materialsByIndex[--materialIndex] = mat;

    jmaterial = jmaterial->next;
  }
}
