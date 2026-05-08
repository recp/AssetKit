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
  return strlen(name) == segLen && strncasecmp(seg, name, segLen) == 0;
}

static
bool
gltf_animPtrSeg(const char ** __restrict p,
                const char  * __restrict end,
                const char  * __restrict name) {
  size_t len;

  if (*p >= end || **p != '/')
    return false;

  (*p)++;
  len = strlen(name);

  if ((size_t)(end - *p) < len || strncmp(*p, name, len) != 0)
    return false;

  *p += len;
  return true;
}

static
bool
gltf_animPtrIndex(const char ** __restrict p,
                  const char  * __restrict end,
                  uint32_t    * __restrict index) {
  uint32_t val;
  bool     found;

  val   = 0;
  found = false;

  while (*p < end && **p >= '0' && **p <= '9') {
    found = true;
    val   = val * 10 + (uint32_t)(**p - '0');
    (*p)++;
  }

  *index = val;
  return found;
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
  char                nodeid[16];

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

  sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex);
  if (!(node = ak_getObjectById(gst->doc, nodeid)))
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
AkTechniqueFxCommon*
gltf_animMaterialCommon(AkMaterial * __restrict mat) {
  AkEffect *effect;

  if (!mat || !mat->effect)
    return NULL;

  effect = mat->effect->base.url.ptr;
  if (!effect)
    return NULL;

  return ak_getProfileTechniqueCommon(effect);
}

static
void
gltf_animSetColorDefault(AkColor * __restrict color,
                         float                 r,
                         float                 g,
                         float                 b,
                         float                 a) {
  color->vec[0] = r;
  color->vec[1] = g;
  color->vec[2] = b;
  color->vec[3] = a;
}

static
AkColor*
gltf_animEnsureColor(AkGLTFState * __restrict gst,
                     AkColorDesc * __restrict desc,
                     void        * __restrict parent,
                     float                    r,
                     float                    g,
                     float                    b,
                     float                    a) {
  AkColor *color;

  if (!desc)
    return NULL;

  if (!(color = desc->color)) {
    color = ak_heap_calloc(gst->heap, parent, sizeof(*color));
    gltf_animSetColorDefault(color, r, g, b, a);
    desc->color = color;
  }

  return color;
}

static
AkMaterialMetallicProp*
gltf_animEnsureMetalProp(AkGLTFState              * __restrict gst,
                         AkTechniqueFxCommon      * __restrict cmn,
                         AkMaterialMetallicProp  ** __restrict prop) {
  if (!*prop) {
    *prop = ak_heap_calloc(gst->heap, cmn, sizeof(**prop));
    (*prop)->intensity = 1.0f;
  }

  return *prop;
}

static
AkMaterialSheen*
gltf_animEnsureSheen(AkGLTFState         * __restrict gst,
                     AkTechniqueFxCommon * __restrict cmn) {
  AkMaterialSheen *sheen;

  if (!(sheen = cmn->sheen)) {
    sheen        = ak_heap_calloc(gst->heap, cmn, sizeof(*sheen));
    sheen->color = ak_heap_calloc(gst->heap, sheen, sizeof(*sheen->color));
    gltf_animEnsureColor(gst, sheen->color, sheen->color,
                         0.0f, 0.0f, 0.0f, 1.0f);
    cmn->sheen = sheen;
  }

  return sheen;
}

static
AkMaterialIridescence*
gltf_animEnsureIridescence(AkGLTFState         * __restrict gst,
                           AkTechniqueFxCommon * __restrict cmn) {
  AkMaterialIridescence *iri;

  if (!(iri = cmn->iridescence)) {
    iri                   = ak_heap_calloc(gst->heap, cmn, sizeof(*iri));
    iri->ior              = 1.3f;
    iri->thicknessMinimum = 100.0f;
    iri->thicknessMaximum = 400.0f;
    cmn->iridescence      = iri;
  }

  return iri;
}

static
AkMaterialVolume*
gltf_animEnsureVolume(AkGLTFState         * __restrict gst,
                      AkTechniqueFxCommon * __restrict cmn) {
  AkMaterialVolume *vol;

  if (!(vol = cmn->volume)) {
    vol = ak_heap_calloc(gst->heap, cmn, sizeof(*vol));
    vol->attenuationColor.vec[0] = 1.0f;
    vol->attenuationColor.vec[1] = 1.0f;
    vol->attenuationColor.vec[2] = 1.0f;
    vol->attenuationColor.vec[3] = 1.0f;
    vol->attenuationDistance     = INFINITY;
    cmn->volume = vol;
  }

  return vol;
}

static
AkMaterialAnisotropy*
gltf_animEnsureAnisotropy(AkGLTFState         * __restrict gst,
                          AkTechniqueFxCommon * __restrict cmn) {
  AkMaterialAnisotropy *aniso;

  if (!(aniso = cmn->anisotropy)) {
    aniso           = ak_heap_calloc(gst->heap, cmn, sizeof(*aniso));
    cmn->anisotropy = aniso;
  }

  return aniso;
}

static
AkMaterialDispersion*
gltf_animEnsureDispersion(AkGLTFState         * __restrict gst,
                          AkTechniqueFxCommon * __restrict cmn) {
  AkMaterialDispersion *disp;

  if (!(disp = cmn->dispersion)) {
    disp            = ak_heap_calloc(gst->heap, cmn, sizeof(*disp));
    cmn->dispersion = disp;
  }

  return disp;
}

static
AkMaterialDiffuseTransmission*
gltf_animEnsureDiffuseTransmission(AkGLTFState         * __restrict gst,
                                   AkTechniqueFxCommon * __restrict cmn) {
  AkMaterialDiffuseTransmission *dt;

  if (!(dt = cmn->diffuseTransmission)) {
    dt        = ak_heap_calloc(gst->heap, cmn, sizeof(*dt));
    dt->color = ak_heap_calloc(gst->heap, dt, sizeof(*dt->color));
    gltf_animEnsureColor(gst, dt->color, dt->color,
                         1.0f, 1.0f, 1.0f, 1.0f);
    cmn->diffuseTransmission = dt;
  }

  return dt;
}

static
bool
gltf_animResolveMaterialPBR(AkGLTFState          * __restrict gst,
                            AkChannel            * __restrict ch,
                            AkTechniqueFxCommon  * __restrict cmn,
                            const char           *p,
                            const char           *end) {
  const char             *seg;
  size_t                  segLen;
  uint32_t                idx;
  bool                    hasIdx;
  AkColor                *color;
  AkMaterialMetallicProp *prop, *rough;

  if (!gltf_animPtrReadSeg(&p, end, &seg, &segLen))
    return false;

  if (gltf_animSegEq(seg, segLen, _s_gltf_baseColorTex)) {
    if (!cmn->albedo)
      cmn->albedo = ak_heap_calloc(gst->heap, cmn, sizeof(*cmn->albedo));
    return gltf_animResolveTexTransform(
             gst,
             ch,
             gltf_animEnsureTexRef(gst,
                                   &cmn->albedo->texture,
                                   cmn->albedo,
                                   AK_TEXTURE_COLORSPACE_SRGB,
                                   AK_TEXTURE_CHANNEL_RGBA),
             p,
             end);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_metalRoughTex)) {
    prop  = gltf_animEnsureMetalProp(gst, cmn, &cmn->metalness);
    rough = gltf_animEnsureMetalProp(gst, cmn, &cmn->roughness);
    prop->textureChannels  = AK_TEXTURE_CHANNEL_B;
    rough->textureChannels = AK_TEXTURE_CHANNEL_G;
    if (!prop->tex)
      prop->tex = gltf_animEnsureTexRef(gst,
                                        &prop->tex,
                                        prop,
                                        AK_TEXTURE_COLORSPACE_LINEAR,
                                        AK_TEXTURE_CHANNEL_GB);
    if (!rough->tex)
      rough->tex = prop->tex;
    return gltf_animResolveTexTransform(gst, ch, prop->tex, p, end);
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
    if (!cmn->albedo)
      cmn->albedo = ak_heap_calloc(gst->heap, cmn, sizeof(*cmn->albedo));
    color = gltf_animEnsureColor(gst, cmn->albedo, cmn->albedo,
                                 1.0f, 1.0f, 1.0f, 1.0f);
    return gltf_animSetFloatArrayTarget(gst, ch, color->vec, 4, idx,
                                        hasIdx, AK_TARGET_COLOR);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_metalFac)) {
    prop = gltf_animEnsureMetalProp(gst, cmn, &cmn->metalness);
    return gltf_animSetFloatTarget(gst, ch, &prop->intensity);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_roughFac)) {
    prop = gltf_animEnsureMetalProp(gst, cmn, &cmn->roughness);
    return gltf_animSetFloatTarget(gst, ch, &prop->intensity);
  }

  return false;
}

static
bool
gltf_animResolveMaterialExt(AkGLTFState          * __restrict gst,
                            AkChannel            * __restrict ch,
                            AkTechniqueFxCommon  * __restrict cmn,
                            const char           *p,
                            const char           *end) {
  const char              *ext;
  const char              *seg;
  size_t                   extLen;
  size_t                   segLen;
  uint32_t                 idx;
  bool                     hasIdx;
  AkColor                 *color;
  AkMaterialSheen         *sheen;
  AkMaterialIridescence   *iri;
  AkMaterialVolume        *vol;
  AkMaterialAnisotropy    *aniso;
  AkMaterialDispersion    *disp;
  AkMaterialDiffuseTransmission *dt;

  if (!gltf_animPtrReadSeg(&p, end, &ext, &extLen)
      || !gltf_animPtrReadSeg(&p, end, &seg, &segLen))
    return false;

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_clearcoat)) {
    if (!cmn->clearcoat) {
      cmn->clearcoat = ak_heap_calloc(gst->heap, cmn,
                                      sizeof(*cmn->clearcoat));
      cmn->clearcoat->normalScale = 1.0f;
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatTexture))
      cmn->clearcoat->textureChannels = AK_TEXTURE_CHANNEL_R;
    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst, &cmn->clearcoat->texture,
                                     cmn->clearcoat,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_R),
               p, end);
    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatRoughnessTexture))
      cmn->clearcoat->roughnessTextureChannels = AK_TEXTURE_CHANNEL_G;
    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatRoughnessTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst,
                                     &cmn->clearcoat->roughnessTexture,
                                     cmn->clearcoat,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_G),
               p, end);
    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatNormalTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst, &cmn->clearcoat->normalTexture,
                                     cmn->clearcoat,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_RGB),
               p, end)
             || (gltf_animPtrSeg(&p, end, _s_gltf_scale)
                 && p == end
                 && gltf_animSetFloatTarget(gst, ch,
                                            &cmn->clearcoat->normalScale));
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_ext_KHR_materials_specular)
      && cmn->specular) {
    if (gltf_animSegEq(seg, segLen, _s_gltf_specularTexture))
      cmn->specular->textureChannels = AK_TEXTURE_CHANNEL_A;
    if (gltf_animSegEq(seg, segLen, _s_gltf_specularTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst, &cmn->specular->specularTex,
                                     cmn->specular,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_A),
               p, end);
    if (gltf_animSegEq(seg, segLen, _s_gltf_specularColorTexture))
      return cmn->specular->color
             && gltf_animResolveTexTransform(
                  gst, ch,
                  gltf_animEnsureTexRef(gst,
                                        &cmn->specular->color->texture,
                                        cmn->specular->color,
                                        AK_TEXTURE_COLORSPACE_SRGB,
                                        AK_TEXTURE_CHANNEL_RGB),
                  p, end);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_transmission)
      && cmn->transmission
      && gltf_animSegEq(seg, segLen, _s_gltf_transmissionTexture))
    cmn->transmission->textureChannels = AK_TEXTURE_CHANNEL_R;
  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_transmission)
      && cmn->transmission
      && gltf_animSegEq(seg, segLen, _s_gltf_transmissionTexture))
    return gltf_animResolveTexTransform(
             gst, ch,
             gltf_animEnsureTexRef(gst, &cmn->transmission->texture,
                                   cmn->transmission,
                                   AK_TEXTURE_COLORSPACE_LINEAR,
                                   AK_TEXTURE_CHANNEL_R),
             p, end);

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_sheen)) {
    sheen = gltf_animEnsureSheen(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_sheenColorTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst, &sheen->color->texture,
                                     sheen->color,
                                     AK_TEXTURE_COLORSPACE_SRGB,
                                     AK_TEXTURE_CHANNEL_RGB),
               p, end);
    if (gltf_animSegEq(seg, segLen, _s_gltf_sheenRoughnessTexture))
      sheen->roughnessTextureChannels = AK_TEXTURE_CHANNEL_A;
    if (gltf_animSegEq(seg, segLen, _s_gltf_sheenRoughnessTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst,
                                     &sheen->roughnessTexture,
                                     sheen,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_A),
               p, end);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_iridescence)) {
    iri = gltf_animEnsureIridescence(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceTexture))
      iri->textureChannels = AK_TEXTURE_CHANNEL_R;
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst,
                                     &iri->texture,
                                     iri,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_R),
               p, end);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceThicknessTexture))
      iri->thicknessTextureChannels = AK_TEXTURE_CHANNEL_G;
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceThicknessTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst,
                                     &iri->thicknessTexture,
                                     iri,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_G),
               p, end);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_volume)) {
    vol = gltf_animEnsureVolume(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_thicknessTexture))
      vol->thicknessTextureChannels = AK_TEXTURE_CHANNEL_G;
    if (gltf_animSegEq(seg, segLen, _s_gltf_thicknessTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst,
                                     &vol->thicknessTexture,
                                     vol,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_G),
               p, end);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_anisotropy)) {
    aniso = gltf_animEnsureAnisotropy(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_anisotropyTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst,
                                     &aniso->texture,
                                     aniso,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_RGB),
               p, end);
  }

  if (gltf_animSegEq(ext, extLen,
                     _s_gltf_KHR_materials_diffuse_transmission)) {
    dt = gltf_animEnsureDiffuseTransmission(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_diffuseTransmissionTexture))
      dt->textureChannels = AK_TEXTURE_CHANNEL_A;
    if (gltf_animSegEq(seg, segLen, _s_gltf_diffuseTransmissionTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst,
                                     &dt->texture,
                                     dt,
                                     AK_TEXTURE_COLORSPACE_LINEAR,
                                     AK_TEXTURE_CHANNEL_A),
               p, end);
    if (gltf_animSegEq(seg, segLen, _s_gltf_diffuseTransmissionColorTexture))
      return gltf_animResolveTexTransform(
               gst, ch,
               gltf_animEnsureTexRef(gst,
                                     &dt->color->texture,
                                     dt->color,
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

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_emissive_strength)
      && gltf_animSegEq(seg, segLen, _s_gltf_emissiveStrength)) {
    if (!cmn->emission) {
      cmn->emission = ak_heap_calloc(gst->heap, cmn, sizeof(*cmn->emission));
      cmn->emission->strength = 1.0f;
    }
    return gltf_animSetFloatTarget(gst, ch, &cmn->emission->strength);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_ior)
      && gltf_animSegEq(seg, segLen, _s_gltf_ior))
    return gltf_animSetFloatTarget(gst, ch, &cmn->ior);

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_transmission)
      && gltf_animSegEq(seg, segLen, _s_gltf_transmissionFactor)) {
    if (!cmn->transmission)
      cmn->transmission = ak_heap_calloc(gst->heap, cmn,
                                         sizeof(*cmn->transmission));
    return gltf_animSetFloatTarget(gst, ch, &cmn->transmission->factor);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_clearcoat)) {
    if (!cmn->clearcoat) {
      cmn->clearcoat = ak_heap_calloc(gst->heap, cmn,
                                      sizeof(*cmn->clearcoat));
      cmn->clearcoat->normalScale = 1.0f;
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatFactor))
      return gltf_animSetFloatTarget(gst, ch, &cmn->clearcoat->intensity);
    if (gltf_animSegEq(seg, segLen, _s_gltf_clearcoatRoughnessFactor))
      return gltf_animSetFloatTarget(gst, ch, &cmn->clearcoat->roughness);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_ext_KHR_materials_specular)) {
    if (!cmn->specular)
      cmn->specular = ak_heap_calloc(gst->heap, cmn, sizeof(*cmn->specular));
    if (gltf_animSegEq(seg, segLen, _s_gltf_specularFactor))
      return gltf_animSetFloatTarget(gst, ch, &cmn->specular->strength);
    if (gltf_animSegEq(seg, segLen, _s_gltf_specularColorFactor)) {
      if (!cmn->specular->color)
        cmn->specular->color = ak_heap_calloc(gst->heap, cmn->specular,
                                              sizeof(*cmn->specular->color));
      color = gltf_animEnsureColor(gst, cmn->specular->color,
                                   cmn->specular->color,
                                   1.0f, 1.0f, 1.0f, 1.0f);
      return gltf_animSetFloatArrayTarget(gst, ch, color->vec, 3, idx,
                                          hasIdx, AK_TARGET_COLOR);
    }
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_sheen)) {
    sheen = gltf_animEnsureSheen(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_sheenColorFactor)) {
      color = gltf_animEnsureColor(gst, sheen->color, sheen->color,
                                   0.0f, 0.0f, 0.0f, 1.0f);
      return gltf_animSetFloatArrayTarget(gst, ch, color->vec, 3, idx,
                                          hasIdx, AK_TARGET_COLOR);
    }
    if (gltf_animSegEq(seg, segLen, _s_gltf_sheenRoughnessFactor))
      return gltf_animSetFloatTarget(gst, ch, &sheen->roughness);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_iridescence)) {
    iri = gltf_animEnsureIridescence(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceFactor))
      return gltf_animSetFloatTarget(gst, ch, &iri->factor);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceIor))
      return gltf_animSetFloatTarget(gst, ch, &iri->ior);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceThicknessMinimum))
      return gltf_animSetFloatTarget(gst, ch, &iri->thicknessMinimum);
    if (gltf_animSegEq(seg, segLen, _s_gltf_iridescenceThicknessMaximum))
      return gltf_animSetFloatTarget(gst, ch, &iri->thicknessMaximum);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_volume)) {
    vol = gltf_animEnsureVolume(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_thicknessFactor))
      return gltf_animSetFloatTarget(gst, ch, &vol->thicknessFactor);
    if (gltf_animSegEq(seg, segLen, _s_gltf_attenuationDistance))
      return gltf_animSetFloatTarget(gst, ch, &vol->attenuationDistance);
    if (gltf_animSegEq(seg, segLen, _s_gltf_attenuationColor))
      return gltf_animSetFloatArrayTarget(gst, ch,
                                          vol->attenuationColor.vec, 3,
                                          idx, hasIdx, AK_TARGET_COLOR);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_anisotropy)) {
    aniso = gltf_animEnsureAnisotropy(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_anisotropyStrength))
      return gltf_animSetFloatTarget(gst, ch, &aniso->strength);
    if (gltf_animSegEq(seg, segLen, _s_gltf_anisotropyRotation))
      return gltf_animSetFloatTarget(gst, ch, &aniso->rotation);
  }

  if (gltf_animSegEq(ext, extLen, _s_gltf_KHR_materials_dispersion)) {
    disp = gltf_animEnsureDispersion(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_dispersion))
      return gltf_animSetFloatTarget(gst, ch, &disp->dispersion);
  }

  if (gltf_animSegEq(ext, extLen,
                     _s_gltf_KHR_materials_diffuse_transmission)) {
    dt = gltf_animEnsureDiffuseTransmission(gst, cmn);
    if (gltf_animSegEq(seg, segLen, _s_gltf_diffuseTransmissionFactor))
      return gltf_animSetFloatTarget(gst, ch, &dt->factor);
    if (gltf_animSegEq(seg, segLen,
                       _s_gltf_diffuseTransmissionColorFactor)) {
      color = gltf_animEnsureColor(gst, dt->color, dt->color,
                                   1.0f, 1.0f, 1.0f, 1.0f);
      return gltf_animSetFloatArrayTarget(gst, ch, color->vec, 3, idx,
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
  AkTechniqueFxCommon *cmn;
  AkColor             *color;
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

  GETCHILD(gst->doc->lib.materials->chld, mat, matIndex);
  if (!(cmn = gltf_animMaterialCommon(mat)))
    return false;

  if (!gltf_animPtrReadSeg(&p, end, &seg, &segLen))
    return false;

  if (gltf_animSegEq(seg, segLen, _s_gltf_pbrMetalRough))
    return gltf_animResolveMaterialPBR(gst, ch, cmn, p, end);

  if (gltf_animSegEq(seg, segLen, _s_gltf_extensions))
    return gltf_animResolveMaterialExt(gst, ch, cmn, p, end);

  if (gltf_animSegEq(seg, segLen, _s_gltf_normalTex)) {
    if (!cmn->normal) {
      cmn->normal = ak_heap_calloc(gst->heap, cmn, sizeof(*cmn->normal));
      cmn->normal->scale = 1.0f;
    }
    q = p;
    if (gltf_animPtrSeg(&q, end, _s_gltf_scale) && q == end)
      return gltf_animSetFloatTarget(gst, ch, &cmn->normal->scale);
    return gltf_animResolveTexTransform(
             gst, ch,
             gltf_animEnsureTexRef(gst,
                                   &cmn->normal->tex,
                                   cmn->normal,
                                   AK_TEXTURE_COLORSPACE_LINEAR,
                                   AK_TEXTURE_CHANNEL_RGB),
             p, end);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_occlusionTex)) {
    if (!cmn->occlusion) {
      cmn->occlusion = ak_heap_calloc(gst->heap, cmn, sizeof(*cmn->occlusion));
      cmn->occlusion->strength = 1.0f;
    }
    q = p;
    if (gltf_animPtrSeg(&q, end, _s_gltf_strength) && q == end)
      return gltf_animSetFloatTarget(gst, ch, &cmn->occlusion->strength);
    cmn->occlusion->textureChannels = AK_TEXTURE_CHANNEL_R;
    return gltf_animResolveTexTransform(
             gst, ch,
             gltf_animEnsureTexRef(gst,
                                   &cmn->occlusion->tex,
                                   cmn->occlusion,
                                   AK_TEXTURE_COLORSPACE_LINEAR,
                                   AK_TEXTURE_CHANNEL_R),
             p, end);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_emissiveTex)) {
    if (!cmn->emission) {
      cmn->emission = ak_heap_calloc(gst->heap, cmn, sizeof(*cmn->emission));
      cmn->emission->strength = 1.0f;
    }
    return gltf_animResolveTexTransform(
             gst, ch,
             gltf_animEnsureTexRef(gst, &cmn->emission->color.texture,
                                   cmn->emission,
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
    if (!cmn->emission)
      cmn->emission = ak_heap_calloc(gst->heap, cmn, sizeof(*cmn->emission));
    color = gltf_animEnsureColor(gst, &cmn->emission->color, cmn->emission,
                                 0.0f, 0.0f, 0.0f, 1.0f);
    return gltf_animSetFloatArrayTarget(gst, ch, color->vec, 3, idx,
                                        hasIdx, AK_TARGET_COLOR);
  }

  if (gltf_animSegEq(seg, segLen, _s_gltf_alphaCutoff)) {
    if (!cmn->transparent) {
      cmn->transparent = ak_heap_calloc(gst->heap, cmn,
                                        sizeof(*cmn->transparent));
      cmn->transparent->amount = 1.0f;
      cmn->transparent->cutoff = 0.5f;
    }
    return gltf_animSetFloatTarget(gst, ch, &cmn->transparent->cutoff);
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

  GETCHILD(gst->doc->lib.cameras->chld, cam, camIndex);
  if (!cam || !cam->optics || !(proj = cam->optics->tcommon))
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

  light = (void *)gst->doc->lib.lights->chld;
  while (light && lightIndex > 0) {
    light = light->next;
    lightIndex--;
  }

  if (!light || !(base = light->tcommon))
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

  if (!(jptrExt = json_get(jext, _s_gltf_KHR_animation_pointer))
      || !(jptr = json_get(jptrExt, _s_gltf_pointer))
      || !(ptr = json_string(jptr)))
    return false;

  if (gltf_animResolveNodePointer(gst, ch, ptr, jptr->valsize))
    return true;
  if (gltf_animResolveMaterialPointer(gst, ch, ptr, jptr->valsize))
    return true;
  if (gltf_animResolveCameraPointer(gst, ch, ptr, jptr->valsize))
    return true;
  if (gltf_animResolveLightPointer(gst, ch, ptr, jptr->valsize))
    return true;

  return false;
}

AK_HIDE
void
gltf_animations(json_t * __restrict janim,
                void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkHeap             *heap;
  AkDoc              *doc;
  const json_array_t *janims;
  AkLibrary          *lib;
  AkAnimation        *anim;

  if (!(janims = json_array(janim)))
    return;

  gst   = userdata;
  heap  = gst->heap;
  doc   = gst->doc;
  janim = janims->base.value;
  lib   = ak_heap_calloc(heap, doc, sizeof(*lib));
  
  while (janim) {
    json_t *anim_it;
    
    json_objmap_t animMap[] = {
      JSON_OBJMAP_OBJ(_s_gltf_samplers, I2P k_anim_samplers),
      JSON_OBJMAP_OBJ(_s_gltf_channels, I2P k_anim_channels),
      JSON_OBJMAP_OBJ(_s_gltf_name,     I2P k_anim_name),
    };
    
    json_objmap(janim, animMap, JSON_ARR_LEN(animMap));
    
    anim = ak_heap_calloc(heap, lib,  sizeof(*anim));

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
          if (json_key_eq(jsampVal, _s_gltf_input)) {
            AkInput *inp;
            
            inp              = ak_heap_calloc(heap, sampler, sizeof(*inp));
            inp->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_input);
            inp->semantic    = AK_INPUT_INPUT;
            inp->accessor    = flist_sp_at(&doc->lib.accessors,
                                           json_int32(jsampVal, -1));
            
            ak_retain(inp->accessor);

            inp->next      = sampler->input;
            sampler->input = inp;
          } else if (json_key_eq(jsampVal, _s_gltf_interpolation)) {
            sampler->uniInterpolation = gltf_interp(jsampVal);
          } else if (json_key_eq(jsampVal, _s_gltf_output)) {
            AkInput *inp;
            
            inp              = ak_heap_calloc(heap, sampler, sizeof(*inp));
            inp->semanticRaw = ak_heap_strdup(gst->heap, anim, _s_gltf_output);
            inp->semantic    = AK_INPUT_OUTPUT;
            inp->accessor    = flist_sp_at(&doc->lib.accessors,
                                           json_int32(jsampVal, -1));
            
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
          if (json_key_eq(jchVal, _s_gltf_sampler)) {
            AkAnimSampler *sampler;
            int32_t        samplerIndex;
            
            samplerIndex = json_int32(jchVal, -1);
            GETCHILD(anim->sampler, sampler, samplerIndex);
            ch->source.ptr = sampler;
          } else if (json_key_eq(jchVal, _s_gltf_target)) {
            const char *path;
            AkNode     *node;
            json_t     *it;
            uint32_t    pathLen;
            
            json_objmap_t targetMap[] = {
              JSON_OBJMAP_OBJ(_s_gltf_path,       I2P k_path),
              JSON_OBJMAP_OBJ(_s_gltf_node,       I2P k_node),
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
                && json_get(it, _s_gltf_KHR_animation_pointer)) {
              if (!gltf_animResolvePointer(gst, ch, it))
                gst->stop = gst->animPointerRequired;
            } else if (path && (it = targetMap[k_node].object)) {
              char    nodeid[16];
              int32_t nodeIndex;
              
              if ((nodeIndex = json_int32(it, -1)) > -1) {
                sprintf(nodeid, "%s%d", _s_gltf_node, nodeIndex);
                
                if ((node = ak_getObjectById(doc, nodeid))) {
                  AkObject *xform = NULL;

                  /* glTF always animates whole vec/quat (no partial component),
                     so isPartial = false and off = 0 for all paths below. */
                  if (strncasecmp(path, _s_gltf_rotation, pathLen) == 0) {
                    ch->targetType = AK_TARGET_QUAT;
                    xform          = ak_getTransformTRS(node, AKT_QUATERNION);
                  } else if (strncasecmp(path, _s_gltf_translation, pathLen) == 0) {
                    ch->targetType = AK_TARGET_POSITION;
                    xform          = ak_getTransformTRS(node, AKT_TRANSLATE);
                  } else if (strncasecmp(path, _s_gltf_scale, pathLen) == 0) {
                    ch->targetType = AK_TARGET_SCALE;
                    xform          = ak_getTransformTRS(node, AKT_SCALE);
                  } else if (strncasecmp(path, _s_gltf_weights, pathLen) == 0) {
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

    anim->base.next = (void *)lib->chld;
    lib->chld       = (void *)anim;
    lib->count++;

    janim = janim->next;
  }

  doc->lib.animations = lib;
}
