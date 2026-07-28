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

#include "anim.h"
#include "accessor.h"
#include "enum.h"
#include "../../../../string_fast.h"
#include "../../../../strpool.h"

#define k_path 0
#define k_node 1
#define k_ext  2

#define k_anim_samplers 0
#define k_anim_channels 1
#define k_anim_name     2

static
bool
gltf_animSegEq(const char * __restrict seg,
               size_t                   segLen,
               const char * __restrict name) {
  size_t i;

  if (!seg || !name)
    return false;

  for (i = 0; i < segLen; i++) {
    if (name[i] == '\0'
        || ak_str_ascii_lower_fast(seg[i])
           != ak_str_ascii_lower_fast(name[i]))
      return false;
  }

  return name[segLen] == '\0';
}

static
bool
gltf_animPtrSeg(const char ** __restrict p,
                const char  * __restrict end,
                const char  * __restrict name) {
  const char *seg;

  if (*p >= end || **p != '/')
    return false;

  (*p)++;
  seg = *p;
  while (seg < end && *name) {
    if (*seg++ != *name++)
      return false;
  }
  if (*name)
    return false;

  *p = seg;
  return true;
}

static
bool
gltf_animPtrIndex(const char ** __restrict p,
                  const char  * __restrict end,
                  uint32_t    * __restrict index) {
  AkUInt val;

  if (*p >= end || **p < '0' || **p > '9')
    return false;

  *p     = ak_str_parse_uint_end_fast((char *)*p, (char *)end, &val);
  *index = val;
  return true;
}

static
bool
gltf_animPtrReadSeg(const char ** __restrict p,
                    const char  * __restrict end,
                    const char ** __restrict seg,
                    size_t      * __restrict segLen) {
  if (*p >= end || **p != '/')
    return false;

  (*p)++;
  *seg = *p;
  while (*p < end && **p != '/')
    (*p)++;

  *segLen = (size_t)(*p - *seg);
  return *segLen > 0;
}

static
void
gltf_animSetTarget(AkGLTFState         * __restrict gst,
                   AkChannel           * __restrict ch,
                   void                * __restrict target,
                   uint32_t                         off,
                   bool                             isPartial,
                   AkTargetPropertyType             targetType) {
  AkResolvedTarget *rt;

  rt = ak_heap_calloc(gst->heap, ch, sizeof(*rt));
  ak_setypeid(rt, AKT_RESOLVED_TARGET);

  rt->target         = target;
  rt->off            = off;
  rt->isPartial      = isPartial;
  ch->targetType     = targetType;
  ch->resolvedTarget = rt;
}

static
bool
gltf_animSetFloatArrayTarget(AkGLTFState         * __restrict gst,
                             AkChannel           * __restrict ch,
                             float               * __restrict target,
                             uint32_t                         len,
                             uint32_t                         index,
                             bool                             hasIndex,
                             AkTargetPropertyType             targetType) {
  if (!target)
    return false;

  if (hasIndex && index >= len)
    return false;

  gltf_animSetTarget(gst, ch, target, index, hasIndex, targetType);
  return true;
}

static
bool
gltf_animSetFloatTarget(AkGLTFState         * __restrict gst,
                        AkChannel           * __restrict ch,
                        float               * __restrict target) {
  if (!target)
    return false;

  gltf_animSetTarget(gst, ch, target, 0, false, AK_TARGET_FLOAT);
  return true;
}

static
AkTextureTransform*
gltf_animEnsureTexTransform(AkGLTFState * __restrict gst,
                            AkTextureRef * __restrict texref) {
  AkTextureTransform *texTransf;

  if (!texref)
    return NULL;

  if (!(texTransf = texref->transform)) {
    texTransf           = ak_heap_calloc(gst->heap, texref, sizeof(*texTransf));
    texTransf->slot     = -1;
    texTransf->scale[0] = 1.0f;
    texTransf->scale[1] = 1.0f;
    texref->transform   = texTransf;
  }

  return texTransf;
}

static
AkTextureRef*
gltf_animEnsureTexRef(AkGLTFState     * __restrict gst,
                      AkTextureRef   ** __restrict texref,
                      void            * __restrict parent,
                      AkTextureColorSpace          colorSpace,
                      AkTextureChannels            channels) {
  if (!*texref) {
    *texref       = ak_heap_calloc(gst->heap, parent, sizeof(**texref));
    (*texref)->slot = -1;
  }

  ak_texref_usage(*texref, colorSpace, channels);

  return *texref;
}

static
bool
gltf_animResolveTexTransform(AkGLTFState     * __restrict gst,
                             AkChannel       * __restrict ch,
                             AkTextureRef    * __restrict texref,
                             const char      *p,
                             const char      *end) {
  AkTextureTransform *texTransf;
  const char         *seg;
  size_t              segLen;
  uint32_t            idx;
  bool                hasIdx;

  if (!gltf_animPtrSeg(&p, end, _s_gltf_extensions)
      || !gltf_animPtrSeg(&p, end, _s_gltf_KHR_texture_transform)
      || !gltf_animPtrReadSeg(&p, end, &seg, &segLen))
    return false;

  hasIdx = false;
  idx    = 0;
  if (p < end) {
    p++;
    if (!gltf_animPtrIndex(&p, end, &idx) || p != end)
      return false;
    hasIdx = true;
  }

  if (!(texTransf = gltf_animEnsureTexTransform(gst, texref)))
    return false;

  if (gltf_animSegEq(seg, segLen, _s_gltf_offset))
    return gltf_animSetFloatArrayTarget(gst, ch, texTransf->offset, 2,
                                        idx, hasIdx, AK_TARGET_VEC2);
  if (gltf_animSegEq(seg, segLen, _s_gltf_rotation))
    return gltf_animSetFloatTarget(gst, ch, &texTransf->rotation);
  if (gltf_animSegEq(seg, segLen, _s_gltf_scale))
    return gltf_animSetFloatArrayTarget(gst, ch, texTransf->scale, 2,
                                        idx, hasIdx, AK_TARGET_VEC2);

  return false;
}

static
bool
gltf_animResolveNodePointer(AkGLTFState     * __restrict gst,
                            AkChannel       * __restrict ch,
                            const char      * __restrict ptr,
                            size_t                       ptrLen) {
  const char         *p;
  const char         *end;
  const char         *prop;
  AkNode             *node;
  AkObject           *xform;
  AkInstanceGeometry *instGeom;
  AkInstanceMorph    *morpher;
  size_t              propLen;
  uint32_t            nodeIndex;
  uint32_t            itemIndex;
  bool                hasItemIndex;

  if (!ptr || ptrLen == 0)
    return false;

  p   = ptr;
  end = ptr + ptrLen;

  if (!gltf_animPtrSeg(&p, end, _s_gltf_nodes)
      || p >= end
      || *p != '/')
    return false;

  p++;
  if (!gltf_animPtrIndex(&p, end, &nodeIndex))
    return false;

  if (p >= end || *p != '/')
    return false;

  p++;
  prop = p;
  while (p < end && *p != '/')
    p++;

  propLen      = (size_t)(p - prop);
  hasItemIndex = false;
  itemIndex    = 0;

  if (!(node = gltf_node_at(gst, nodeIndex)))
    return false;

  if (gltf_animSegEq(prop, propLen, _s_gltf_extensions)) {
    if (!gltf_animPtrSeg(&p, end, _s_gltf_KHR_node_visibility)
        || !gltf_animPtrSeg(&p, end, _s_gltf_visible)
        || p != end)
      return false;

    gltf_animSetTarget(gst, ch, &node->visible, 0, false, AK_TARGET_BOOL);
    return true;
  }

  if (p < end) {
    p++;
    if (!gltf_animPtrIndex(&p, end, &itemIndex) || p != end)
      return false;
    hasItemIndex = true;
  }

  xform = NULL;
  if (gltf_animSegEq(prop, propLen, _s_gltf_rotation)) {
    if (hasItemIndex && itemIndex > 3)
      return false;
    xform = ak_getTransformTRS(node, AKT_QUATERNION);
    if (xform)
      gltf_animSetTarget(gst, ch, xform, itemIndex, hasItemIndex,
                         AK_TARGET_QUAT);
    return xform != NULL;
  }

  if (gltf_animSegEq(prop, propLen, _s_gltf_translation)) {
    if (hasItemIndex && itemIndex > 2)
      return false;
    xform = ak_getTransformTRS(node, AKT_TRANSLATE);
    if (xform)
      gltf_animSetTarget(gst, ch, xform, itemIndex, hasItemIndex,
                         AK_TARGET_POSITION);
    return xform != NULL;
  }

  if (gltf_animSegEq(prop, propLen, _s_gltf_scale)) {
    if (hasItemIndex && itemIndex > 2)
      return false;
    xform = ak_getTransformTRS(node, AKT_SCALE);
    if (xform)
      gltf_animSetTarget(gst, ch, xform, itemIndex, hasItemIndex,
                         AK_TARGET_SCALE);
    return xform != NULL;
  }

  if (gltf_animSegEq(prop, propLen, _s_gltf_weights)) {
    if (!(instGeom = node->geometry) || !(morpher = instGeom->morpher))
      return false;
    if (hasItemIndex
        && morpher->morph
        && itemIndex >= morpher->morph->targetCount)
      return false;
    gltf_animSetTarget(gst, ch, morpher, itemIndex, hasItemIndex,
                       AK_TARGET_WEIGHTS);
    return true;
  }

  return false;
}

static
AkMaterialSurface*
gltf_animMaterialSurface(AkGLTFState * __restrict gst,
                         AkMaterial  * __restrict mat) {
  AkMaterialSurface *surface;

  if (!mat)
    return NULL;

  if ((surface = mat->surface))
    return surface;

  surface                   = ak_heap_calloc(gst->heap, mat, sizeof(*surface));
  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->alphaCutoff      = 0.5f;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;

  mat->surface = surface;
  return surface;
}

static
void
gltf_animPushMaterialFeature(AkMaterialSurface * __restrict surface,
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
gltf_animEnsureMaterialFeature(AkGLTFState          * __restrict gst,
                               AkMaterialSurface    * __restrict surface,
                               AkMaterialFeatureType             type,
                               size_t                            size) {
  AkMaterialFeature *feature;

  if (!surface)
    return NULL;

  if ((feature = ak_materialFeature(surface, type)))
    return feature;

  feature       = ak_heap_calloc(gst->heap, surface, size);
  feature->type = type;
  gltf_animPushMaterialFeature(surface, feature);

  return feature;
}

static
AkMaterialInput*
gltf_animEnsureMaterialInput(AkGLTFState          * __restrict gst,
                             void                 * __restrict parent,
                             AkMaterialInput     ** __restrict slot,
                             const char           * __restrict semantic,
                             AkMaterialInputValue              valueType,
                             AkTextureColorSpace               colorSpace,
                             AkTextureChannels                 channels) {
  AkMaterialInput *input;

  if (!slot)
    return NULL;

  if (!(input = *slot)) {
    input             = ak_heap_aligned_calloc(gst->heap,
                                               parent,
                                               AK_ALIGNOF(AkMaterialInput),
                                               sizeof(*input));
    input->semantic   = semantic;
    input->source     = AK_MATERIAL_INPUT_CONSTANT;
    input->valueType  = valueType;
    input->colorSpace = colorSpace;
    input->channels   = channels;
    *slot             = input;
  } else {
    if (!input->semantic)
      input->semantic = semantic;
    if (input->valueType == AK_MATERIAL_VALUE_NONE)
      input->valueType = valueType;
    if (input->channels == AK_TEXTURE_CHANNEL_NONE)
      input->channels = channels;
    input->colorSpace = colorSpace;
  }

  return input;
}

static
AkMaterialInput*
gltf_animEnsureScalarInput(AkGLTFState          * __restrict gst,
                           void                 * __restrict parent,
                           AkMaterialInput     ** __restrict slot,
                           const char           * __restrict semantic,
                           float                             defaultValue,
                           AkTextureRef         * __restrict texture,
                           AkTextureColorSpace               colorSpace,
                           AkTextureChannels                 channels) {
  AkMaterialInput *input;
  bool             created;

  created = *slot == NULL;
  input = gltf_animEnsureMaterialInput(gst,
                                       parent,
                                       slot,
                                       semantic,
                                       AK_MATERIAL_VALUE_FLOAT,
                                       colorSpace,
                                       channels);
  if (!input)
    return NULL;

  if (created)
    input->value[0] = defaultValue;

  if (texture) {
    input->texture = texture;
    input->source  = AK_MATERIAL_INPUT_TEXTURE;
  }

  return input;
}

static
AkMaterialInput*
gltf_animEnsureColorInput(AkGLTFState          * __restrict gst,
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
  bool             created;

  created = *slot == NULL;
  input = gltf_animEnsureMaterialInput(gst,
                                       parent,
                                       slot,
                                       semantic,
                                       AK_MATERIAL_VALUE_COLOR,
                                       colorSpace,
                                       channels);
  if (!input)
    return NULL;

  if (created) {
    input->color.vec[0] = r;
    input->color.vec[1] = g;
    input->color.vec[2] = b;
    input->color.vec[3] = a;
  }

  if (texture) {
    input->texture = texture;
    input->source  = AK_MATERIAL_INPUT_TEXTURE;
  }

  return input;
}

static
AkMaterialClearcoatFeature*
gltf_animEnsureClearcoatFeature(AkGLTFState       * __restrict gst,
                                AkMaterialSurface * __restrict surface) {
  AkMaterialClearcoatFeature *feature;

  feature = (void*)ak_materialFeature(surface, AK_MATERIAL_FEATURE_CLEARCOAT);
  if (feature)
    return feature;

  feature              = ak_heap_calloc(gst->heap, surface, sizeof(*feature));
  feature->base.type   = AK_MATERIAL_FEATURE_CLEARCOAT;
  feature->normalScale = 1.0f;
  gltf_animPushMaterialFeature(surface, &feature->base);

  return feature;
}

static
AkMaterialIridescenceFeature*
gltf_animEnsureIridescenceFeature(AkGLTFState       * __restrict gst,
                                  AkMaterialSurface * __restrict surface) {
  AkMaterialIridescenceFeature *feature;

  feature = (void*)ak_materialFeature(surface,
                                      AK_MATERIAL_FEATURE_IRIDESCENCE);
  if (feature)
    return feature;

  feature = ak_heap_calloc(gst->heap, surface, sizeof(*feature));
  feature->base.type        = AK_MATERIAL_FEATURE_IRIDESCENCE;
  feature->ior              = 1.3f;
  feature->thicknessMinimum = 100.0f;
  feature->thicknessMaximum = 400.0f;
  gltf_animPushMaterialFeature(surface, &feature->base);

  return feature;
}

static
AkMaterialVolumeFeature*
gltf_animEnsureVolumeFeature(AkGLTFState       * __restrict gst,
                             AkMaterialSurface * __restrict surface) {
  AkMaterialVolumeFeature *feature;

  feature = (void*)ak_materialFeature(surface, AK_MATERIAL_FEATURE_VOLUME);
  if (feature)
    return feature;

  feature = ak_heap_aligned_calloc(
    gst->heap,
    surface,
    AK_ALIGNOF(AkMaterialVolumeFeature),
    sizeof(*feature));
  feature->base.type               = AK_MATERIAL_FEATURE_VOLUME;
  feature->attenuationColor.vec[0] = 1.0f;
  feature->attenuationColor.vec[1] = 1.0f;
  feature->attenuationColor.vec[2] = 1.0f;
  feature->attenuationColor.vec[3] = 1.0f;
  feature->attenuationDistance     = INFINITY;
  gltf_animPushMaterialFeature(surface, &feature->base);

  return feature;
}

static
AkTextureRef*
gltf_animEnsureInputTexRef(AkGLTFState        * __restrict gst,
                           AkMaterialInput    * __restrict input,
                           AkTextureRef      ** __restrict texref,
                           void               * __restrict parent,
                           AkTextureColorSpace             colorSpace,
                           AkTextureChannels               channels) {
  AkTextureRef *tex;

  if (input && input->texture) {
    tex = input->texture;
    if (texref && !*texref)
      *texref = tex;
    ak_texref_usage(tex, colorSpace, channels);
  } else {
    tex = gltf_animEnsureTexRef(gst, texref, parent, colorSpace, channels);
  }

  if (input && tex) {
    input->texture    = tex;
    input->source     = AK_MATERIAL_INPUT_TEXTURE;
    input->colorSpace = colorSpace;
    input->channels   = channels;
  }

  return tex;
}

static
bool
gltf_animResolveMaterialPBR(AkGLTFState          * __restrict gst,
                            AkChannel            * __restrict ch,
                            AkMaterial           * __restrict mat,
                            const char           *p,
                            const char           *end) {
  const char             *seg;
  size_t                  segLen;
  uint32_t                idx;
  bool                    hasIdx;
  AkMaterialSurface      *surface;
  AkMaterialInput        *input, *roughInput;
  AkTextureRef           *tex;

  if (!gltf_animPtrReadSeg(&p, end, &seg, &segLen))
    return false;

  if (!(surface = gltf_animMaterialSurface(gst, mat)))
    return false;

  if (gltf_animSegEq(seg, segLen, _s_gltf_baseColorTex)) {
    input = gltf_animEnsureColorInput(gst,
                                      surface,
                                      &surface->baseColor,
                                      ak_materialSemanticName(AK_MATERIAL_SEMANTIC_BASE_COLOR),
                                      1.0f, 1.0f, 1.0f, 1.0f,
                                      surface->baseColor ? surface->baseColor->texture : NULL,
                                      AK_TEXTURE_COLORSPACE_SRGB,
                                      AK_TEXTURE_CHANNEL_RGBA);
    return gltf_animResolveTexTransform(
             gst,
             ch,
             gltf_animEnsureInputTexRef(gst,
                                        input,
                                        &input->texture,
                                        input,
                                        AK_TEXTURE_COLORSPACE_SRGB,
                                        AK_TEXTURE_CHANNEL_RGBA),
             p,
             end);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_metalRoughTex)) {
    input = gltf_animEnsureScalarInput(gst,
                                       surface,
                                       &surface->metallic,
                                       ak_materialSemanticName(AK_MATERIAL_SEMANTIC_METALLIC),
                                       1.0f,
                                       surface->metallic ? surface->metallic->texture : NULL,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       AK_TEXTURE_CHANNEL_B);
    roughInput = gltf_animEnsureScalarInput(gst,
                                            surface,
                                            &surface->roughness,
                                            ak_materialSemanticName(AK_MATERIAL_SEMANTIC_ROUGHNESS),
                                            1.0f,
                                            surface->roughness ? surface->roughness->texture : NULL,
                                            AK_TEXTURE_COLORSPACE_LINEAR,
                                            AK_TEXTURE_CHANNEL_G);
    tex = input ? input->texture : NULL;
    if (!tex && roughInput)
      tex = roughInput->texture;
    if (!tex)
      tex = gltf_animEnsureTexRef(gst,
                                  input ? &input->texture : NULL,
                                  input ? (void *)input : (void *)surface,
                                  AK_TEXTURE_COLORSPACE_LINEAR,
                                  AK_TEXTURE_CHANNEL_GB);
    if (input && tex) {
      input->texture = tex;
      input->source  = AK_MATERIAL_INPUT_TEXTURE;
      input->channels = AK_TEXTURE_CHANNEL_B;
    }
    if (roughInput && tex) {
      roughInput->texture = tex;
      roughInput->source  = AK_MATERIAL_INPUT_TEXTURE;
      roughInput->channels = AK_TEXTURE_CHANNEL_G;
    }
    return gltf_animResolveTexTransform(gst, ch, tex, p, end);
  }

  hasIdx = false;
  idx    = 0;
  if (p < end) {
    p++;
    if (!gltf_animPtrIndex(&p, end, &idx) || p != end)
      return false;
    hasIdx = true;
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_baseColor)) {
    input = gltf_animEnsureColorInput(gst,
                                      surface,
                                      &surface->baseColor,
                                      ak_materialSemanticName(AK_MATERIAL_SEMANTIC_BASE_COLOR),
                                      1.0f, 1.0f, 1.0f, 1.0f,
                                      surface->baseColor ? surface->baseColor->texture : NULL,
                                      AK_TEXTURE_COLORSPACE_SRGB,
                                      AK_TEXTURE_CHANNEL_RGBA);
    return gltf_animSetFloatArrayTarget(gst, ch, input->color.vec, 4, idx,
                                        hasIdx, AK_TARGET_COLOR);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_metalFac)) {
    input = gltf_animEnsureScalarInput(gst,
                                       surface,
                                       &surface->metallic,
                                       ak_materialSemanticName(AK_MATERIAL_SEMANTIC_METALLIC),
                                       1.0f,
                                       surface->metallic ? surface->metallic->texture : NULL,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       surface->metallic ? surface->metallic->channels : AK_TEXTURE_CHANNEL_B);
    return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_roughFac)) {
    input = gltf_animEnsureScalarInput(gst,
                                       surface,
                                       &surface->roughness,
                                       ak_materialSemanticName(AK_MATERIAL_SEMANTIC_ROUGHNESS),
                                       1.0f,
                                       surface->roughness ? surface->roughness->texture : NULL,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       surface->roughness ? surface->roughness->channels : AK_TEXTURE_CHANNEL_G);
    return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
  }

  return false;
}

static
bool
gltf_animResolveMaterialExt(AkGLTFState * __restrict gst,
                            AkChannel   * __restrict ch,
                            AkMaterial  * __restrict mat,
                            const char  *p,
                            const char  *end) {
  const char                           *ext;
  const char                           *seg;
  AkMaterialSurface                    *surface;
  AkMaterialInput                      *input;
  AkTextureRef                         *tex;
  AkMaterialClearcoatFeature           *clearcoatFeature;
  AkMaterialSpecularFeature            *specularFeature;
  AkMaterialTransmissionFeature        *transmissionFeature;
  AkMaterialSheenFeature               *sheenFeature;
  AkMaterialIridescenceFeature         *iriFeature;
  AkMaterialVolumeFeature              *volumeFeature;
  AkMaterialAnisotropyFeature          *anisoFeature;
  AkMaterialDispersionFeature          *dispFeature;
  AkMaterialDiffuseTransmissionFeature *dtFeature;
  size_t                                extLen;
  size_t                                segLen;
  uint32_t                              idx;
  bool                                  hasIdx;

  if (!gltf_animPtrReadSeg(&p, end, &ext, &extLen)
      || !gltf_animPtrReadSeg(&p, end, &seg, &segLen))
    return false;

  if (!(surface = gltf_animMaterialSurface(gst, mat)))
    return false;

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_clearcoat)) {
    clearcoatFeature = gltf_animEnsureClearcoatFeature(gst, surface);

    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         clearcoatFeature,
                                         &clearcoatFeature->factor,
                                         _s_ak_clearcoat,
                                         0.0f,
                                         clearcoatFeature->factor ? clearcoatFeature->factor->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_R);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_R),
               p, end);
    }

    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatRoughnessTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         clearcoatFeature,
                                         &clearcoatFeature->roughness,
                                         _s_ak_clearcoatRoughness,
                                         0.0f,
                                         clearcoatFeature->roughness ? clearcoatFeature->roughness->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_G);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_G),
               p, end);
    }

    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatNormalTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         clearcoatFeature,
                                         &clearcoatFeature->normal,
                                         _s_ak_clearcoatNormal,
                                         clearcoatFeature->normalScale,
                                         clearcoatFeature->normal ? clearcoatFeature->normal->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_RGB);
      tex = gltf_animEnsureInputTexRef(gst,
                                       input,
                                       &input->texture,
                                       input,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       AK_TEXTURE_CHANNEL_RGB);
      return gltf_animResolveTexTransform(gst, ch, tex, p, end)
             || (gltf_animPtrSeg(&p, end, _s_gltf_scale)
                 && p == end
                 && gltf_animSetFloatTarget(gst, ch,
                                            &clearcoatFeature->normalScale));
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_specular)) {
    specularFeature = (void*)gltf_animEnsureMaterialFeature(
                              gst,
                              surface,
                              AK_MATERIAL_FEATURE_SPECULAR,
                              sizeof(*specularFeature));

    if (gltf_animSegEq(seg, segLen, _s_gltf_specularTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         specularFeature,
                                         &specularFeature->factor,
                                         _s_ak_specularFactor,
                                         1.0f,
                                         specularFeature->factor ? specularFeature->factor->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_A);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_A),
               p, end);
    }

    if (gltf_animSegEq(seg, segLen, _s_gltf_specularColorTexture)) {
      input = gltf_animEnsureColorInput(gst,
                                        specularFeature,
                                        &specularFeature->color,
                                        _s_ak_specularColor,
                                        1.0f, 1.0f, 1.0f, 1.0f,
                                        specularFeature->color ? specularFeature->color->texture : NULL,
                                        AK_TEXTURE_COLORSPACE_SRGB,
                                        AK_TEXTURE_CHANNEL_RGB);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_SRGB,
                                          AK_TEXTURE_CHANNEL_RGB),
               p, end);
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_transmission)
      && gltf_animSegEq(seg, segLen, _s_gltf_transmissionTexture)) {
    transmissionFeature = (void*)gltf_animEnsureMaterialFeature(
                                  gst,
                                  surface,
                                  AK_MATERIAL_FEATURE_TRANSMISSION,
                                  sizeof(*transmissionFeature));
    input = gltf_animEnsureScalarInput(gst,
                                       transmissionFeature,
                                       &transmissionFeature->factor,
                                       _s_ak_transmission,
                                       0.0f,
                                       transmissionFeature->factor ? transmissionFeature->factor->texture : NULL,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       AK_TEXTURE_CHANNEL_R);
    return gltf_animResolveTexTransform(
             gst, ch,
             gltf_animEnsureInputTexRef(gst,
                                        input,
                                        &input->texture,
                                        input,
                                        AK_TEXTURE_COLORSPACE_LINEAR,
                                        AK_TEXTURE_CHANNEL_R),
             p, end);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_sheen)) {
    sheenFeature = (void*)gltf_animEnsureMaterialFeature(
                           gst,
                           surface,
                           AK_MATERIAL_FEATURE_SHEEN,
                           sizeof(*sheenFeature));
    if (gltf_animSegEq(seg, segLen, _s_gltf_sheenColorTexture)) {
      input = gltf_animEnsureColorInput(gst,
                                        sheenFeature,
                                        &sheenFeature->color,
                                        _s_ak_sheenColor,
                                        0.0f, 0.0f, 0.0f, 1.0f,
                                        sheenFeature->color ? sheenFeature->color->texture : NULL,
                                        AK_TEXTURE_COLORSPACE_SRGB,
                                        AK_TEXTURE_CHANNEL_RGB);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_SRGB,
                                          AK_TEXTURE_CHANNEL_RGB),
               p, end);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_sheenRoughnessTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         sheenFeature,
                                         &sheenFeature->roughness,
                                         _s_ak_sheenRoughness,
                                         0.0f,
                                         sheenFeature->roughness ? sheenFeature->roughness->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_A);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_A),
               p, end);
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_iridescence)) {
    iriFeature = gltf_animEnsureIridescenceFeature(gst, surface);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         iriFeature,
                                         &iriFeature->factor,
                                         _s_ak_iridescence,
                                         0.0f,
                                         iriFeature->factor ? iriFeature->factor->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_R);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_R),
               p, end);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceThicknessTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         iriFeature,
                                         &iriFeature->thickness,
                                         _s_ak_iridescenceThickness,
                                         iriFeature->thicknessMaximum,
                                         iriFeature->thickness ? iriFeature->thickness->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_G);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_G),
               p, end);
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_volume)) {
    volumeFeature = gltf_animEnsureVolumeFeature(gst, surface);
    if (gltf_animSegEq(seg, segLen, _s_gltf_thicknessTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         volumeFeature,
                                         &volumeFeature->thickness,
                                         _s_ak_thickness,
                                         0.0f,
                                         volumeFeature->thickness ? volumeFeature->thickness->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_G);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_G),
               p, end);
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_anisotropy)) {
    anisoFeature = (void*)gltf_animEnsureMaterialFeature(
                           gst,
                           surface,
                           AK_MATERIAL_FEATURE_ANISOTROPY,
                           sizeof(*anisoFeature));
    if (gltf_animSegEq(seg, segLen, _s_gltf_anisotropyTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         anisoFeature,
                                         &anisoFeature->strength,
                                         _s_ak_anisotropyStrength,
                                         0.0f,
                                         anisoFeature->strength ? anisoFeature->strength->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_RGB);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_RGB),
               p, end);
    }
  }

  if (gltf_animSegEq(ext, extLen,
                     _s_gltf_KHR_materials_diffuse_transmission)) {
    dtFeature = (void*)gltf_animEnsureMaterialFeature(
                        gst,
                        surface,
                        AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION,
                        sizeof(*dtFeature));
    if (gltf_animSegEq(seg, segLen, _s_gltf_diffuseTransmissionTexture)) {
      input = gltf_animEnsureScalarInput(gst,
                                         dtFeature,
                                         &dtFeature->factor,
                                         _s_ak_diffuseTransmission,
                                         0.0f,
                                         dtFeature->factor ? dtFeature->factor->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_A);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          AK_TEXTURE_CHANNEL_A),
               p, end);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_diffuseTransmissionColorTexture)) {
      input = gltf_animEnsureColorInput(gst,
                                        dtFeature,
                                        &dtFeature->color,
                                        _s_ak_diffuseTransmissionColor,
                                        1.0f, 1.0f, 1.0f, 1.0f,
                                        dtFeature->color ? dtFeature->color->texture : NULL,
                                        AK_TEXTURE_COLORSPACE_SRGB,
                                        AK_TEXTURE_CHANNEL_RGB);
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureInputTexRef(gst,
                                          input,
                                          &input->texture,
                                          input,
                                          AK_TEXTURE_COLORSPACE_SRGB,
                                          AK_TEXTURE_CHANNEL_RGB),
               p, end);
    }
  }

  hasIdx = false;
  idx    = 0;
  if (p < end) {
    p++;
    if (!gltf_animPtrIndex(&p, end, &idx) || p != end)
      return false;
    hasIdx = true;
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_emissive_strength)
      && gltf_animSegEq(seg, segLen, _s_gltf_emissiveStrength)) {
    if (surface->emissiveStrength == 0.0f)
      surface->emissiveStrength = 1.0f;
    return gltf_animSetFloatTarget(gst, ch, &surface->emissiveStrength);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_ior)
      && gltf_animSegEq(seg, segLen, _s_gltf_ior)) {
    if (surface->ior == 0.0f)
      surface->ior = 1.5f;
    return gltf_animSetFloatTarget(gst, ch, &surface->ior);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_transmission)
      && gltf_animSegEq(seg, segLen, _s_gltf_transmissionFactor)) {
    transmissionFeature = (void*)gltf_animEnsureMaterialFeature(
                                  gst,
                                  surface,
                                  AK_MATERIAL_FEATURE_TRANSMISSION,
                                  sizeof(*transmissionFeature));
    input = gltf_animEnsureScalarInput(gst,
                                       transmissionFeature,
                                       &transmissionFeature->factor,
                                       _s_ak_transmission,
                                       0.0f,
                                       transmissionFeature->factor ? transmissionFeature->factor->texture : NULL,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       AK_TEXTURE_CHANNEL_R);
    return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_clearcoat)) {
    clearcoatFeature = gltf_animEnsureClearcoatFeature(gst, surface);
    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatFactor)) {
      input = gltf_animEnsureScalarInput(gst,
                                         clearcoatFeature,
                                         &clearcoatFeature->factor,
                                         _s_ak_clearcoat,
                                         0.0f,
                                         clearcoatFeature->factor ? clearcoatFeature->factor->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_R);
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatRoughnessFactor)) {
      input = gltf_animEnsureScalarInput(gst,
                                         clearcoatFeature,
                                         &clearcoatFeature->roughness,
                                         _s_ak_clearcoatRoughness,
                                         0.0f,
                                         clearcoatFeature->roughness ? clearcoatFeature->roughness->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_G);
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_specular)) {
    specularFeature = (void*)gltf_animEnsureMaterialFeature(
                              gst,
                              surface,
                              AK_MATERIAL_FEATURE_SPECULAR,
                              sizeof(*specularFeature));
    if (gltf_animSegEq(seg, segLen, _s_gltf_specularFactor)) {
      input = gltf_animEnsureScalarInput(gst,
                                         specularFeature,
                                         &specularFeature->factor,
                                         _s_ak_specularFactor,
                                         1.0f,
                                         specularFeature->factor ? specularFeature->factor->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_A);
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_specularColorFactor)) {
      input = gltf_animEnsureColorInput(gst,
                                        specularFeature,
                                        &specularFeature->color,
                                        _s_ak_specularColor,
                                        1.0f, 1.0f, 1.0f, 1.0f,
                                        specularFeature->color ? specularFeature->color->texture : NULL,
                                        AK_TEXTURE_COLORSPACE_SRGB,
                                        AK_TEXTURE_CHANNEL_RGB);
      return gltf_animSetFloatArrayTarget(gst, ch, input->color.vec, 3, idx,
                                          hasIdx, AK_TARGET_COLOR);
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_sheen)) {
    sheenFeature = (void*)gltf_animEnsureMaterialFeature(
                           gst,
                           surface,
                           AK_MATERIAL_FEATURE_SHEEN,
                           sizeof(*sheenFeature));
    if (gltf_animSegEq(seg, segLen, _s_gltf_sheenColorFactor)) {
      input = gltf_animEnsureColorInput(gst,
                                        sheenFeature,
                                        &sheenFeature->color,
                                        _s_ak_sheenColor,
                                        0.0f, 0.0f, 0.0f, 1.0f,
                                        sheenFeature->color ? sheenFeature->color->texture : NULL,
                                        AK_TEXTURE_COLORSPACE_SRGB,
                                        AK_TEXTURE_CHANNEL_RGB);
      return gltf_animSetFloatArrayTarget(gst, ch, input->color.vec, 3, idx,
                                          hasIdx, AK_TARGET_COLOR);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_sheenRoughnessFactor)) {
      input = gltf_animEnsureScalarInput(gst,
                                         sheenFeature,
                                         &sheenFeature->roughness,
                                         _s_ak_sheenRoughness,
                                         0.0f,
                                         sheenFeature->roughness ? sheenFeature->roughness->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_A);
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_iridescence)) {
    iriFeature = gltf_animEnsureIridescenceFeature(gst, surface);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceFactor)) {
      input = gltf_animEnsureScalarInput(gst,
                                         iriFeature,
                                         &iriFeature->factor,
                                         _s_ak_iridescence,
                                         0.0f,
                                         iriFeature->factor ? iriFeature->factor->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_R);
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceIor))
      return gltf_animSetFloatTarget(gst, ch, &iriFeature->ior);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceThicknessMinimum))
      return gltf_animSetFloatTarget(gst, ch, &iriFeature->thicknessMinimum);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceThicknessMaximum))
      return gltf_animSetFloatTarget(gst, ch, &iriFeature->thicknessMaximum);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_volume)) {
    volumeFeature = gltf_animEnsureVolumeFeature(gst, surface);
    if (gltf_animSegEq(seg, segLen, _s_gltf_thicknessFactor)) {
      input = gltf_animEnsureScalarInput(gst,
                                         volumeFeature,
                                         &volumeFeature->thickness,
                                         _s_ak_thickness,
                                         0.0f,
                                         volumeFeature->thickness ? volumeFeature->thickness->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_G);
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_attenuationDistance))
      return gltf_animSetFloatTarget(gst, ch,
                                     &volumeFeature->attenuationDistance);
    if (gltf_animSegEq(seg, segLen, _s_gltf_attenuationColor))
      return gltf_animSetFloatArrayTarget(gst, ch,
                                          volumeFeature->attenuationColor.vec, 3,
                                          idx, hasIdx, AK_TARGET_COLOR);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_anisotropy)) {
    anisoFeature = (void*)gltf_animEnsureMaterialFeature(
                           gst,
                           surface,
                           AK_MATERIAL_FEATURE_ANISOTROPY,
                           sizeof(*anisoFeature));
    if (gltf_animSegEq(seg, segLen, _s_gltf_anisotropyStrength)) {
      input = gltf_animEnsureScalarInput(gst,
                                         anisoFeature,
                                         &anisoFeature->strength,
                                         _s_ak_anisotropyStrength,
                                         0.0f,
                                         anisoFeature->strength ? anisoFeature->strength->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_RGB);
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_anisotropyRotation)) {
      input = gltf_animEnsureScalarInput(gst,
                                         anisoFeature,
                                         &anisoFeature->rotation,
                                         _s_ak_anisotropyRotation,
                                         0.0f,
                                         NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_NONE);
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_dispersion)) {
    dispFeature = (void*)gltf_animEnsureMaterialFeature(
                          gst,
                          surface,
                          AK_MATERIAL_FEATURE_DISPERSION,
                          sizeof(*dispFeature));
    if (gltf_animSegEq(seg, segLen, _s_gltf_dispersion))
      return gltf_animSetFloatTarget(gst, ch, &dispFeature->dispersion);
  }

  if (gltf_animSegEq(ext, extLen,
                     _s_gltf_KHR_materials_diffuse_transmission)) {
    dtFeature = (void*)gltf_animEnsureMaterialFeature(
                        gst,
                        surface,
                        AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION,
                        sizeof(*dtFeature));
    if (gltf_animSegEq(seg, segLen, _s_gltf_diffuseTransmissionFactor)) {
      input = gltf_animEnsureScalarInput(gst,
                                         dtFeature,
                                         &dtFeature->factor,
                                         _s_ak_diffuseTransmission,
                                         0.0f,
                                         dtFeature->factor ? dtFeature->factor->texture : NULL,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         AK_TEXTURE_CHANNEL_A);
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    }
    if (gltf_animSegEq(seg, segLen,
                       _s_gltf_diffuseTransmissionColorFactor)) {
      input = gltf_animEnsureColorInput(gst,
                                        dtFeature,
                                        &dtFeature->color,
                                        _s_ak_diffuseTransmissionColor,
                                        1.0f, 1.0f, 1.0f, 1.0f,
                                        dtFeature->color ? dtFeature->color->texture : NULL,
                                        AK_TEXTURE_COLORSPACE_SRGB,
                                        AK_TEXTURE_CHANNEL_RGB);
      return gltf_animSetFloatArrayTarget(gst, ch, input->color.vec, 3, idx,
                                          hasIdx, AK_TARGET_COLOR);
    }
  }

  return false;
}

static
bool
gltf_animResolveMaterialPointer(AkGLTFState     * __restrict gst,
                                AkChannel       * __restrict ch,
                                const char      * __restrict ptr,
                                size_t                       ptrLen) {
  const char          *p;
  const char          *end;
  const char          *seg;
  const char          *q;
  AkMaterial          *mat;
  AkMaterialSurface   *surface;
  AkMaterialInput     *input;
  size_t               segLen;
  uint32_t             matIndex;
  uint32_t             idx;
  bool                 hasIdx;

  p   = ptr;
  end = ptr + ptrLen;

  if (!gltf_animPtrSeg(&p, end, _s_gltf_materials)
      || p >= end || *p != '/')
    return false;

  p++;
  if (!gltf_animPtrIndex(&p, end, &matIndex)
      || p >= end || *p != '/')
    return false;

  mat = gltf_material_at(gst, matIndex);
  if (!(surface = gltf_animMaterialSurface(gst, mat)))
    return false;

  if (!gltf_animPtrReadSeg(&p, end, &seg, &segLen))
    return false;

  if (gltf_animSegEq(seg, segLen, _s_gltf_pbrMetalRough))
    return gltf_animResolveMaterialPBR(gst, ch, mat, p, end);

  if (gltf_animSegEq(seg, segLen, _s_gltf_extensions))
    return gltf_animResolveMaterialExt(gst, ch, mat, p, end);

  if (gltf_animSegEq(seg, segLen, _s_gltf_normalTex)) {
    q = p;
    input = gltf_animEnsureScalarInput(gst,
                                       surface,
                                       &surface->normal,
                                       ak_materialSemanticName(AK_MATERIAL_SEMANTIC_NORMAL),
                                       1.0f,
                                       surface->normal ? surface->normal->texture : NULL,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       AK_TEXTURE_CHANNEL_RGB);
    if (gltf_animPtrSeg(&q, end, _s_gltf_scale) && q == end)
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    return gltf_animResolveTexTransform(
             gst, ch,
             gltf_animEnsureInputTexRef(gst,
                                        input,
                                        &input->texture,
                                        input,
                                        AK_TEXTURE_COLORSPACE_LINEAR,
                                        AK_TEXTURE_CHANNEL_RGB),
             p, end);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_occlusionTex)) {
    q = p;
    input = gltf_animEnsureScalarInput(gst,
                                       surface,
                                       &surface->occlusion,
                                       ak_materialSemanticName(AK_MATERIAL_SEMANTIC_OCCLUSION),
                                       1.0f,
                                       surface->occlusion ? surface->occlusion->texture : NULL,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       AK_TEXTURE_CHANNEL_R);
    if (gltf_animPtrSeg(&q, end, _s_gltf_strength) && q == end)
      return gltf_animSetFloatTarget(gst, ch, &input->value[0]);
    return gltf_animResolveTexTransform(
             gst, ch,
             gltf_animEnsureInputTexRef(gst,
                                        input,
                                        &input->texture,
                                        input,
                                        AK_TEXTURE_COLORSPACE_LINEAR,
                                        AK_TEXTURE_CHANNEL_R),
             p, end);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_emissiveTex)) {
    input = gltf_animEnsureColorInput(gst,
                                      surface,
                                      &surface->emissive,
                                      ak_materialSemanticName(AK_MATERIAL_SEMANTIC_EMISSIVE),
                                      0.0f, 0.0f, 0.0f, 1.0f,
                                      surface->emissive ? surface->emissive->texture : NULL,
                                      AK_TEXTURE_COLORSPACE_SRGB,
                                      AK_TEXTURE_CHANNEL_RGB);
    return gltf_animResolveTexTransform(
             gst, ch,
             gltf_animEnsureInputTexRef(gst,
                                        input,
                                        &input->texture,
                                        input,
                                        AK_TEXTURE_COLORSPACE_SRGB,
                                        AK_TEXTURE_CHANNEL_RGB),
             p, end);
  }

  hasIdx = false;
  idx    = 0;
  if (p < end) {
    p++;
    if (!gltf_animPtrIndex(&p, end, &idx) || p != end)
      return false;
    hasIdx = true;
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_emissiveFac)) {
    input = gltf_animEnsureColorInput(gst,
                                      surface,
                                      &surface->emissive,
                                      ak_materialSemanticName(AK_MATERIAL_SEMANTIC_EMISSIVE),
                                      0.0f, 0.0f, 0.0f, 1.0f,
                                      surface->emissive ? surface->emissive->texture : NULL,
                                      AK_TEXTURE_COLORSPACE_SRGB,
                                      AK_TEXTURE_CHANNEL_RGB);
    return gltf_animSetFloatArrayTarget(gst, ch, input->color.vec, 3, idx,
                                        hasIdx, AK_TARGET_COLOR);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_alphaCutoff)) {
    if (surface->alphaCutoff == 0.0f)
      surface->alphaCutoff = 0.5f;
    return gltf_animSetFloatTarget(gst, ch, &surface->alphaCutoff);
  }

  return false;
}

static
bool
gltf_animResolveCameraPointer(AkGLTFState     * __restrict gst,
                              AkChannel       * __restrict ch,
                              const char      * __restrict ptr,
                              size_t                       ptrLen) {
  const char     *p;
  const char     *end;
  const char     *seg;
  const char     *prop;
  AkCamera       *cam;
  AkProjection   *proj;
  AkPerspective  *persp;
  AkOrthographic *ortho;
  size_t          segLen;
  size_t          propLen;
  uint32_t        camIndex;

  p   = ptr;
  end = ptr + ptrLen;

  if (!gltf_animPtrSeg(&p, end, _s_gltf_cameras)
      || p >= end || *p != '/')
    return false;

  p++;
  if (!gltf_animPtrIndex(&p, end, &camIndex))
    return false;

  cam = gltf_camera_at(gst, camIndex);
  if (!cam || !cam->optics || !(proj = cam->optics->proj))
    return false;

  if (!gltf_animPtrReadSeg(&p, end, &seg, &segLen)
      || !gltf_animPtrReadSeg(&p, end, &prop, &propLen)
      || p != end)
    return false;

  if (proj->type == AK_PROJECTION_PERSPECTIVE
      && gltf_animSegEq(seg, segLen, _s_gltf_perspective)) {
    persp = (AkPerspective *)proj;
    if (gltf_animSegEq(prop, propLen, _s_gltf_xfov))
      return gltf_animSetFloatTarget(gst, ch, &persp->xfov);
    if (gltf_animSegEq(prop, propLen, _s_gltf_yfov))
      return gltf_animSetFloatTarget(gst, ch, &persp->yfov);
    if (gltf_animSegEq(prop, propLen, _s_gltf_aspectRatio))
      return gltf_animSetFloatTarget(gst, ch, &persp->aspectRatio);
    if (gltf_animSegEq(prop, propLen, _s_gltf_znear))
      return gltf_animSetFloatTarget(gst, ch, &persp->znear);
    if (gltf_animSegEq(prop, propLen, _s_gltf_zfar))
      return gltf_animSetFloatTarget(gst, ch, &persp->zfar);
  }

  if (proj->type == AK_PROJECTION_ORTHOGRAPHIC
      && gltf_animSegEq(seg, segLen, _s_gltf_orthographic)) {
    ortho = (AkOrthographic *)proj;
    if (gltf_animSegEq(prop, propLen, _s_gltf_xmag))
      return gltf_animSetFloatTarget(gst, ch, &ortho->xmag);
    if (gltf_animSegEq(prop, propLen, _s_gltf_ymag))
      return gltf_animSetFloatTarget(gst, ch, &ortho->ymag);
    if (gltf_animSegEq(prop, propLen, _s_gltf_znear))
      return gltf_animSetFloatTarget(gst, ch, &ortho->znear);
    if (gltf_animSegEq(prop, propLen, _s_gltf_zfar))
      return gltf_animSetFloatTarget(gst, ch, &ortho->zfar);
  }

  return false;
}

static
bool
gltf_animResolveLightPointer(AkGLTFState     * __restrict gst,
                             AkChannel       * __restrict ch,
                             const char      * __restrict ptr,
                             size_t                       ptrLen) {
  const char  *p;
  const char  *end;
  const char  *seg;
  AkLight     *light;
  AkLightBase *base;
  AkSpotLight *spot;
  size_t       segLen;
  uint32_t     lightIndex;
  uint32_t     idx;
  bool         hasIdx;

  p   = ptr;
  end = ptr + ptrLen;

  if (!gltf_animPtrSeg(&p, end, _s_gltf_extensions)
      || !gltf_animPtrSeg(&p, end, _s_gltf_KHR_lights_punctual)
      || !gltf_animPtrSeg(&p, end, _s_gltf_lights)
      || p >= end || *p != '/')
    return false;

  p++;
  if (!gltf_animPtrIndex(&p, end, &lightIndex))
    return false;

  light = gst->doc->lib.lights.first;
  while (light && lightIndex > 0) {
    light = light->next;
    lightIndex--;
  }

  if (!light || !(base = light->data))
    return false;

  if (!gltf_animPtrReadSeg(&p, end, &seg, &segLen))
    return false;

  if (base->type == AK_LIGHT_TYPE_SPOT
      && gltf_animSegEq(seg, segLen, _s_gltf_spot)) {
    spot = (AkSpotLight *)base;
    if (!gltf_animPtrReadSeg(&p, end, &seg, &segLen) || p != end)
      return false;
    if (gltf_animSegEq(seg, segLen, _s_gltf_innerConeAngle))
      return gltf_animSetFloatTarget(gst, ch, &spot->innerConeAngle);
    if (gltf_animSegEq(seg, segLen, _s_gltf_outerConeAngle))
      return gltf_animSetFloatTarget(gst, ch, &spot->outerConeAngle);
    return false;
  }

  hasIdx = false;
  idx    = 0;
  if (p < end) {
    p++;
    if (!gltf_animPtrIndex(&p, end, &idx) || p != end)
      return false;
    hasIdx = true;
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_color))
    return gltf_animSetFloatArrayTarget(gst, ch, base->color.vec, 3, idx,
                                        hasIdx, AK_TARGET_COLOR);
  if (gltf_animSegEq(seg, segLen, _s_gltf_intensity))
    return gltf_animSetFloatTarget(gst, ch, &base->intensity);
  if (gltf_animSegEq(seg, segLen, _s_gltf_range))
    return gltf_animSetFloatTarget(gst, ch, &base->range);

  return false;
}

static
bool
gltf_animResolvePointer(AkGLTFState * __restrict gst,
                        AkChannel   * __restrict ch,
                        const json_t * __restrict jext) {
  const json_t *jptrExt;
  const json_t *jptr;
  const char   *ptr;
  size_t        ptrlen;

  if (!(jptrExt = GLTF_JSON_GET(jext, KHR_animation_pointer))
      || !(jptr = GLTF_JSON_GET8(jptrExt, pointer))
      || !(ptr = json_string(jptr)))
    return false;

  ptrlen = jptr->valsize;

  return gltf_animResolveNodePointer(gst, ch, ptr, ptrlen)
      || gltf_animResolveMaterialPointer(gst, ch, ptr, ptrlen)
      || gltf_animResolveCameraPointer(gst, ch, ptr, ptrlen)
      || gltf_animResolveLightPointer(gst, ch, ptr, ptrlen);
}

AK_HIDE
void
gltf_animations(json_t * __restrict janim,
                void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *janims;
  AkAnimation        *anim;

  if (!(janims = json_array(janim)))
    return;

  gst   = userdata;
  heap  = gst->heap;
  doc   = gst->doc;
  janim = janims->base.value;
  
  while (janim) {
    json_t *anim_it;
    
    json_objmap_t animMap[] = {
      GLTF_JSON_OBJMAP_OBJ8(samplers, I2P k_anim_samplers),
      GLTF_JSON_OBJMAP_OBJ8(channels, I2P k_anim_channels),
      GLTF_JSON_OBJMAP_OBJ8(name,     I2P k_anim_name),
    };
    
    json_objmap(janim, animMap, JSON_ARR_LEN(animMap));
    
    anim = ak_heap_calloc(heap, doc,  sizeof(*anim));

    if ((anim_it = animMap[k_anim_name].object)) {
      anim->name = json_strdup(anim_it, heap, anim);
    }
    
    if ((anim_it = animMap[k_anim_samplers].object)) {
      AkAnimSampler *sampler;
      json_array_t  *jsamplers;
      json_t        *jsampler;
      
      if (!(jsamplers = json_array(anim_it)))
        goto anm_nxt;
      
      jsampler = jsamplers->base.value;
      
      /* samplers */
      while (jsampler) {
        json_t *jsampVal;
        
        jsampVal = jsampler->value;
        sampler     = ak_heap_calloc(heap, anim, sizeof(*sampler));
        
        while (jsampVal) {
          if (GLTF_JSON_KEY_EQ8(jsampVal, input)) {
            AkInput *inp;
            
            inp              = ak_heap_calloc(heap, sampler, sizeof(*inp));
            inp->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_input);
            inp->semantic    = AK_INPUT_INPUT;
            inp->accessor    = gltf_accessor_at(gst, json_int32(jsampVal, -1));
            
            ak_retain(inp->accessor);

            inp->next      = sampler->input;
            sampler->input = inp;
          } else if (GLTF_JSON_KEY_EQ(jsampVal, interpolation)) {
            sampler->uniInterpolation = gltf_interp(jsampVal);
          } else if (GLTF_JSON_KEY_EQ8(jsampVal, output)) {
            AkInput *inp;
            
            inp              = ak_heap_calloc(heap, sampler, sizeof(*inp));
            inp->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_output);
            inp->semantic    = AK_INPUT_OUTPUT;
            inp->accessor    = gltf_accessor_at(gst, json_int32(jsampVal, -1));
            
            ak_retain(inp->accessor);

            inp->next      = sampler->input;
            sampler->input = inp;
          }
          
          /* Default is LINEAR */
          if (sampler->uniInterpolation == AK_INTERPOLATION_UNKNOWN) {
            sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;
          }

          jsampVal = jsampVal->next;
        }

        sampler->base.next = (void *)anim->sampler;
        anim->sampler      = sampler;

        jsampler = jsampler->next;
      }
    }
    
    if ((anim_it = animMap[k_anim_channels].object)) {
      AkChannel    *ch;
      json_array_t *jchannels;
      json_t       *jchannel;
      
      if (!(jchannels = json_array(anim_it)))
        goto anm_nxt;
      
      jchannel = jchannels->base.value;
      
      while (jchannel) {
        json_t *jchVal;
        
        ch     = ak_heap_calloc(heap, anim, sizeof(*ch));
        jchVal = jchannel->value;
        
        while (jchVal) {
          if (GLTF_JSON_KEY_EQ8(jchVal, sampler)) {
            AkAnimSampler *sampler;
            int32_t        samplerIndex;
            
            samplerIndex = json_int32(jchVal, -1);
            GETCHILD(anim->sampler, sampler, samplerIndex);
            ch->source.ptr = sampler;
          } else if (GLTF_JSON_KEY_EQ8(jchVal, target)) {
            const char *path;
            AkNode     *node;
            json_t     *it;
            uint32_t    pathLen;
            
            json_objmap_t targetMap[] = {
              GLTF_JSON_OBJMAP_OBJ8(path,         I2P k_path),
              GLTF_JSON_OBJMAP_OBJ8(node,         I2P k_node),
              JSON_OBJMAP_OBJ(_s_gltf_extensions, I2P k_ext)
            };

            json_objmap(jchVal, targetMap, JSON_ARR_LEN(targetMap));

            path    = NULL;
            pathLen = 0;

            if ((it = targetMap[k_path].object)) {
              path    = json_string(it);
              pathLen = it->valsize;
            }

            if ((it = targetMap[k_ext].object)
                && GLTF_JSON_GET(it, KHR_animation_pointer)) {
              if (!gltf_animResolvePointer(gst, ch, it))
                gst->stop = gst->animPointerRequired;
            } else if (path && (it = targetMap[k_node].object)) {
              int32_t nodeIndex;
              
              if ((nodeIndex = json_int32(it, -1)) > -1) {
                if ((node = gltf_node_at(gst, nodeIndex))) {
                  AkObject *xform = NULL;

                  /* glTF always animates whole vec/quat (no partial component),
                     so isPartial = false and off = 0 for all paths below. */
                  if (ak_str_eq_packed_fast(path,
                                            pathLen,
                                            _s_gltf_rotation_u64_exact,
                                            _s_gltf_rotation_len)) {
                    ch->targetType = AK_TARGET_QUAT;
                    xform          = ak_getTransformTRS(node, AKT_QUATERNION);
                  } else if (ak_str_eq_fast(path,
                                            pathLen,
                                            _s_gltf_translation,
                                            _s_gltf_translation_len)) {
                    ch->targetType = AK_TARGET_POSITION;
                    xform          = ak_getTransformTRS(node, AKT_TRANSLATE);
                  } else if (ak_str_eq_packed_fast(path,
                                                   pathLen,
                                                   _s_gltf_scale_u64_exact,
                                                   _s_gltf_scale_len)) {
                    ch->targetType = AK_TARGET_SCALE;
                    xform          = ak_getTransformTRS(node, AKT_SCALE);
                  } else if (ak_str_eq_packed_fast(path,
                                                   pathLen,
                                                   _s_gltf_weights_u64_exact,
                                                   _s_gltf_weights_len)) {
                    AkInstanceGeometry *instGeom;
                    AkInstanceMorph    *morpher;

                    ch->targetType = AK_TARGET_WEIGHTS;

                    if ((instGeom = node->geometry)
                        && (morpher = instGeom->morpher)) {
                      AkResolvedTarget *rt;

                      rt = ak_heap_calloc(heap, ch, sizeof(*rt));
                      ak_setypeid(rt, AKT_RESOLVED_TARGET);

                      rt->target         = morpher;
                      rt->off            = 0;
                      rt->isPartial      = false;
                      ch->resolvedTarget = rt;
                    }
                    /* else: morpher not yet set, channel left un-resolved */
                  }

                  /* common: wrap transform component in AkResolvedTarget */
                  if (xform) {
                    AkResolvedTarget *rt;

                    rt = ak_heap_calloc(heap, ch, sizeof(*rt));
                    ak_setypeid(rt, AKT_RESOLVED_TARGET);

                    rt->target         = xform;
                    rt->off            = 0;
                    rt->isPartial      = false;
                    ch->resolvedTarget = rt;
                  }
                }
              } /* if nodeIndex */
            } /* if k_node */
          } /* if _s_gltf_target */
          
          jchVal = jchVal->next;
        }
        ch->next      = anim->channel;
        anim->channel = ch;
        
        jchannel = jchannel->next;
      }
    }
    
  anm_nxt:

    AK_LIB_PREPEND(doc->lib.animations, anim, next);

    janim = janim->next;
  }
}
