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

#include "scenekit.h"
#include "../../../mat/internal.h"
#include "../../../color.h"
#include "../../../string_fast.h"

#include <ctype.h>
#include <string.h>

static
bool
dae_strcase_contains(const char * __restrict str,
                     const char * __restrict needle) {
  const char *s, *n, *match;

  if (!str || !needle || !needle[0])
    return false;

  for (; *str; str++) {
    s     = str;
    n     = needle;
    match = str;

    while (*s && *n
           && tolower((unsigned char)*s) == tolower((unsigned char)*n)) {
      s++;
      n++;
    }

    if (!*n)
      return true;

    str = match;
  }

  return false;
}

AK_HIDE
bool
dae_scenekit_authored(AkDoc * __restrict doc) {
  AkContributor *contr;

  if (!doc || !doc->inf)
    return false;

  for (contr = doc->inf->base.contributor; contr; contr = contr->next) {
    if (dae_strcase_contains(contr->authoringTool, "scenekit"))
      return true;
  }

  return false;
}

static
void
dae_authoring_state_inspect(DAEState * __restrict dst) {
  AkContributor *contr;
  bool           authoringToolSeen;

  if (!dst
      || (dst->sceneKitAuthoringChecked
          && dst->colorSRGBAuthoringChecked))
    return;

  dst->sceneKitAuthored = false;
  dst->colorSRGBAuthored = false;
  authoringToolSeen = false;
  if (dst->doc && dst->doc->inf) {
    for (contr = dst->doc->inf->base.contributor; contr; contr = contr->next) {
      if (!contr->authoringTool || !contr->authoringTool[0])
        continue;

      authoringToolSeen = true;
      if (dae_strcase_contains(contr->authoringTool, "scenekit")) {
        dst->sceneKitAuthored = true;
        dst->colorSRGBAuthored = true;
        break;
      }
      if (dae_strcase_contains(contr->authoringTool, "sketchup"))
        dst->colorSRGBAuthored = true;
    }
  }

  if (!authoringToolSeen && dst->sceneKitProfileSeen) {
    dst->sceneKitAuthored  = true;
    dst->colorSRGBAuthored = true;
  }

  dst->sceneKitAuthoringChecked = true;
  dst->colorSRGBAuthoringChecked = true;
}

static
bool
dae_scenekit_state_authored(DAEState * __restrict dst) {
  if (!dst || !dst->doc)
    return false;

  dae_authoring_state_inspect(dst);
  return dst->sceneKitAuthored;
}

static
bool
dae_srgb_color_authored(DAEState * __restrict dst) {
  if (!dst || !dst->doc)
    return false;

  dae_authoring_state_inspect(dst);
  return dst->colorSRGBAuthored;
}

static
bool
dae_scenekit_accessor_is_color(AkDoc      * __restrict doc,
                               AkAccessor * __restrict accessor) {
  AkGeometry *geom;

  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    AkMesh          *mesh;
    AkMeshPrimitive *prim;

    if (!geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
      continue;

    mesh = ak_objGet(geom->gdata);
    for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next) {
      AkInput *input;

      for (input = prim->input; input; input = input->next) {
        if (input->semantic == AK_INPUT_COLOR && input->accessor == accessor)
          return true;
      }
    }
  }

  return false;
}

static
size_t
dae_scenekit_color_input_count(AkDoc * __restrict doc) {
  AkGeometry *geom;
  size_t      count;

  count = 0u;
  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    AkMesh          *mesh;
    AkMeshPrimitive *prim;

    if (!geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
      continue;

    mesh = ak_objGet(geom->gdata);
    for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next) {
      AkInput *input;

      for (input = prim->input; input; input = input->next) {
        if (input->semantic == AK_INPUT_COLOR && input->accessor)
          count++;
      }
    }
  }

  return count;
}

static
bool
dae_scenekit_color_accessor_insert(AkAccessor ** __restrict slots,
                                    size_t                   mask,
                                    AkAccessor * __restrict accessor) {
  size_t slot;

  slot = (size_t)ak__docPtrHash(accessor) & mask;
  while (slots[slot]) {
    if (slots[slot] == accessor)
      return false;
    slot = (slot + 1u) & mask;
  }

  slots[slot] = accessor;
  return true;
}

static
void
dae_scenekit_normalize_accessor_color(AkAccessor * __restrict accessor) {
  char    *row;
  size_t   stride;
  uint32_t i, components;

  if (!accessor
      || accessor->componentType != AKT_FLOAT
      || !accessor->buffer
      || !accessor->buffer->data)
    return;

  components = accessor->componentCount;
  if (components < 3u)
    return;

  stride = accessor->byteStride
           ? accessor->byteStride
           : (size_t)components * sizeof(float);
  row = (char *)accessor->buffer->data + accessor->byteOffset;
  for (i = 0; i < accessor->count; i++) {
    float *color;

    color    = (float *)(void *)row;
    color[0] = ak_srgb_to_linearf_fast(color[0]);
    color[1] = ak_srgb_to_linearf_fast(color[1]);
    color[2] = ak_srgb_to_linearf_fast(color[2]);
    row     += stride;
  }
}

static
void
dae_scenekit_normalize_color(AkColor * __restrict color) {
  if (!color)
    return;

  color->rgba.R = ak_srgb_to_linearf_fast(color->rgba.R);
  color->rgba.G = ak_srgb_to_linearf_fast(color->rgba.G);
  color->rgba.B = ak_srgb_to_linearf_fast(color->rgba.B);
}

static
void
dae_scenekit_normalize_color_desc(AkColorDesc * __restrict desc) {
  if (desc && !desc->texture)
    dae_scenekit_normalize_color(desc->color);
}

static
void
dae_scenekit_normalize_sampler(AkSampler * __restrict sampler) {
  if (sampler)
    dae_scenekit_normalize_color(sampler->borderColor);
}

static
void
dae_scenekit_normalize_newparams(AkNewParam * __restrict param) {
  for (; param; param = param->next) {
    AkTexture *texture;

    if (!param->val || !param->val->value)
      continue;

    switch (param->val->type.typeId) {
      case AKT_SAMPLER1D:
      case AKT_SAMPLER2D:
      case AKT_SAMPLER3D:
      case AKT_SAMPLER_CUBE:
      case AKT_SAMPLER_RECT:
      case AKT_SAMPLER_DEPTH:
        texture = param->val->value;
        dae_scenekit_normalize_sampler(texture->sampler);
        break;
      default:
        break;
    }
  }
}

static
void
dae_scenekit_normalize_technique_colors(
  AkTechniqueFxCommon * __restrict common) {
  if (!common)
    return;

  dae_scenekit_normalize_color_desc(common->ambient);
  if (common->emission)
    dae_scenekit_normalize_color_desc(&common->emission->color);
  dae_scenekit_normalize_color_desc(common->diffuse);
  dae_scenekit_normalize_color_desc(common->constantDiffuse);
  if (common->specular)
    dae_scenekit_normalize_color_desc(common->specular->color);
  if (common->reflective)
    dae_scenekit_normalize_color_desc(common->reflective->color);
  if (common->transparent)
    dae_scenekit_normalize_color_desc(common->transparent->color);
}

static
void
dae_scenekit_normalize_effect_colors(DAEState * __restrict dst) {
  AkEffect *effect;

  for (effect = dst->effects; effect; effect = effect->next) {
    AkProfile *profile;

    dae_scenekit_normalize_newparams(effect->newparam);
    for (profile = effect->profile; profile; profile = profile->next) {
      AkTechniqueFx *technique;

      dae_scenekit_normalize_newparams(profile->newparam);
      for (technique = profile->technique;
           technique;
           technique = technique->next) {
        dae_scenekit_normalize_technique_colors(technique->common);
      }
    }
  }
}

static
void
dae_scenekit_normalize_mesh_colors_fallback(AkDoc * __restrict doc) {
  AkAccessor *accessor;

  for (accessor = doc->lib.accessors.first;
       accessor;
       accessor = accessor->next) {
    if (dae_scenekit_accessor_is_color(doc, accessor))
      dae_scenekit_normalize_accessor_color(accessor);
  }
}

static
void
dae_scenekit_normalize_mesh_colors(AkDoc * __restrict doc) {
  AkAccessor  *stackSlots[64] = {0};
  AkAccessor **slots;
  AkGeometry  *geom;
  size_t       colorInputCount;
  size_t       capacity;

  colorInputCount = dae_scenekit_color_input_count(doc);
  if (!colorInputCount)
    return;

  slots    = stackSlots;
  capacity = AK_ARRAY_LEN(stackSlots);
  if (colorInputCount > capacity / 2u) {
    if (colorInputCount > SIZE_MAX / 2u) {
      dae_scenekit_normalize_mesh_colors_fallback(doc);
      return;
    }

    capacity = 1u;
    while (capacity < colorInputCount * 2u)
      capacity <<= 1u;

    slots = calloc(capacity, sizeof(*slots));
    if (!slots) {
      dae_scenekit_normalize_mesh_colors_fallback(doc);
      return;
    }
  }

  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    AkMesh          *mesh;
    AkMeshPrimitive *prim;

    if (!geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
      continue;

    mesh = ak_objGet(geom->gdata);
    for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next) {
      AkInput *input;

      for (input = prim->input; input; input = input->next) {
        if (input->semantic == AK_INPUT_COLOR
            && input->accessor
            && dae_scenekit_color_accessor_insert(slots,
                                                   capacity - 1u,
                                                   input->accessor)) {
          dae_scenekit_normalize_accessor_color(input->accessor);
        }
      }
    }
  }

  if (slots != stackSlots)
    free(slots);
}

AK_HIDE
void
dae_normalize_srgb_colors(DAEState * __restrict dst) {
  AkLight *light;

  if (!dae_srgb_color_authored(dst))
    return;

  if (dst->hasMeshColorInputs)
    dae_scenekit_normalize_mesh_colors(dst->doc);

  for (light = dst->doc->lib.lights.first; light; light = light->next) {
    if (light->data)
      dae_scenekit_normalize_color(&light->data->color);
  }

  dae_scenekit_normalize_effect_colors(dst);
}

static
AkColor*
dae_scenekit_desc_target(AkColorDesc * __restrict desc,
                         void        * __restrict target) {
  if (desc && desc->color == target)
    return desc->color;
  return NULL;
}

static
AkColor*
dae_scenekit_technique_color_target(
  AkTechniqueFxCommon * __restrict common,
  void                * __restrict target) {
  AkColor *color;

  if (!common)
    return NULL;

  if ((color = dae_scenekit_desc_target(common->ambient, target)))
    return color;
  if (common->emission
      && (color = dae_scenekit_desc_target(&common->emission->color, target)))
    return color;
  if ((color = dae_scenekit_desc_target(common->diffuse, target)))
    return color;
  if ((color = dae_scenekit_desc_target(common->constantDiffuse, target)))
    return color;
  if (common->specular
      && (color = dae_scenekit_desc_target(common->specular->color, target)))
    return color;
  if (common->reflective
      && (color = dae_scenekit_desc_target(common->reflective->color, target)))
    return color;
  if (common->transparent
      && (color = dae_scenekit_desc_target(common->transparent->color, target)))
    return color;

  return NULL;
}

static
AkColor*
dae_scenekit_newparam_color_target(AkNewParam * __restrict param,
                                   void       * __restrict target) {
  for (; param; param = param->next) {
    AkTexture *texture;

    if (!param->val || !param->val->value)
      continue;

    switch (param->val->type.typeId) {
      case AKT_SAMPLER1D:
      case AKT_SAMPLER2D:
      case AKT_SAMPLER3D:
      case AKT_SAMPLER_CUBE:
      case AKT_SAMPLER_RECT:
      case AKT_SAMPLER_DEPTH:
        texture = param->val->value;
        if (texture->sampler && texture->sampler->borderColor == target)
          return texture->sampler->borderColor;
        break;
      default:
        break;
    }
  }

  return NULL;
}

static
AkColor*
dae_scenekit_color_target(DAEState * __restrict dst,
                          void     * __restrict target) {
  AkEffect *effect;
  AkLight  *light;

  if (!dst || !target)
    return NULL;

  for (light = dst->doc->lib.lights.first; light; light = light->next) {
    if (light->data == target || (light->data && &light->data->color == target))
      return &light->data->color;
  }

  for (effect = dst->effects; effect; effect = effect->next) {
    AkColor   *color;
    AkProfile *profile;

    if ((color = dae_scenekit_newparam_color_target(effect->newparam, target)))
      return color;
    for (profile = effect->profile; profile; profile = profile->next) {
      AkTechniqueFx *technique;

      if ((color = dae_scenekit_newparam_color_target(profile->newparam,
                                                       target)))
        return color;
      for (technique = profile->technique;
           technique;
           technique = technique->next) {
        color = dae_scenekit_technique_color_target(technique->common, target);
        if (color)
          return color;
      }
    }
  }

  return NULL;
}

static
void
dae_scenekit_normalize_scalar_accessor(AkAccessor * __restrict accessor) {
  char    *row;
  size_t   stride;
  uint32_t i;

  if (!accessor
      || accessor->componentType != AKT_FLOAT
      || accessor->componentCount == 0u
      || !accessor->buffer
      || !accessor->buffer->data)
    return;

  stride = accessor->byteStride
           ? accessor->byteStride
           : (size_t)accessor->componentCount * sizeof(float);
  row = (char *)accessor->buffer->data + accessor->byteOffset;
  for (i = 0; i < accessor->count; i++) {
    float *value = (float *)(void *)row;
    value[0] = ak_srgb_to_linearf_fast(value[0]);
    row += stride;
  }
}

static
AkInput*
dae_scenekit_sampler_input(AkAnimSampler    * __restrict sampler,
                           AkInputSemantic               semantic) {
  AkInput *input;

  if (!sampler)
    return NULL;

  switch (semantic) {
    case AK_INPUT_INPUT:
      if (sampler->inputInput) return sampler->inputInput;
      break;
    case AK_INPUT_OUTPUT:
      if (sampler->outputInput) return sampler->outputInput;
      break;
    case AK_INPUT_IN_TANGENT:
      if (sampler->inTangentInput) return sampler->inTangentInput;
      break;
    case AK_INPUT_OUT_TANGENT:
      if (sampler->outTangentInput) return sampler->outTangentInput;
      break;
    case AK_INPUT_INTERPOLATION:
      if (sampler->interpInput) return sampler->interpInput;
      break;
    default: break;
  }

  for (input = sampler->input; input; input = input->next) {
    if (input->semantic == semantic)
      return input;
  }
  return NULL;
}

static
float*
dae_scenekit_accessor_row(AkAccessor * __restrict accessor, uint32_t row) {
  size_t stride;

  if (!accessor
      || accessor->componentType != AKT_FLOAT
      || !accessor->componentCount
      || row >= accessor->count
      || !accessor->buffer
      || !accessor->buffer->data)
    return NULL;

  stride = accessor->byteStride
           ? accessor->byteStride
           : (size_t)accessor->componentCount * sizeof(float);
  return (float *)(void *)((char *)accessor->buffer->data
                           + accessor->byteOffset
                           + (size_t)row * stride);
}

static
AkInterpolationType
dae_scenekit_sampler_interpolation(AkAnimSampler * __restrict sampler,
                                   uint32_t                    key,
                                   uint32_t                    component) {
  AkAccessor *accessor;
  AkInput    *input;
  const char *row;
  size_t      stride;
  uint32_t    index;

  if (!sampler)
    return AK_INTERPOLATION_UNKNOWN;
  if (sampler->uniInterpolation != AK_INTERPOLATION_UNKNOWN)
    return sampler->uniInterpolation;

  input    = dae_scenekit_sampler_input(sampler, AK_INPUT_INTERPOLATION);
  accessor = input ? input->accessor : NULL;
  if (!accessor
      || accessor->componentType != AKT_UBYTE
      || !accessor->componentCount
      || key >= accessor->count
      || !accessor->buffer
      || !accessor->buffer->data)
    return AK_INTERPOLATION_UNKNOWN;

  index  = component < accessor->componentCount ? component : 0u;
  stride = accessor->byteStride
           ? accessor->byteStride
           : (size_t)accessor->componentCount;
  row = (const char *)accessor->buffer->data
        + accessor->byteOffset
        + (size_t)key * stride;
  return (AkInterpolationType)(uint8_t)row[index];
}

static
bool
dae_scenekit_tangent_component(uint32_t tangentComponents,
                               uint32_t outputComponents,
                               uint32_t outputComponent,
                               uint32_t * __restrict tangentComponent) {
  uint32_t index;

  if (!outputComponents || outputComponent >= outputComponents)
    return false;

  /* Standard COLLADA animation tangents have one shared key/time followed
     by one value per OUTPUT component. Some older exporters omit the key,
     and a few write independent (time,value) pairs per component. */
  if (tangentComponents == outputComponents) {
    index = outputComponent;
  } else if (outputComponents <= UINT32_MAX / 2u
             && tangentComponents == outputComponents * 2u) {
    index = outputComponent * 2u + 1u;
  } else if (tangentComponents == outputComponents + 1u) {
    index = outputComponent + 1u;
  } else {
    return false;
  }

  if (index >= tangentComponents)
    return false;
  *tangentComponent = index;
  return true;
}

static
void
dae_scenekit_normalize_tangent_accessor(
  AkAnimSampler * __restrict sampler,
  AkAccessor    * __restrict tangent,
  AkAccessor    * __restrict output,
  bool                        partial,
  bool                        inTangent) {
  uint32_t colorComponents;
  uint32_t keyCount;
  uint32_t key;

  if (!tangent
      || !output
      || tangent->componentType != AKT_FLOAT
      || output->componentType != AKT_FLOAT
      || !tangent->componentCount
      || !output->componentCount
      || (!partial && output->componentCount < 3u))
    return;

  colorComponents = partial
                    ? 1u
                    : (output->componentCount < 3u
                       ? output->componentCount
                       : 3u);
  keyCount = tangent->count < output->count ? tangent->count : output->count;

  for (key = 0u; key < keyCount; key++) {
    float   *tangentRow;
    float   *outputRow;
    uint32_t outputComponent;
    uint32_t segment;

    tangentRow = dae_scenekit_accessor_row(tangent, key);
    outputRow  = dae_scenekit_accessor_row(output, key);
    if (!tangentRow || !outputRow)
      return;

    segment = inTangent && key > 0u ? key - 1u : key;
    for (outputComponent = 0u;
         outputComponent < colorComponents;
         outputComponent++) {
      AkInterpolationType interpolation;
      uint32_t            tangentComponent;

      if (!dae_scenekit_tangent_component(tangent->componentCount,
                                           output->componentCount,
                                           outputComponent,
                                           &tangentComponent))
        continue;

      interpolation = dae_scenekit_sampler_interpolation(sampler,
                                                          segment,
                                                          outputComponent);
      switch (interpolation) {
        case AK_INTERPOLATION_BEZIER:
          /* Bézier tangents store control-point coordinates. The time/key
             coordinate is deliberately excluded by tangentComponent(). */
          tangentRow[tangentComponent]
            = ak_srgb_to_linearf_fast(tangentRow[tangentComponent]);
          break;
        case AK_INTERPOLATION_HERMITE:
        case AK_INTERPOLATION_CARDINAL:
          /* Hermite/cardinal tangents are derivatives. Apply the chain rule
             at the corresponding still-sRGB key value. */
          tangentRow[tangentComponent]
            *= ak_srgb_to_linear_derivativef_fast(
                 outputRow[outputComponent]);
          break;
        default:
          break;
      }
    }
  }
}

typedef enum DaeSceneKitAnimationPass {
  DAE_SCENEKIT_ANIMATION_TANGENTS,
  DAE_SCENEKIT_ANIMATION_OUTPUTS
} DaeSceneKitAnimationPass;

static
void
dae_scenekit_normalize_animation_walk(DAEState    * __restrict dst,
                                      AkAnimation * __restrict animation,
                                      AkContext   * __restrict context,
                                      AkAccessor ** __restrict slots,
                                      size_t                    mask,
                                      DaeSceneKitAnimationPass  pass) {
  for (; animation; animation = animation->next) {
    AkChannel *channel;

    for (channel = animation->channel; channel; channel = channel->next) {
      AkResolvedTarget resolved;
      AkResolvedTarget *stored;
      AkAnimSampler *sampler;
      AkInput       *output;
      AkColor       *color;
      const char    *attribute;
      uint32_t       offset;
      bool           partial;

      attribute = NULL;
      memset(&resolved, 0, sizeof(resolved));
      if (channel->resolvedTarget) {
        resolved = *channel->resolvedTarget;
      } else if (channel->target) {
        resolved.target = ak_sid_resolve(context, channel->target, &attribute);
      }
      color = dae_scenekit_color_target(dst, resolved.target);
      if (!color) {
        if (attribute)
          ak_free((void *)attribute);
        continue;
      }

      partial = channel->resolvedTarget
                ? resolved.isPartial
                : attribute != NULL;
      offset = channel->resolvedTarget
               ? resolved.off
               : (attribute ? ak_sid_attr_offset(attribute) : 0u);
      if (partial && offset == UINT32_MAX) {
        ak_free((void *)attribute);
        continue;
      }

      if (!channel->resolvedTarget) {
        stored = ak_heap_calloc(dst->heap, channel, sizeof(*stored));
        stored->target    = color;
        stored->off       = offset;
        stored->isPartial = partial;
        channel->resolvedTarget = stored;
      }
      channel->targetType = partial ? AK_TARGET_FLOAT : AK_TARGET_COLOR;

      sampler = ak_getObjectByUrl(&channel->source);
      output  = dae_scenekit_sampler_input(sampler, AK_INPUT_OUTPUT);
      if (output && output->accessor && (!partial || offset < 3u)) {
        if (pass == DAE_SCENEKIT_ANIMATION_TANGENTS) {
          AkInput *inTangent;
          AkInput *outTangent;

          inTangent = dae_scenekit_sampler_input(sampler,
                                                  AK_INPUT_IN_TANGENT);
          if (inTangent
              && inTangent->accessor
              && dae_scenekit_color_accessor_insert(slots,
                                                     mask,
                                                     inTangent->accessor)) {
            dae_scenekit_normalize_tangent_accessor(sampler,
                                                     inTangent->accessor,
                                                     output->accessor,
                                                     partial,
                                                     true);
          }

          outTangent = dae_scenekit_sampler_input(sampler,
                                                   AK_INPUT_OUT_TANGENT);
          if (outTangent
              && outTangent->accessor
              && dae_scenekit_color_accessor_insert(slots,
                                                     mask,
                                                     outTangent->accessor)) {
            dae_scenekit_normalize_tangent_accessor(sampler,
                                                     outTangent->accessor,
                                                     output->accessor,
                                                     partial,
                                                     false);
          }
        } else if (dae_scenekit_color_accessor_insert(slots,
                                                       mask,
                                                       output->accessor)) {
          if (partial)
            dae_scenekit_normalize_scalar_accessor(output->accessor);
          else
            dae_scenekit_normalize_accessor_color(output->accessor);
        }
      }

      if (attribute)
        ak_free((void *)attribute);
    }

    if (animation->animation)
      dae_scenekit_normalize_animation_walk(dst,
                                            animation->animation,
                                            context,
                                            slots,
                                            mask,
                                            pass);
  }
}

static
size_t
dae_scenekit_animation_channel_count(AkAnimation * __restrict animation) {
  size_t count;

  count = 0u;
  for (; animation; animation = animation->next) {
    AkChannel *channel;

    for (channel = animation->channel; channel; channel = channel->next) {
      if (count == SIZE_MAX)
        return SIZE_MAX;
      count++;
    }
    if (animation->animation) {
      size_t childCount;

      childCount = dae_scenekit_animation_channel_count(animation->animation);
      if (childCount > SIZE_MAX - count)
        return SIZE_MAX;
      count += childCount;
    }
  }
  return count;
}

AK_HIDE
void
dae_normalize_srgb_animation_colors(DAEState * __restrict dst) {
  AkAccessor  *stackSlots[128] = {0};
  AkAccessor **slots;
  AkContext    context;
  size_t       capacity;
  size_t       channelCount;
  size_t       accessorCount;

  if (!dst
      || !dst->doc
      || !dst->doc->lib.animations.first
      || !dae_srgb_color_authored(dst))
    return;

  channelCount = dae_scenekit_animation_channel_count(
                   dst->doc->lib.animations.first);
  if (!channelCount || channelCount == SIZE_MAX)
    return;

  if (channelCount > SIZE_MAX / 2u)
    return;
  accessorCount = channelCount * 2u;

  slots    = stackSlots;
  capacity = AK_ARRAY_LEN(stackSlots);
  if (accessorCount > capacity / 2u) {
    if (accessorCount > SIZE_MAX / 2u)
      return;
    capacity = 1u;
    while (capacity < accessorCount * 2u) {
      if (capacity > SIZE_MAX / 2u)
        return;
      capacity <<= 1u;
    }
    slots = calloc(capacity, sizeof(*slots));
    if (!slots)
      return;
  }

  memset(&context, 0, sizeof(context));
  context.doc = dst->doc;
  dae_scenekit_normalize_animation_walk(dst,
                                        dst->doc->lib.animations.first,
                                        &context,
                                        slots,
                                        capacity - 1u,
                                        DAE_SCENEKIT_ANIMATION_TANGENTS);
  memset(slots, 0, capacity * sizeof(*slots));
  dae_scenekit_normalize_animation_walk(dst,
                                        dst->doc->lib.animations.first,
                                        &context,
                                        slots,
                                        capacity - 1u,
                                        DAE_SCENEKIT_ANIMATION_OUTPUTS);
  if (slots != stackSlots)
    free(slots);
}

static
bool
dae_colordesc_has_texture(DAEState    * __restrict dst,
                          AkColorDesc * __restrict color) {
  if (!color)
    return false;

  return color->texture || (dst->texmap && rb_find(dst->texmap, color));
}

static
bool
dae_scenekit_is_red_fill(AkMaterial          * __restrict material,
                         AkTechniqueFxCommon * __restrict common) {
  AkColor *color;

  if (!material
      || !material->name
      || !ak_str_eq_cstr_fast(material->name,
                              _s_dae_material,
                              _s_dae_material_len)
      || !common
      || !common->diffuse
      || !common->diffuse->color)
    return false;

  color = common->diffuse->color;

  return color->rgba.R > 0.45f
         && color->rgba.R < 0.90f
         && color->rgba.G < 0.40f
         && color->rgba.B < 0.50f
         && color->rgba.R > color->rgba.G + 0.20f
         && color->rgba.R > color->rgba.B + 0.15f
         && color->rgba.A > 0.95f;
}

static
AkTechniqueFxCommon*
dae_scenekit_primitive_common(DAEState            * __restrict dst,
                              AkInstanceGeometry  * __restrict instGeom,
                              AkMeshPrimitive     * __restrict prim,
                              AkMaterial         ** __restrict materialOut) {
  AkResolvedMaterial resolved;
  AkEffect          *effect;

  *materialOut = NULL;

  if (!ak_materialResolve(prim, instGeom, UINT32_MAX, &resolved)
      || !resolved.material)
    return NULL;

  effect = dae_material_effect(dst, resolved.material);
  if (!effect)
    return NULL;

  *materialOut = resolved.material;
  return ak_getProfileTechniqueCommon(effect);
}

static
bool
dae_scenekit_primitive_has_texture(DAEState           * __restrict dst,
                                   AkInstanceGeometry * __restrict instGeom,
                                   AkMeshPrimitive    * __restrict prim) {
  AkTechniqueFxCommon *common;
  AkMaterial          *material;

  common = dae_scenekit_primitive_common(dst, instGeom, prim, &material);

  return common && dae_colordesc_has_texture(dst, common->diffuse);
}

static
bool
dae_scenekit_is_textured_twin(DAEState           * __restrict dst,
                              AkInstanceGeometry * __restrict instGeom,
                              AkMeshPrimitive    * __restrict prim,
                              AkMeshPrimitive    * __restrict other) {
  return other
         && other != prim
         && other->type == AK_PRIMITIVE_TRIANGLES
         && other->nPolygons == prim->nPolygons
         && dae_scenekit_primitive_has_texture(dst, instGeom, other);
}

static
bool
dae_scenekit_should_drop_primitive(DAEState           * __restrict dst,
                                   AkInstanceGeometry * __restrict instGeom,
                                   AkMeshPrimitive    * __restrict prim,
                                   AkMeshPrimitive    * __restrict prev,
                                   AkMeshPrimitive    * __restrict next) {
  AkTechniqueFxCommon *common;
  AkMaterial          *material;

  if (prim->type != AK_PRIMITIVE_TRIANGLES || prim->nPolygons == 0)
    return false;

  common = dae_scenekit_primitive_common(dst, instGeom, prim, &material);
  if (!common || dae_colordesc_has_texture(dst, common->diffuse))
    return false;

  return dae_scenekit_is_red_fill(material, common)
         && (dae_scenekit_is_textured_twin(dst, instGeom, prim, prev)
             || dae_scenekit_is_textured_twin(dst, instGeom, prim, next));
}

static
void
dae_scenekit_unmap_material(AkGeometry      * __restrict geom,
                            AkMeshPrimitive * __restrict prim) {
  AkMapItem *head, *item;

  if (!geom || !geom->materialMap || !prim->bindmaterial)
    return;

  head = ak_map_findm(geom->materialMap, (void *)prim->bindmaterial);
  if (!head)
    return;

  item = head->data;
  while (item) {
    if (item->data == prim) {
      if (item->prev)
        item->prev->next = item->next;
      else
        head->data = item->next;

      if (item->next)
        item->next->prev = item->prev;

      return;
    }

    item = item->next;
  }
}

static
void
dae_scenekit_fix_mesh(DAEState           * __restrict dst,
                      AkInstanceGeometry * __restrict instGeom,
                      AkGeometry         * __restrict geom,
                      AkMesh             * __restrict mesh) {
  AkMeshPrimitive *prim, *prev, *next;

  prev = NULL;
  prim = mesh->primitive;

  while (prim) {
    next = prim->next;

    if (dae_scenekit_should_drop_primitive(dst, instGeom, prim, prev, next)) {
      if (prev)
        prev->next = next;
      else
        mesh->primitive = next;

      dae_scenekit_unmap_material(geom, prim);

      prim->next = NULL;
      if (mesh->primitiveCount > 0)
        mesh->primitiveCount--;

    } else {
      prev = prim;
    }

    prim = next;
  }
}

static
void
dae_scenekit_fix_node(DAEState * __restrict dst, AkNode * __restrict node) {
  AkInstanceGeometry *instGeom;
  AkGeometry         *geom;
  AkObject           *geomData;

  for (; node; node = node->next) {
    for (instGeom = node->geometry; instGeom;
         instGeom = (AkInstanceGeometry *)instGeom->base.next) {
      geom = ak_instanceObject(&instGeom->base);
      if (!geom || ak_typeid(geom) != AKT_GEOMETRY)
        continue;

      geomData = geom->gdata;
      if (!geomData || (AkGeometryType)geomData->type != AK_GEOMETRY_MESH)
        continue;

      dae_scenekit_fix_mesh(dst, instGeom, geom, ak_objGet(geomData));
    }

    if (node->chld)
      dae_scenekit_fix_node(dst, node->chld);

    if (node->node) {
      AkInstanceNode *instNode;

      for (instNode = node->node; instNode; instNode = instNode->next) {
        AkNode *target;

        target = ak_instanceNodeTarget(instNode);
        if (target)
          dae_scenekit_fix_node(dst, target);
      }
    }
  }
}

AK_HIDE
void
dae_bugfix_scenekit_material_surfaces(DAEState * __restrict dst) {
  AkMaterial *material;

  if (!dst
      || !dst->doc
      || !ak_opt_get(AK_OPT_BUGFIXES)
      || !dae_scenekit_state_authored(dst))
    return;

  for (material = dst->doc->lib.materials.first; material; material = material->next) {
    if (material->surface)
      material->surface->flags |= AK_MATERIAL_FLAG_DOUBLE_SIDED;
  }
}

AK_HIDE
void
dae_bugfix_scenekit_backfaces(DAEState * __restrict dst) {
  AkScene *vscn;

  if (!dst
      || !dst->doc
      || !ak_opt_get(AK_OPT_BUGFIXES)
      || !dae_scenekit_state_authored(dst)
      || !dst->doc->lib.scenes.first)
    return;

  for (vscn = dst->doc->lib.scenes.first;
       vscn;
       vscn = vscn->next) {
    dae_scenekit_fix_node(dst, vscn->node);
  }
}
