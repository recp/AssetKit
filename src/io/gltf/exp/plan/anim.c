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

#include "internal.h"

#define GLTF_EXP_TEX_XFORM_OFFSET   0u
#define GLTF_EXP_TEX_XFORM_SCALE    1u
#define GLTF_EXP_TEX_XFORM_ROTATION 2u

#define GLTF_EXP_TEX_ROLE_BASE_COLOR                  0u
#define GLTF_EXP_TEX_ROLE_METALLIC_ROUGHNESS          1u
#define GLTF_EXP_TEX_ROLE_OCCLUSION                   2u
#define GLTF_EXP_TEX_ROLE_NORMAL                      3u
#define GLTF_EXP_TEX_ROLE_EMISSIVE                    4u
#define GLTF_EXP_TEX_ROLE_SPECULAR                    6u
#define GLTF_EXP_TEX_ROLE_SPECULAR_COLOR              7u
#define GLTF_EXP_TEX_ROLE_CLEARCOAT                   8u
#define GLTF_EXP_TEX_ROLE_CLEARCOAT_ROUGHNESS         9u
#define GLTF_EXP_TEX_ROLE_CLEARCOAT_NORMAL           10u
#define GLTF_EXP_TEX_ROLE_TRANSMISSION               11u
#define GLTF_EXP_TEX_ROLE_SHEEN_COLOR                12u
#define GLTF_EXP_TEX_ROLE_SHEEN_ROUGHNESS            13u
#define GLTF_EXP_TEX_ROLE_IRIDESCENCE                14u
#define GLTF_EXP_TEX_ROLE_IRIDESCENCE_THICKNESS      15u
#define GLTF_EXP_TEX_ROLE_VOLUME_THICKNESS           16u
#define GLTF_EXP_TEX_ROLE_ANISOTROPY                 17u
#define GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION       18u
#define GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION_COLOR 19u

#define GLTF_EXP_MAT_PTR_BASE_COLOR                   1u
#define GLTF_EXP_MAT_PTR_METALLIC                     2u
#define GLTF_EXP_MAT_PTR_ROUGHNESS                    3u
#define GLTF_EXP_MAT_PTR_ALPHA_CUTOFF                 4u
#define GLTF_EXP_MAT_PTR_EMISSIVE_COLOR               5u
#define GLTF_EXP_MAT_PTR_EMISSIVE_STRENGTH            6u
#define GLTF_EXP_MAT_PTR_NORMAL_SCALE                 7u
#define GLTF_EXP_MAT_PTR_OCCLUSION_STRENGTH           8u
#define GLTF_EXP_MAT_PTR_IOR                          9u
#define GLTF_EXP_MAT_PTR_SPECULAR                     10u
#define GLTF_EXP_MAT_PTR_SPECULAR_COLOR               11u
#define GLTF_EXP_MAT_PTR_CLEARCOAT                    12u
#define GLTF_EXP_MAT_PTR_CLEARCOAT_ROUGHNESS          13u
#define GLTF_EXP_MAT_PTR_CLEARCOAT_NORMAL_SCALE       14u
#define GLTF_EXP_MAT_PTR_TRANSMISSION                 15u
#define GLTF_EXP_MAT_PTR_SHEEN_COLOR                  16u
#define GLTF_EXP_MAT_PTR_SHEEN_ROUGHNESS              17u
#define GLTF_EXP_MAT_PTR_IRIDESCENCE                  18u
#define GLTF_EXP_MAT_PTR_IRIDESCENCE_IOR              19u
#define GLTF_EXP_MAT_PTR_IRIDESCENCE_THICKNESS_MIN    20u
#define GLTF_EXP_MAT_PTR_IRIDESCENCE_THICKNESS_MAX    21u
#define GLTF_EXP_MAT_PTR_VOLUME_THICKNESS             22u
#define GLTF_EXP_MAT_PTR_VOLUME_ATTENUATION_DISTANCE  23u
#define GLTF_EXP_MAT_PTR_VOLUME_ATTENUATION_COLOR     24u
#define GLTF_EXP_MAT_PTR_ANISOTROPY                   25u
#define GLTF_EXP_MAT_PTR_ANISOTROPY_ROTATION          26u
#define GLTF_EXP_MAT_PTR_DISPERSION                   27u
#define GLTF_EXP_MAT_PTR_DIFFUSE_TRANSMISSION         28u
#define GLTF_EXP_MAT_PTR_DIFFUSE_TRANSMISSION_COLOR   29u

AK_HIDE
AkInput*
gltf_anim_sampler_input(AkAnimSampler    * __restrict sampler,
                        AkInputSemantic               semantic) {
  AkInput *input;

  if (!sampler)
    return NULL;

  switch (semantic) {
    case AK_INPUT_INPUT:
      if (sampler->inputInput)
        return sampler->inputInput;
      break;
    case AK_INPUT_OUTPUT:
      if (sampler->outputInput)
        return sampler->outputInput;
      break;
    default:
      break;
  }

  for (input = sampler->input; input; input = input->next) {
    if (input->semantic == semantic)
      return input;
  }

  return NULL;
}

AK_HIDE
uint32_t
gltf_accessor_component_count(AkAccessor * __restrict accessor) {
  if (!accessor)
    return 0;

  if (accessor->componentCount > 0)
    return accessor->componentCount;

  switch (accessor->componentSize) {
    case AK_COMPONENT_SIZE_SCALAR: return 1;
    case AK_COMPONENT_SIZE_VEC2:   return 2;
    case AK_COMPONENT_SIZE_VEC3:   return 3;
    case AK_COMPONENT_SIZE_VEC4:   return 4;
    case AK_COMPONENT_SIZE_MAT2:   return 4;
    case AK_COMPONENT_SIZE_MAT3:   return 9;
    case AK_COMPONENT_SIZE_MAT4:   return 16;
    default: break;
  }

  return 0;
}

AK_HIDE
AkInterpolationType
gltf_anim_sampler_interpolation(AkAnimSampler * __restrict sampler) {
  AkInterpolationType interpolation;

  interpolation = sampler ? sampler->uniInterpolation : AK_INTERPOLATION_UNKNOWN;
  if (interpolation == AK_INTERPOLATION_UNKNOWN)
    interpolation = AK_INTERPOLATION_LINEAR;

  return interpolation;
}

AK_HIDE
bool
gltf_anim_sampler_packed_cubic(AkAnimSampler * __restrict sampler,
                               AkAccessor    * __restrict inputAccessor,
                               AkAccessor    * __restrict outputAccessor,
                               uint32_t                    targetValueCount) {
  size_t expectedCount;

  if (!sampler || !inputAccessor || !outputAccessor)
    return false;

  if (sampler->inTangentInput || sampler->outTangentInput)
    return false;

  if ((size_t)inputAccessor->count > SIZE_MAX / 3u)
    return false;

  expectedCount = (size_t)inputAccessor->count * 3u;
  if (targetValueCount > 0 && expectedCount > SIZE_MAX / targetValueCount)
    return false;

  expectedCount *= targetValueCount;

  return (size_t)outputAccessor->count == expectedCount;
}

AK_HIDE
bool
gltf_anim_input_accessor_supported(AkAccessor * __restrict accessor) {
  return accessor
         && accessor->buffer
         && accessor->buffer->data
         && accessor->componentType == AKT_FLOAT
         && gltf_accessor_component_count(accessor) == 1
         && accessor->count > 0;
}

AK_HIDE
bool
gltf_anim_output_accessor_supported(GLTFExpAnimPath               path,
                                    AkInterpolationType           interpolation,
                                    AkAccessor       * __restrict inputAccessor,
                                    AkAccessor       * __restrict outputAccessor,
                                    AkResolvedTarget * __restrict target) {
  AkInstanceMorph *morpher;
  uint32_t         componentCount;
  uint32_t         keyValueCount;
  size_t           expectedCount;

  if (!outputAccessor
      || !outputAccessor->buffer
      || !outputAccessor->buffer->data)
    return false;

  componentCount = gltf_accessor_component_count(outputAccessor);
  keyValueCount  = interpolation == AK_INTERPOLATION_HERMITE ? 3u : 1u;

  switch (path) {
    case GLTF_EXP_ANIM_POINTER_NODE_VISIBLE:
      return interpolation == AK_INTERPOLATION_STEP
             && outputAccessor->componentType == AKT_UBYTE
             && componentCount == 1
             && outputAccessor->count == inputAccessor->count;
    case GLTF_EXP_ANIM_POINTER_MATERIAL_VALUE:
      if ((size_t)inputAccessor->count > SIZE_MAX / keyValueCount)
        return false;
      return outputAccessor->componentType == AKT_FLOAT
             && (componentCount == 1 || componentCount == 3 || componentCount == 4)
             && (size_t)outputAccessor->count
                == (size_t)inputAccessor->count * keyValueCount;
    case GLTF_EXP_ANIM_POINTER_TEXTURE_TRANSFORM:
      if ((size_t)inputAccessor->count > SIZE_MAX / keyValueCount)
        return false;
      if (target
          && target->off != GLTF_EXP_TEX_XFORM_OFFSET
          && target->off != GLTF_EXP_TEX_XFORM_SCALE
          && target->off != GLTF_EXP_TEX_XFORM_ROTATION)
        return outputAccessor->componentType == AKT_FLOAT
               && componentCount == 1
               && (size_t)outputAccessor->count
                  == (size_t)inputAccessor->count * keyValueCount;
      if (target && target->off == GLTF_EXP_TEX_XFORM_ROTATION)
        return outputAccessor->componentType == AKT_FLOAT
               && componentCount == 1
               && (size_t)outputAccessor->count
                  == (size_t)inputAccessor->count * keyValueCount;
      if (target
          && (target->off == GLTF_EXP_TEX_XFORM_OFFSET
              || target->off == GLTF_EXP_TEX_XFORM_SCALE)
          && outputAccessor->componentType == AKT_FLOAT
          && componentCount == 2
          && (size_t)outputAccessor->count
             == (size_t)inputAccessor->count * keyValueCount)
        return true;
      return outputAccessor->componentType == AKT_FLOAT
             && (componentCount == 1 || componentCount == 3 || componentCount == 4)
             && (size_t)outputAccessor->count
                == (size_t)inputAccessor->count * keyValueCount;
    case GLTF_EXP_ANIM_TRANSLATION:
    case GLTF_EXP_ANIM_SCALE:
      if ((size_t)inputAccessor->count > SIZE_MAX / keyValueCount)
        return false;
      return outputAccessor->componentType == AKT_FLOAT
             && componentCount == 3
             && (size_t)outputAccessor->count
                == (size_t)inputAccessor->count * keyValueCount;
    case GLTF_EXP_ANIM_ROTATION:
      if ((size_t)inputAccessor->count > SIZE_MAX / keyValueCount)
        return false;
      return outputAccessor->componentType == AKT_FLOAT
             && componentCount == 4
             && (size_t)outputAccessor->count
                == (size_t)inputAccessor->count * keyValueCount;
    case GLTF_EXP_ANIM_WEIGHTS:
      if (outputAccessor->componentType != AKT_FLOAT)
        return false;
      if (componentCount != 1)
        return false;
      morpher = target ? target->target : NULL;
      if (!morpher || !morpher->morph || morpher->morph->targetCount == 0) {
        if ((size_t)inputAccessor->count > SIZE_MAX / keyValueCount)
          return false;
        return (size_t)outputAccessor->count
               >= (size_t)inputAccessor->count * keyValueCount;
      }
      if ((size_t)inputAccessor->count > SIZE_MAX / keyValueCount)
        return false;
      expectedCount = (size_t)inputAccessor->count * keyValueCount;
      if (expectedCount > SIZE_MAX / morpher->morph->targetCount)
        return false;
      expectedCount *= morpher->morph->targetCount;
      return (size_t)outputAccessor->count == expectedCount;
    default:
      break;
  }

  return false;
}

AK_HIDE
uint32_t
gltf_anim_target_value_count(GLTFExpAnimPath path,
                             AkResolvedTarget * __restrict target) {
  AkInstanceMorph *morpher;

  if (path != GLTF_EXP_ANIM_WEIGHTS || !target || !target->target)
    return 1u;

  morpher = target->target;
  if (!morpher->morph || morpher->morph->targetCount == 0)
    return 1u;

  return morpher->morph->targetCount;
}

AK_HIDE
bool
gltf_anim_path(AkChannel        * __restrict channel,
               AkResolvedTarget * __restrict target,
               GLTFExpAnimPath  * __restrict path) {
  if (!channel || !target || !target->target || target->isPartial)
    return false;

  switch (channel->targetType) {
    case AK_TARGET_POSITION:
      *path = GLTF_EXP_ANIM_TRANSLATION;
      return true;
    case AK_TARGET_QUAT:
      *path = GLTF_EXP_ANIM_ROTATION;
      return true;
    case AK_TARGET_SCALE:
      *path = GLTF_EXP_ANIM_SCALE;
      return true;
    case AK_TARGET_WEIGHTS:
      *path = GLTF_EXP_ANIM_WEIGHTS;
      return true;
    case AK_TARGET_BOOL:
      *path = GLTF_EXP_ANIM_POINTER_NODE_VISIBLE;
      return true;
    case AK_TARGET_VEC2:
      *path = GLTF_EXP_ANIM_POINTER_TEXTURE_TRANSFORM;
      return true;
    case AK_TARGET_COLOR:
    case AK_TARGET_VEC4:
      *path = GLTF_EXP_ANIM_POINTER_MATERIAL_VALUE;
      return true;
    case AK_TARGET_FLOAT:
      *path = target->off > GLTF_EXP_TEX_XFORM_ROTATION
              ? GLTF_EXP_ANIM_POINTER_MATERIAL_VALUE
              : GLTF_EXP_ANIM_POINTER_TEXTURE_TRANSFORM;
      return true;
    default:
      break;
  }

  return false;
}

AK_HIDE
AkObject*
gltf_node_transform(AkNode * __restrict node, AkTypeId type) {
  AkObject *obj;

  if (!node || !node->transform)
    return NULL;

  for (obj = node->transform->item; obj; obj = obj->next) {
    if (obj->type == type)
      return obj;
  }

  return NULL;
}

AK_HIDE
bool
gltf_node_transform_chain_trs(AkNode * __restrict node) {
  AkObject *obj;
  bool      hasTranslation;
  bool      hasRotation;
  bool      hasScale;

  if (!node || !node->transform)
    return false;

  hasTranslation = false;
  hasRotation    = false;
  hasScale       = false;

  for (obj = node->transform->item; obj; obj = obj->next) {
    switch (obj->type) {
      case AKT_TRANSLATE:
        if (hasTranslation)
          return false;
        hasTranslation = true;
        break;
      case AKT_QUATERNION:
        if (hasRotation)
          return false;
        hasRotation = true;
        break;
      case AKT_SCALE:
        if (hasScale)
          return false;
        hasScale = true;
        break;
      default:
        return false;
    }
  }

  return hasTranslation || hasRotation || hasScale;
}

AK_HIDE
bool
gltf_node_has_morpher(AkNode * __restrict node,
                      void   * __restrict target) {
  AkInstanceGeometry *inst;

  for (inst = node ? node->geometry : NULL;
       inst;
       inst = (AkInstanceGeometry *)inst->base.next) {
    if (inst->morpher == target)
      return true;
  }

  return false;
}

AK_HIDE
bool
gltf_anim_node_matches(GLTFExpNodeOut  * __restrict out,
                       AkResolvedTarget * __restrict target,
                       GLTFExpAnimPath               path) {
  if (!out || !out->node || !target)
    return false;

  switch (path) {
    case GLTF_EXP_ANIM_TRANSLATION:
      return gltf_node_transform_chain_trs(out->node)
             && gltf_node_transform(out->node, AKT_TRANSLATE) == target->target;
    case GLTF_EXP_ANIM_ROTATION:
      return gltf_node_transform_chain_trs(out->node)
             && gltf_node_transform(out->node, AKT_QUATERNION) == target->target;
    case GLTF_EXP_ANIM_SCALE:
      return gltf_node_transform_chain_trs(out->node)
             && gltf_node_transform(out->node, AKT_SCALE) == target->target;
    case GLTF_EXP_ANIM_WEIGHTS:
      return gltf_node_has_morpher(out->node, target->target);
    case GLTF_EXP_ANIM_POINTER_NODE_VISIBLE:
      return target->target == &out->node->visible;
    default:
      break;
  }

  return false;
}

AK_HIDE
bool
gltf_anim_input_matches_value(AkMaterialInput  * __restrict input,
                              AkResolvedTarget * __restrict target) {
  return input
         && target
         && target->target
         && (target->target == input
             || target->target == &input->value[0]
             || target->target == input->color.vec);
}

AK_HIDE
bool
gltf_anim_float_matches_value(float            * __restrict value,
                              AkResolvedTarget * __restrict target) {
  return value && target && target->target == value;
}

AK_HIDE
bool
gltf_anim_color_matches_value(AkColor          * __restrict value,
                              AkResolvedTarget * __restrict target) {
  return value && target && target->target == value->vec;
}

AK_HIDE
bool
gltf_anim_feature_value_matches(AkMaterialFeature * __restrict feature,
                                AkResolvedTarget  * __restrict target,
                                uint32_t          * __restrict prop) {
  if (!feature || !target || !target->target || !prop)
    return false;

  switch (feature->type) {
    case AK_MATERIAL_FEATURE_SPECULAR: {
      AkMaterialSpecularFeature *f = (AkMaterialSpecularFeature *)feature;
      if (gltf_anim_input_matches_value(f->factor, target)) {
        *prop = GLTF_EXP_MAT_PTR_SPECULAR;
        return true;
      }
      if (gltf_anim_input_matches_value(f->color, target)) {
        *prop = GLTF_EXP_MAT_PTR_SPECULAR_COLOR;
        return true;
      }
      break;
    }
    case AK_MATERIAL_FEATURE_CLEARCOAT: {
      AkMaterialClearcoatFeature *f = (AkMaterialClearcoatFeature *)feature;
      if (gltf_anim_input_matches_value(f->factor, target)) {
        *prop = GLTF_EXP_MAT_PTR_CLEARCOAT;
        return true;
      }
      if (gltf_anim_input_matches_value(f->roughness, target)) {
        *prop = GLTF_EXP_MAT_PTR_CLEARCOAT_ROUGHNESS;
        return true;
      }
      if (gltf_anim_input_matches_value(f->normal, target)
          || gltf_anim_float_matches_value(&f->normalScale, target)) {
        *prop = GLTF_EXP_MAT_PTR_CLEARCOAT_NORMAL_SCALE;
        return true;
      }
      break;
    }
    case AK_MATERIAL_FEATURE_TRANSMISSION: {
      AkMaterialTransmissionFeature *f = (AkMaterialTransmissionFeature *)feature;
      if (gltf_anim_input_matches_value(f->factor, target)) {
        *prop = GLTF_EXP_MAT_PTR_TRANSMISSION;
        return true;
      }
      break;
    }
    case AK_MATERIAL_FEATURE_SHEEN: {
      AkMaterialSheenFeature *f = (AkMaterialSheenFeature *)feature;
      if (gltf_anim_input_matches_value(f->color, target)) {
        *prop = GLTF_EXP_MAT_PTR_SHEEN_COLOR;
        return true;
      }
      if (gltf_anim_input_matches_value(f->roughness, target)) {
        *prop = GLTF_EXP_MAT_PTR_SHEEN_ROUGHNESS;
        return true;
      }
      break;
    }
    case AK_MATERIAL_FEATURE_IRIDESCENCE: {
      AkMaterialIridescenceFeature *f = (AkMaterialIridescenceFeature *)feature;
      if (gltf_anim_input_matches_value(f->factor, target)) {
        *prop = GLTF_EXP_MAT_PTR_IRIDESCENCE;
        return true;
      }
      if (gltf_anim_float_matches_value(&f->ior, target)) {
        *prop = GLTF_EXP_MAT_PTR_IRIDESCENCE_IOR;
        return true;
      }
      if (gltf_anim_float_matches_value(&f->thicknessMinimum, target)) {
        *prop = GLTF_EXP_MAT_PTR_IRIDESCENCE_THICKNESS_MIN;
        return true;
      }
      if (gltf_anim_float_matches_value(&f->thicknessMaximum, target)) {
        *prop = GLTF_EXP_MAT_PTR_IRIDESCENCE_THICKNESS_MAX;
        return true;
      }
      break;
    }
    case AK_MATERIAL_FEATURE_VOLUME: {
      AkMaterialVolumeFeature *f = (AkMaterialVolumeFeature *)feature;
      if (gltf_anim_input_matches_value(f->thickness, target)) {
        *prop = GLTF_EXP_MAT_PTR_VOLUME_THICKNESS;
        return true;
      }
      if (gltf_anim_float_matches_value(&f->attenuationDistance, target)) {
        *prop = GLTF_EXP_MAT_PTR_VOLUME_ATTENUATION_DISTANCE;
        return true;
      }
      if (gltf_anim_color_matches_value(&f->attenuationColor, target)) {
        *prop = GLTF_EXP_MAT_PTR_VOLUME_ATTENUATION_COLOR;
        return true;
      }
      break;
    }
    case AK_MATERIAL_FEATURE_ANISOTROPY: {
      AkMaterialAnisotropyFeature *f = (AkMaterialAnisotropyFeature *)feature;
      if (gltf_anim_input_matches_value(f->strength, target)) {
        *prop = GLTF_EXP_MAT_PTR_ANISOTROPY;
        return true;
      }
      if (gltf_anim_input_matches_value(f->rotation, target)) {
        *prop = GLTF_EXP_MAT_PTR_ANISOTROPY_ROTATION;
        return true;
      }
      break;
    }
    case AK_MATERIAL_FEATURE_DISPERSION: {
      AkMaterialDispersionFeature *f = (AkMaterialDispersionFeature *)feature;
      if (gltf_anim_float_matches_value(&f->dispersion, target)) {
        *prop = GLTF_EXP_MAT_PTR_DISPERSION;
        return true;
      }
      break;
    }
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION: {
      AkMaterialDiffuseTransmissionFeature *f;
      f = (AkMaterialDiffuseTransmissionFeature *)feature;
      if (gltf_anim_input_matches_value(f->factor, target)) {
        *prop = GLTF_EXP_MAT_PTR_DIFFUSE_TRANSMISSION;
        return true;
      }
      if (gltf_anim_input_matches_value(f->color, target)) {
        *prop = GLTF_EXP_MAT_PTR_DIFFUSE_TRANSMISSION_COLOR;
        return true;
      }
      break;
    }
    default:
      break;
  }

  return false;
}

AK_HIDE
bool
gltf_anim_surface_value_matches(AkMaterialSurface * __restrict surface,
                                AkResolvedTarget  * __restrict target,
                                uint32_t          * __restrict prop) {
  AkMaterialFeature *feature;

  if (!surface || !target || !target->target || !prop)
    return false;

  if (gltf_anim_input_matches_value(surface->baseColor, target)) {
    *prop = GLTF_EXP_MAT_PTR_BASE_COLOR;
    return true;
  }
  if (gltf_anim_input_matches_value(surface->metallic, target)) {
    *prop = GLTF_EXP_MAT_PTR_METALLIC;
    return true;
  }
  if (gltf_anim_input_matches_value(surface->roughness, target)) {
    *prop = GLTF_EXP_MAT_PTR_ROUGHNESS;
    return true;
  }
  if (gltf_anim_float_matches_value(&surface->alphaCutoff, target)) {
    *prop = GLTF_EXP_MAT_PTR_ALPHA_CUTOFF;
    return true;
  }
  if (gltf_anim_input_matches_value(surface->emissive, target)) {
    *prop = GLTF_EXP_MAT_PTR_EMISSIVE_COLOR;
    return true;
  }
  if (gltf_anim_float_matches_value(&surface->emissiveStrength, target)) {
    *prop = GLTF_EXP_MAT_PTR_EMISSIVE_STRENGTH;
    return true;
  }
  if (gltf_anim_input_matches_value(surface->normal, target)) {
    *prop = GLTF_EXP_MAT_PTR_NORMAL_SCALE;
    return true;
  }
  if (gltf_anim_input_matches_value(surface->occlusion, target)) {
    *prop = GLTF_EXP_MAT_PTR_OCCLUSION_STRENGTH;
    return true;
  }
  if (gltf_anim_float_matches_value(&surface->ior, target)) {
    *prop = GLTF_EXP_MAT_PTR_IOR;
    return true;
  }

  for (feature = surface->features; feature; feature = feature->next) {
    if (gltf_anim_feature_value_matches(feature, target, prop))
      return true;
  }

  return false;
}

AK_HIDE
bool
gltf_anim_material_value_index(GLTFExpState     * __restrict st,
                               AkResolvedTarget * __restrict target,
                               GLTFExpIndex     * __restrict materialIndex,
                               uint32_t         * __restrict prop) {
  size_t i;

  if (!st || !target || !target->target || !materialIndex || !prop)
    return false;

  for (i = 0; i < st->materials.count; i++) {
    AkMaterialSurface *surface;
    AkMaterial        *material;

    material = st->materials.items[i].material;
    surface  = material ? material->surface : NULL;
    if (gltf_anim_surface_value_matches(surface, target, prop)) {
      *materialIndex = (GLTFExpIndex)i;
      return true;
    }
  }

  return false;
}

AK_HIDE
AkTextureTransform*
gltf_anim_input_texture_transform(AkMaterialInput * __restrict input) {
  AkTextureRef *texref;

  texref = ak_materialInputTexture(input);
  return texref ? texref->transform : NULL;
}

AK_HIDE
bool
gltf_anim_input_transform_prop(AkMaterialInput  * __restrict input,
                               AkResolvedTarget * __restrict target,
                               uint32_t         * __restrict prop) {
  AkTextureTransform *transform;

  if (!input || !target || !target->target || !prop)
    return false;

  transform = gltf_anim_input_texture_transform(input);
  if (!transform)
    return false;

  if (target->target == transform) {
    if (target->off != GLTF_EXP_TEX_XFORM_OFFSET
        && target->off != GLTF_EXP_TEX_XFORM_SCALE
        && target->off != GLTF_EXP_TEX_XFORM_ROTATION)
      return false;
    *prop = target->off;
    return true;
  }

  if (target->target == transform->offset) {
    *prop = GLTF_EXP_TEX_XFORM_OFFSET;
    return true;
  }
  if (target->target == &transform->rotation) {
    *prop = GLTF_EXP_TEX_XFORM_ROTATION;
    return true;
  }
  if (target->target == transform->scale) {
    *prop = GLTF_EXP_TEX_XFORM_SCALE;
    return true;
  }

  return false;
}

AK_HIDE
bool
gltf_anim_feature_texture_matches(AkMaterialFeature * __restrict feature,
                                  AkResolvedTarget  * __restrict target,
                                  uint32_t                       role,
                                  uint32_t         * __restrict prop) {
  if (!feature)
    return false;

  switch (feature->type) {
    case AK_MATERIAL_FEATURE_SPECULAR: {
      AkMaterialSpecularFeature *f = (AkMaterialSpecularFeature *)feature;
      return (role == GLTF_EXP_TEX_ROLE_SPECULAR
              && gltf_anim_input_transform_prop(f->factor, target, prop))
             || (role == GLTF_EXP_TEX_ROLE_SPECULAR_COLOR
                 && gltf_anim_input_transform_prop(f->color, target, prop));
    }
    case AK_MATERIAL_FEATURE_CLEARCOAT: {
      AkMaterialClearcoatFeature *f = (AkMaterialClearcoatFeature *)feature;
      return (role == GLTF_EXP_TEX_ROLE_CLEARCOAT
              && gltf_anim_input_transform_prop(f->factor, target, prop))
             || (role == GLTF_EXP_TEX_ROLE_CLEARCOAT_ROUGHNESS
                 && gltf_anim_input_transform_prop(f->roughness, target, prop))
             || (role == GLTF_EXP_TEX_ROLE_CLEARCOAT_NORMAL
                 && gltf_anim_input_transform_prop(f->normal, target, prop));
    }
    case AK_MATERIAL_FEATURE_TRANSMISSION: {
      AkMaterialTransmissionFeature *f = (AkMaterialTransmissionFeature *)feature;
      return role == GLTF_EXP_TEX_ROLE_TRANSMISSION
             && gltf_anim_input_transform_prop(f->factor, target, prop);
    }
    case AK_MATERIAL_FEATURE_SHEEN: {
      AkMaterialSheenFeature *f = (AkMaterialSheenFeature *)feature;
      return (role == GLTF_EXP_TEX_ROLE_SHEEN_COLOR
              && gltf_anim_input_transform_prop(f->color, target, prop))
             || (role == GLTF_EXP_TEX_ROLE_SHEEN_ROUGHNESS
                 && gltf_anim_input_transform_prop(f->roughness, target, prop));
    }
    case AK_MATERIAL_FEATURE_IRIDESCENCE: {
      AkMaterialIridescenceFeature *f = (AkMaterialIridescenceFeature *)feature;
      return (role == GLTF_EXP_TEX_ROLE_IRIDESCENCE
              && gltf_anim_input_transform_prop(f->factor, target, prop))
             || (role == GLTF_EXP_TEX_ROLE_IRIDESCENCE_THICKNESS
                 && gltf_anim_input_transform_prop(f->thickness, target, prop));
    }
    case AK_MATERIAL_FEATURE_VOLUME: {
      AkMaterialVolumeFeature *f = (AkMaterialVolumeFeature *)feature;
      return role == GLTF_EXP_TEX_ROLE_VOLUME_THICKNESS
             && gltf_anim_input_transform_prop(f->thickness, target, prop);
    }
    case AK_MATERIAL_FEATURE_ANISOTROPY: {
      AkMaterialAnisotropyFeature *f = (AkMaterialAnisotropyFeature *)feature;
      return role == GLTF_EXP_TEX_ROLE_ANISOTROPY
             && gltf_anim_input_transform_prop(f->strength, target, prop);
    }
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION: {
      AkMaterialDiffuseTransmissionFeature *f;
      f = (AkMaterialDiffuseTransmissionFeature *)feature;
      return (role == GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION
              && gltf_anim_input_transform_prop(f->factor, target, prop))
             || (role == GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION_COLOR
                 && gltf_anim_input_transform_prop(f->color, target, prop));
    }
    default:
      break;
  }

  return false;
}

AK_HIDE
bool
gltf_anim_surface_texture_matches(AkMaterialSurface * __restrict surface,
                                  AkResolvedTarget  * __restrict target,
                                  uint32_t                       role,
                                  uint32_t         * __restrict prop) {
  AkMaterialFeature *feature;

  if (!surface)
    return false;

  switch (role) {
    case GLTF_EXP_TEX_ROLE_BASE_COLOR:
      return gltf_anim_input_transform_prop(surface->baseColor, target, prop);
    case GLTF_EXP_TEX_ROLE_METALLIC_ROUGHNESS:
      return gltf_anim_input_transform_prop(surface->metallic, target, prop)
             || gltf_anim_input_transform_prop(surface->roughness, target, prop);
    case GLTF_EXP_TEX_ROLE_OCCLUSION:
      return gltf_anim_input_transform_prop(surface->occlusion, target, prop);
    case GLTF_EXP_TEX_ROLE_NORMAL:
      return gltf_anim_input_transform_prop(surface->normal, target, prop);
    case GLTF_EXP_TEX_ROLE_EMISSIVE:
      return gltf_anim_input_transform_prop(surface->emissive, target, prop);
    default:
      break;
  }

  for (feature = surface->features; feature; feature = feature->next) {
    if (gltf_anim_feature_texture_matches(feature, target, role, prop))
      return true;
  }

  return false;
}

AK_HIDE
bool
gltf_anim_texture_transform_index(GLTFExpState     * __restrict st,
                                  AkResolvedTarget * __restrict target,
                                  GLTFExpIndex     * __restrict materialIndex,
                                  uint32_t         * __restrict role,
                                  uint32_t         * __restrict prop) {
  static const uint32_t roles[] = {
    GLTF_EXP_TEX_ROLE_BASE_COLOR,
    GLTF_EXP_TEX_ROLE_METALLIC_ROUGHNESS,
    GLTF_EXP_TEX_ROLE_OCCLUSION,
    GLTF_EXP_TEX_ROLE_NORMAL,
    GLTF_EXP_TEX_ROLE_EMISSIVE,
    GLTF_EXP_TEX_ROLE_SPECULAR,
    GLTF_EXP_TEX_ROLE_SPECULAR_COLOR,
    GLTF_EXP_TEX_ROLE_CLEARCOAT,
    GLTF_EXP_TEX_ROLE_CLEARCOAT_ROUGHNESS,
    GLTF_EXP_TEX_ROLE_CLEARCOAT_NORMAL,
    GLTF_EXP_TEX_ROLE_TRANSMISSION,
    GLTF_EXP_TEX_ROLE_SHEEN_COLOR,
    GLTF_EXP_TEX_ROLE_SHEEN_ROUGHNESS,
    GLTF_EXP_TEX_ROLE_IRIDESCENCE,
    GLTF_EXP_TEX_ROLE_IRIDESCENCE_THICKNESS,
    GLTF_EXP_TEX_ROLE_VOLUME_THICKNESS,
    GLTF_EXP_TEX_ROLE_ANISOTROPY,
    GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION,
    GLTF_EXP_TEX_ROLE_DIFFUSE_TRANSMISSION_COLOR
  };
  size_t i;
  size_t r;

  if (!st || !target || !target->target || !materialIndex || !role || !prop)
    return false;
  for (i = 0; i < st->materials.count; i++) {
    AkMaterialSurface *surface;
    AkMaterial        *material;

    material = st->materials.items[i].material;
    surface  = material ? material->surface : NULL;
    for (r = 0; r < sizeof(roles) / sizeof(roles[0]); r++) {
      if (gltf_anim_surface_texture_matches(surface, target, roles[r], prop)) {
        *materialIndex = (GLTFExpIndex)i;
        *role = roles[r];
        return true;
      }
    }
  }

  return false;
}

AK_HIDE
bool
gltf_anim_add_channel_ex(GLTFExpState   * __restrict st,
                         GLTFExpIndex                samplerIndex,
                         GLTFExpIndex                nodeIndex,
                         GLTFExpAnimPath             path,
                         uint32_t                    pointerRole,
                         uint32_t                    pointerProp) {
  GLTFExpAnimChannelOut *out;
  size_t                 newCap;

  if (st->animChannels.count >= GLTF_EXP_INDEX_NONE)
    return false;

  if (st->animChannels.count == st->animChannels.capacity) {
    if (!gltf_next_capacity(st->animChannels.capacity, 64, &newCap))
      return false;
    if (!gltf_anim_channels_reserve(&st->animChannels, newCap))
      return false;
  }

  out = &st->animChannels.items[st->animChannels.count++];
  out->samplerIndex = samplerIndex;
  out->nodeIndex    = nodeIndex;
  out->path         = path;
  out->pointerRole  = pointerRole;
  out->pointerProp  = pointerProp;

  if (path == GLTF_EXP_ANIM_POINTER_NODE_VISIBLE) {
    st->nodes.items[nodeIndex].forceVisibilityExtension = true;
    st->usesNodeVisibility   = true;
    st->usesAnimationPointer = true;
  } else if (path == GLTF_EXP_ANIM_POINTER_MATERIAL_VALUE) {
    st->usesAnimationPointer = true;
  } else if (path == GLTF_EXP_ANIM_POINTER_TEXTURE_TRANSFORM) {
    st->usesAnimationPointer = true;
  }

  return true;
}

AK_HIDE
bool
gltf_anim_add_channel(GLTFExpState   * __restrict st,
                      GLTFExpIndex                samplerIndex,
                      GLTFExpIndex                nodeIndex,
                      GLTFExpAnimPath             path) {
  return gltf_anim_add_channel_ex(st, samplerIndex, nodeIndex, path, 0, 0);
}

AK_HIDE
GLTFExpIndex
gltf_anim_find_sampler(GLTFExpState * __restrict st,
                       GLTFExpAnimOut * __restrict anim,
                       AkAnimSampler * __restrict sampler) {
  uint32_t i;

  for (i = 0; i < anim->samplerCount; i++) {
    GLTFExpIndex index;

    index = anim->samplerOffset + i;
    if (st->animSamplers.items[index].sampler == sampler)
      return index;
  }

  return GLTF_EXP_INDEX_NONE;
}

AK_HIDE
bool
gltf_anim_add_sampler(GLTFExpState     * __restrict st,
                      GLTFExpAnimOut   * __restrict anim,
                      AkAnimSampler    * __restrict sampler,
                      AkAccessor       * __restrict inputAccessor,
                      AkAccessor       * __restrict outputAccessor,
                      GLTFExpIndex     * __restrict samplerIndex) {
  GLTFExpAnimSamplerOut *out;
  AkInterpolationType    interpolation;
  GLTFExpIndex           index;
  size_t                 newCap;

  index = gltf_anim_find_sampler(st, anim, sampler);
  if (index != GLTF_EXP_INDEX_NONE) {
    *samplerIndex = index;
    return true;
  }

  if (anim->samplerCount == UINT32_MAX)
    return false;

  interpolation = gltf_anim_sampler_interpolation(sampler);

  if (interpolation != AK_INTERPOLATION_LINEAR
      && interpolation != AK_INTERPOLATION_STEP
      && interpolation != AK_INTERPOLATION_HERMITE)
    return false;

  if (!gltf_accessors_require_minmax(&st->accessors, inputAccessor)
      || !gltf_accessors_add_accessor(&st->accessors, outputAccessor))
    return false;

  if (st->animSamplers.count >= GLTF_EXP_INDEX_NONE)
    return false;

  if (st->animSamplers.count == st->animSamplers.capacity) {
    if (!gltf_next_capacity(st->animSamplers.capacity, 32, &newCap))
      return false;
    if (!gltf_anim_samplers_reserve(&st->animSamplers, newCap))
      return false;
  }

  index = (GLTFExpIndex)st->animSamplers.count++;
  out   = &st->animSamplers.items[index];
  out->sampler             = sampler;
  out->inputAccessorIndex  = gltf_accessor_index(&st->accessors, inputAccessor);
  out->outputAccessorIndex = gltf_accessor_index(&st->accessors, outputAccessor);
  out->interpolation       = interpolation;

  if (out->inputAccessorIndex == GLTF_EXP_INDEX_NONE
      || out->outputAccessorIndex == GLTF_EXP_INDEX_NONE)
    return false;

  anim->samplerCount++;
  *samplerIndex = index;

  return true;
}

AK_HIDE
GLTFExpAnimPlanResult
gltf_plan_anim_channel(GLTFExpState   * __restrict st,
                       GLTFExpAnimOut * __restrict anim,
                       AkChannel      * __restrict channel) {
  
  AkAnimSampler      *sampler;
  AkInput            *inputInput;
  AkInput            *outputInput;
  AkAccessor         *inputAccessor;
  AkAccessor         *outputAccessor;
  AkResolvedTarget    target;
  AkInterpolationType interpolation;
  GLTFExpAnimPath     path;
  GLTFExpIndex        samplerIndex;
  GLTFExpIndex        materialIndex;
  size_t              i;
  uint32_t            pointerRole;
  uint32_t            pointerProp;
  uint32_t            nodeMatchCount;
  uint32_t            targetValueCount;

  if (!channel || !channel->source.ptr || !channel->resolvedTarget)
    return GLTF_EXP_ANIM_PLAN_SKIP;

  target = *channel->resolvedTarget;
  if (!gltf_anim_path(channel, &target, &path))
    return GLTF_EXP_ANIM_PLAN_SKIP;

  sampler          = channel->source.ptr;
  inputInput       = gltf_anim_sampler_input(sampler, AK_INPUT_INPUT);
  outputInput      = gltf_anim_sampler_input(sampler, AK_INPUT_OUTPUT);
  inputAccessor    = inputInput  ? inputInput->accessor  : NULL;
  outputAccessor   = outputInput ? outputInput->accessor : NULL;
  interpolation    = gltf_anim_sampler_interpolation(sampler);
  targetValueCount = gltf_anim_target_value_count(path, &target);

  if (!gltf_anim_input_accessor_supported(inputAccessor)
      || (interpolation == AK_INTERPOLATION_HERMITE
          && !gltf_anim_sampler_packed_cubic(sampler,
                                             inputAccessor,
                                             outputAccessor,
                                             targetValueCount))
      || !gltf_anim_output_accessor_supported(path,
                                              interpolation,
                                              inputAccessor,
                                              outputAccessor,
                                              &target))
    return GLTF_EXP_ANIM_PLAN_SKIP;

  if (path == GLTF_EXP_ANIM_POINTER_MATERIAL_VALUE) {
    if (!gltf_anim_material_value_index(st, &target, &materialIndex, &pointerProp))
      return GLTF_EXP_ANIM_PLAN_SKIP;

    if (!gltf_anim_add_sampler(st,
                               anim,
                               sampler,
                               inputAccessor,
                               outputAccessor,
                               &samplerIndex))
      return GLTF_EXP_ANIM_PLAN_ERROR;

    if (anim->channelCount == UINT32_MAX)
      return GLTF_EXP_ANIM_PLAN_ERROR;

    if (!gltf_anim_add_channel_ex(st,
                                  samplerIndex,
                                  materialIndex,
                                  path,
                                  0,
                                  pointerProp))
      return GLTF_EXP_ANIM_PLAN_ERROR;
    anim->channelCount++;
    return GLTF_EXP_ANIM_PLAN_OK;
  }

  if (path == GLTF_EXP_ANIM_POINTER_TEXTURE_TRANSFORM) {
    if (!gltf_anim_texture_transform_index(st,
                                           &target,
                                           &materialIndex,
                                           &pointerRole,
                                           &pointerProp)) {
      if (!gltf_anim_material_value_index(st,
                                          &target,
                                          &materialIndex,
                                          &pointerProp))
        return GLTF_EXP_ANIM_PLAN_SKIP;
      path = GLTF_EXP_ANIM_POINTER_MATERIAL_VALUE;
      pointerRole = 0;
    }

    if (!gltf_anim_add_sampler(st,
                               anim,
                               sampler,
                               inputAccessor,
                               outputAccessor,
                               &samplerIndex))
      return GLTF_EXP_ANIM_PLAN_ERROR;

    if (anim->channelCount == UINT32_MAX)
      return GLTF_EXP_ANIM_PLAN_ERROR;

    if (!gltf_anim_add_channel_ex(st,
                                  samplerIndex,
                                  materialIndex,
                                  path,
                                  pointerRole,
                                  pointerProp))
      return GLTF_EXP_ANIM_PLAN_ERROR;
    anim->channelCount++;
    return GLTF_EXP_ANIM_PLAN_OK;
  }

  nodeMatchCount = 0;
  for (i = 0; i < st->nodes.count; i++) {
    if (gltf_anim_node_matches(&st->nodes.items[i], &target, path))
      nodeMatchCount++;
  }

  if (nodeMatchCount == 0)
    return GLTF_EXP_ANIM_PLAN_SKIP;

  if (!gltf_anim_add_sampler(st,
                             anim,
                             sampler,
                             inputAccessor,
                             outputAccessor,
                             &samplerIndex))
    return GLTF_EXP_ANIM_PLAN_ERROR;

  for (i = 0; i < st->nodes.count; i++) {
    if (!gltf_anim_node_matches(&st->nodes.items[i], &target, path))
      continue;

    if (anim->channelCount == UINT32_MAX)
      return GLTF_EXP_ANIM_PLAN_ERROR;

    if (i >= GLTF_EXP_INDEX_NONE
        || !gltf_anim_add_channel(st, samplerIndex, (GLTFExpIndex)i, path))
      return GLTF_EXP_ANIM_PLAN_ERROR;

    if (path != GLTF_EXP_ANIM_WEIGHTS
        && path != GLTF_EXP_ANIM_POINTER_NODE_VISIBLE)
      st->nodes.items[i].forceTRS = true;
    anim->channelCount++;
  }

  return GLTF_EXP_ANIM_PLAN_OK;
}

AK_HIDE
bool
gltf_plan_animation_one(GLTFExpState * __restrict st,
                        AkAnimation  * __restrict animation) {
  GLTFExpAnimOut *out;
  AkChannel      *channel;
  GLTFExpIndex    animIndex;
  size_t          newCap;
  GLTFExpIndex    samplerStart;
  GLTFExpIndex    channelStart;

  if (!animation)
    return true;

  if (st->animSamplers.count    >= GLTF_EXP_INDEX_NONE
      || st->animChannels.count >= GLTF_EXP_INDEX_NONE
      || st->animations.count   >= GLTF_EXP_INDEX_NONE)
    return false;

  if (st->animations.count == st->animations.capacity) {
    if (!gltf_next_capacity(st->animations.capacity, 16, &newCap) 
        || !gltf_anims_reserve(&st->animations, newCap))
      return false;
  }

  samplerStart = (GLTFExpIndex)st->animSamplers.count;
  channelStart = (GLTFExpIndex)st->animChannels.count;
  animIndex    = (GLTFExpIndex)st->animations.count++;
  out          = &st->animations.items[animIndex];

  memset(out, 0, sizeof(*out));
  out->animation     = animation;
  out->name          = animation->name;
  out->samplerOffset = samplerStart;
  out->channelOffset = channelStart;

  for (channel = animation->channel; channel; channel = channel->next) {
    GLTFExpAnimPlanResult result;

    result = gltf_plan_anim_channel(st, out, channel);
    if (result == GLTF_EXP_ANIM_PLAN_ERROR)
      return false;
  }

  if (out->channelCount == 0) {
    st->animations.count   = animIndex;
    st->animSamplers.count = samplerStart;
    st->animChannels.count = channelStart;
  }

  return true;
}

AK_HIDE
bool
gltf_plan_animation_tree(GLTFExpState * __restrict st,
                         AkAnimation  * __restrict animation) {
  for (; animation; animation = animation->next) {
    if (!gltf_plan_animation_one(st, animation))
      return false;
    if (animation->animation
        && !gltf_plan_animation_tree(st, animation->animation))
      return false;
  }

  return true;
}

AK_HIDE
bool
gltf_plan_animations(GLTFExpState * __restrict st) {
  return gltf_plan_animation_tree(st, st->doc->lib.animations.first);
}
