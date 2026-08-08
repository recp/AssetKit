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

#include "mesh.h"
#include "extra.h"
#include "image.h"
#include "material.h"
#include "plan.h"
#include "../strpool.h"

static
bool
gltf_primitive_core_extension_skip(const char * __restrict name,
                                   size_t                  nameLen,
                                   void * __restrict       userdata) {
  (void)userdata;
  return ak_str_eq_fast(name,
                        nameLen,
                        _s_gltf_KHR_materials_variants,
                        _s_gltf_KHR_materials_variants_len);
}

static
void
gltf_w_semantic_set_key(GLTFExpWriter * __restrict w,
                        const char    * __restrict base,
                        size_t                     baseLen,
                        uint32_t                   set) {
  gltf_w_ch(w, '"');
  gltf_w_raw(w, base, baseLen);
  gltf_w_ch(w, '_');
  gltf_w_uint(w, set);
  gltf_w_raw(w, "\":", 2);
}

static
bool
gltf_w_input_key(GLTFExpWriter * __restrict w,
                 GLTFExpState  * __restrict st,
                 AkMeshPrimitive * __restrict prim,
                 AkInput       * __restrict input) {
  uint32_t set;

  if (!input)
    return false;

  set = gltf_input_export_set(st, prim, input);

  switch (input->semantic) {
    case AK_INPUT_POSITION:
      gltf_w_key(w, _s_gltf_POSITION, _s_gltf_POSITION_len);
      return true;
    case AK_INPUT_NORMAL:
      gltf_w_key(w, _s_gltf_NORMAL, _s_gltf_NORMAL_len);
      return true;
    case AK_INPUT_TANGENT:
      gltf_w_key(w, _s_gltf_TANGENT, _s_gltf_TANGENT_len);
      return true;
    case AK_INPUT_TEXCOORD:
    case AK_INPUT_UV:
      gltf_w_semantic_set_key(w, _s_gltf_TEXCOORD, _s_gltf_TEXCOORD_len, set);
      return true;
    case AK_INPUT_COLOR:
      gltf_w_semantic_set_key(w, _s_gltf_COLOR, _s_gltf_COLOR_len, set);
      return true;
    case AK_INPUT_JOINT:
      gltf_w_semantic_set_key(w, _s_gltf_JOINTS, _s_gltf_JOINTS_len, set);
      return true;
    case AK_INPUT_WEIGHT:
      gltf_w_semantic_set_key(w, _s_gltf_WEIGHTS, _s_gltf_WEIGHTS_len, set);
      return true;
    default:
      break;
  }

  return false;
}

static
bool
gltf_input_key(GLTFExpState    * __restrict st,
               AkMeshPrimitive * __restrict prim,
               AkInput  * __restrict input,
               uint32_t * __restrict kind,
               uint32_t * __restrict set) {
  if (!input || !input->accessor)
    return false;

  *set = 0;
  switch (input->semantic) {
    case AK_INPUT_POSITION:
      *kind = 1;
      return true;
    case AK_INPUT_NORMAL:
      *kind = 2;
      return true;
    case AK_INPUT_TANGENT:
      *kind = 3;
      return true;
    case AK_INPUT_TEXCOORD:
    case AK_INPUT_UV:
      *kind = 4;
      *set  = gltf_input_export_set(st, prim, input);
      return true;
    case AK_INPUT_COLOR:
      *kind = 5;
      *set  = gltf_input_export_set(st, prim, input);
      return true;
    case AK_INPUT_JOINT:
      *kind = 6;
      *set  = gltf_input_export_set(st, prim, input);
      return true;
    case AK_INPUT_WEIGHT:
      *kind = 7;
      *set  = gltf_input_export_set(st, prim, input);
      return true;
    default:
      break;
  }

  return false;
}

static
bool
gltf_input_key_same(GLTFExpState    * __restrict st,
                    AkMeshPrimitive * __restrict prim,
                    AkInput * __restrict a,
                    AkInput * __restrict b) {
  uint32_t kindA;
  uint32_t kindB;
  uint32_t setA;
  uint32_t setB;

  if (!gltf_input_key(st, prim, a, &kindA, &setA)
      || !gltf_input_key(st, prim, b, &kindB, &setB))
    return false;

  return kindA == kindB && setA == setB;
}

static
bool
gltf_input_key_seen_before(GLTFExpState    * __restrict st,
                           AkMeshPrimitive * __restrict prim,
                           AkInput * __restrict first,
                           AkInput * __restrict input,
                           AkInput * __restrict posInput) {
  AkInput *scan;

  for (scan = first; scan && scan != input; scan = scan->next) {
    if (!scan->accessor
        || !gltf_input_supported(scan)
        || !gltf_normal_input_valid(st, scan)
        || (posInput && !gltf_input_count_valid(st, prim, scan, posInput)))
      continue;
    if (gltf_input_key_same(st, prim, scan, input))
      return true;
  }

  return false;
}

static
bool
gltf_write_primitive_attributes(GLTFExpWriter  * __restrict w,
                                GLTFExpState   * __restrict st,
                                AkMeshPrimitive * __restrict prim,
                                AkNode          * __restrict bakeNode,
                                GLTFExpSkinAttrOut * __restrict skinAttr) {
  AkInput *input;
  AkInput *posInput;
  bool     any;

  gltf_w_key(w, _s_gltf_attributes, _s_gltf_attributes_len);
  gltf_w_ch(w, '{');

  any      = false;
  posInput = gltf_primitive_position_input(prim);

  if (posInput && posInput->accessor) {
    GLTFExpIndex idx;

    idx = gltf_baked_accessor_index(st, bakeNode, prim, AK_INPUT_POSITION);
    if (idx == GLTF_EXP_INDEX_NONE)
      idx = gltf_position_accessor_index(st, prim);
    if (idx == GLTF_EXP_INDEX_NONE)
      idx = gltf_input_accessor_index(&st->accessors, posInput);
    if (idx == GLTF_EXP_INDEX_NONE)
      idx = gltf_accessor_index(&st->accessors, posInput->accessor);
    if (idx != GLTF_EXP_INDEX_NONE) {
      gltf_w_key(w, _s_gltf_POSITION, _s_gltf_POSITION_len);
      gltf_w_uint(w, idx);
      any = true;
    }
  }

  for (input = prim->input; input; input = input->next) {
    GLTFExpIndex idx;

    if (!input->accessor || input == posInput
        || input->semantic == AK_INPUT_POSITION
        || !gltf_normal_input_valid(st, input)
        || !gltf_input_count_valid(st, prim, input, posInput)
        || gltf_input_key_seen_before(st,
                                      prim,
                                      prim->input,
                                      input,
                                      posInput))
      continue;

    idx = input->semantic == AK_INPUT_NORMAL
          ? gltf_baked_accessor_index(st, bakeNode, prim, AK_INPUT_NORMAL)
          : GLTF_EXP_INDEX_NONE;
    if (idx == GLTF_EXP_INDEX_NONE) {
      if (!gltf_input_supported(input))
        continue;
      idx = gltf_raw_accessor_index(&st->accessors, input);
      if (idx == GLTF_EXP_INDEX_NONE)
        idx = gltf_input_accessor_index(&st->accessors, input);
      if (idx == GLTF_EXP_INDEX_NONE)
        idx = gltf_accessor_index(&st->accessors, input->accessor);
    }
    if (idx == GLTF_EXP_INDEX_NONE)
      continue;

    if (any)
      gltf_w_ch(w, ',');

    if (!gltf_w_input_key(w, st, prim, input))
      continue;

    gltf_w_uint(w, idx);
    any = true;
  }

  if (skinAttr
      && skinAttr->jointsAccessorIndex != GLTF_EXP_INDEX_NONE
      && skinAttr->weightsAccessorIndex != GLTF_EXP_INDEX_NONE) {
    if (any)
      gltf_w_ch(w, ',');
    gltf_w_semantic_set_key(w, _s_gltf_JOINTS, _s_gltf_JOINTS_len, 0);
    gltf_w_uint(w, skinAttr->jointsAccessorIndex);
    gltf_w_ch(w, ',');
    gltf_w_semantic_set_key(w, _s_gltf_WEIGHTS, _s_gltf_WEIGHTS_len, 0);
    gltf_w_uint(w, skinAttr->weightsAccessorIndex);
    any = true;
  }

  gltf_w_ch(w, '}');

  return any;
}

static
bool
gltf_morph_input_supported(AkInput * __restrict input) {
  if (!input)
    return false;

  switch (input->semantic) {
    case AK_INPUT_POSITION:
    case AK_INPUT_NORMAL:
    case AK_INPUT_TANGENT:
      return true;
    default:
      break;
  }

  return false;
}

static
AkMorphable*
gltf_morphable_at(AkMorphTarget * __restrict target, uint32_t primIndex) {
  AkMorphable *morphable;
  uint32_t     i;

  if (!target
      || !target->target
      || target->target->type != AK_MORPHABLE_MORPHABLE)
    return NULL;

  morphable = ak_objGet(target->target);
  for (i = 0; morphable && i < primIndex; i++)
    morphable = morphable->next;

  return morphable;
}

static
AkMeshPrimitive*
gltf_geometry_primitive_at(AkGeometry * __restrict geom, uint32_t primIndex) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  uint32_t         i;

  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return NULL;

  mesh = ak_objGet(geom->gdata);
  if (!mesh)
    return NULL;

  prim = mesh->primitive;
  for (i = 0; prim && i < primIndex; i++)
    prim = prim->next;

  return prim;
}

static
bool
gltf_morph_geometry_target_has_attributes(GLTFExpState    * __restrict st,
                                          AkMeshPrimitive * __restrict prim) {
  AkInput *input;

  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if (!input->accessor || !gltf_morph_input_supported(input))
      continue;
    if (gltf_accessor_index(&st->accessors, input->accessor) != GLTF_EXP_INDEX_NONE)
      return true;
  }

  return false;
}

static
bool
gltf_morph_target_has_attributes(GLTFExpState * __restrict st,
                                 AkMorphable  * __restrict morphable) {
  AkInput *input;

  for (input = morphable ? morphable->input : NULL; input; input = input->next) {
    if (!input->accessor
        || !gltf_morph_input_supported(input)
        || gltf_input_key_seen_before(st,
                                      NULL,
                                      morphable->input,
                                      input,
                                      NULL))
      continue;
    if (gltf_accessor_index(&st->accessors, input->accessor) != GLTF_EXP_INDEX_NONE)
      return true;
  }

  return false;
}

static
bool
gltf_write_generated_morph_target(GLTFExpWriter       * __restrict w,
                                  GLTFExpMorphAttrOut * __restrict attr) {
  bool any;

  if (!attr)
    return false;

  gltf_w_ch(w, '{');
  any = false;

  if (attr->positionAccessorIndex != GLTF_EXP_INDEX_NONE) {
    gltf_w_key(w, _s_gltf_POSITION, _s_gltf_POSITION_len);
    gltf_w_uint(w, attr->positionAccessorIndex);
    any = true;
  }

  if (attr->normalAccessorIndex != GLTF_EXP_INDEX_NONE) {
    if (any)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_NORMAL, _s_gltf_NORMAL_len);
    gltf_w_uint(w, attr->normalAccessorIndex);
    any = true;
  }

  if (attr->tangentAccessorIndex != GLTF_EXP_INDEX_NONE) {
    if (any)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_TANGENT, _s_gltf_TANGENT_len);
    gltf_w_uint(w, attr->tangentAccessorIndex);
    any = true;
  }

  gltf_w_ch(w, '}');

  return any;
}

static
bool
gltf_write_morph_geometry_target(GLTFExpWriter  * __restrict w,
                                 GLTFExpState   * __restrict st,
                                 AkMeshPrimitive * __restrict prim) {
  AkInput *input;
  bool     any;

  gltf_w_ch(w, '{');
  any = false;

  for (input = prim ? prim->input : NULL; input; input = input->next) {
    GLTFExpIndex idx;

    if (!input->accessor
        || !gltf_morph_input_supported(input)
        || gltf_input_key_seen_before(st, NULL, prim->input, input, NULL))
      continue;

    idx = gltf_accessor_index(&st->accessors, input->accessor);
    if (idx == GLTF_EXP_INDEX_NONE)
      continue;

    if (any)
      gltf_w_ch(w, ',');
    if (!gltf_w_input_key(w, st, NULL, input))
      continue;
    gltf_w_uint(w, idx);
    any = true;
  }

  gltf_w_ch(w, '}');

  return any;
}

static
bool
gltf_write_morph_target(GLTFExpWriter * __restrict w,
                        GLTFExpState  * __restrict st,
                        AkMorphable   * __restrict morphable) {
  AkInput *input;
  bool     any;

  gltf_w_ch(w, '{');
  any = false;
  for (input = morphable ? morphable->input : NULL; input; input = input->next) {
    GLTFExpIndex idx;

    if (!input->accessor
        || !gltf_morph_input_supported(input)
        || gltf_input_key_seen_before(st,
                                      NULL,
                                      morphable->input,
                                      input,
                                      NULL))
      continue;

    idx = gltf_accessor_index(&st->accessors, input->accessor);
    if (idx == GLTF_EXP_INDEX_NONE)
      continue;

    if (any)
      gltf_w_ch(w, ',');
    if (!gltf_w_input_key(w, st, NULL, input))
      continue;
    gltf_w_uint(w, idx);
    any = true;
  }
  gltf_w_ch(w, '}');

  return any;
}

static
bool
gltf_write_primitive_targets(GLTFExpWriter      * __restrict w,
                             GLTFExpState       * __restrict st,
                             AkMorph            * __restrict morph,
                             uint32_t                         primIndex,
                             GLTFExpMorphAttrOut * __restrict morphAttrs,
                             uint32_t                         morphAttrPrimCount,
                             bool               * __restrict outerComma) {
  AkMorphTarget *target;
  bool           comma;
  bool           hasTarget;
  uint32_t       targetIndex;

  if (!morph)
    return false;

  if (morph->method != AK_MORPH_METHOD_RELATIVE
      && morph->method != AK_MORPH_METHOD_ADDITIVE
      && morph->method != AK_MORPH_METHOD_NORMALIZED) {
    w->result = AK_ERR;
    return false;
  }

  hasTarget = false;
  targetIndex = 0;
  for (target = morph->target; target; target = target->next, targetIndex++) {
    AkMorphable *morphable;

    if (target->target && target->target->type == AK_MORPHABLE_GEOMETRY) {
      if (morph->method == AK_MORPH_METHOD_NORMALIZED) {
        GLTFExpMorphAttrOut *attr;

        attr = morphAttrs && morphAttrPrimCount > 0
               ? &morphAttrs[(size_t)targetIndex * morphAttrPrimCount
                            + primIndex]
               : NULL;
        if (!attr || attr->positionAccessorIndex == GLTF_EXP_INDEX_NONE)
          return false;
      } else {
        AkMeshPrimitive *targetPrim;

        targetPrim = gltf_geometry_primitive_at(ak_objGetTarget(target->target),
                                                primIndex);
        if (!gltf_morph_geometry_target_has_attributes(st, targetPrim))
          return false;
      }
      hasTarget = true;
      continue;
    }

    morphable = gltf_morphable_at(target, primIndex);
    if (!morphable || !gltf_morph_target_has_attributes(st, morphable))
      return false;
    hasTarget = true;
  }
  if (!hasTarget)
    return false;

  if (*outerComma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_targets, _s_gltf_targets_len);
  gltf_w_ch(w, '[');

  comma = false;
  targetIndex = 0;
  for (target = morph->target; target; target = target->next, targetIndex++) {
    AkMorphable *morphable;
    bool         wrote;

    if (comma)
      gltf_w_ch(w, ',');

    wrote = false;
    if (target->target && target->target->type == AK_MORPHABLE_GEOMETRY) {
      if (morph->method == AK_MORPH_METHOD_NORMALIZED) {
        GLTFExpMorphAttrOut *attr;

        attr = morphAttrs && morphAttrPrimCount > 0
               ? &morphAttrs[(size_t)targetIndex * morphAttrPrimCount
                            + primIndex]
               : NULL;
        wrote = gltf_write_generated_morph_target(w, attr);
      } else {
        AkMeshPrimitive *targetPrim;

        targetPrim = gltf_geometry_primitive_at(ak_objGetTarget(target->target),
                                                primIndex);
        wrote = gltf_write_morph_geometry_target(w, st, targetPrim);
      }
    } else {
      morphable = gltf_morphable_at(target, primIndex);
      wrote     = morphable && gltf_write_morph_target(w, st, morphable);
    }

    if (!wrote) {
      w->result = AK_ERR;
      return false;
    }
    comma = true;
  }

  gltf_w_ch(w, ']');
  *outerComma = true;

  return true;
}

static
bool
gltf_material_texture_ref_compatible(GLTFExpState       * __restrict st,
                                     AkMeshPrimitive    * __restrict prim,
                                     AkInstanceGeometry * __restrict inst,
                                     AkTextureRef       * __restrict texref,
                                     bool                             plannedOnly) {
  int32_t slot;

  if (!texref || !texref->texture)
    return true;

  if (plannedOnly) {
    if (!st || gltf_ptrs_index(&st->textures, texref->texture) == GLTF_EXP_INDEX_NONE)
      return true;
  } else if (!st
             || !texref->texture->image
             || !gltf_image_exportable(st, texref->texture->image)) {
    return true;
  }

  slot = ak_materialTextureSlot(prim, inst, texref);
  if (slot < 0)
    slot = 0;

  return gltf_texcoord_source_set_valid(st, prim, slot);
}

static
bool
gltf_material_input_compatible(GLTFExpState       * __restrict st,
                               AkMeshPrimitive    * __restrict prim,
                               AkInstanceGeometry * __restrict inst,
                               AkMaterialInput    * __restrict input,
                               bool                             plannedOnly) {
  return gltf_material_texture_ref_compatible(st,
                                              prim,
                                              inst,
                                              ak_materialInputTexture(input),
                                              plannedOnly);
}

static
bool
gltf_material_feature_compatible(GLTFExpState       * __restrict st,
                                 AkMeshPrimitive    * __restrict prim,
                                 AkInstanceGeometry * __restrict inst,
                                 AkMaterialFeature  * __restrict feature,
                                 bool                             plannedOnly) {
  switch (feature->type) {
    case AK_MATERIAL_FEATURE_CLEARCOAT: {
      AkMaterialClearcoatFeature *f = (AkMaterialClearcoatFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->factor, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->roughness, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->normal, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_SPECULAR: {
      AkMaterialSpecularFeature *f = (AkMaterialSpecularFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->factor, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->color, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS: {
      AkMaterialSpecularGlossinessFeature *f;
      f = (AkMaterialSpecularGlossinessFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->diffuse, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->specular, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->glossiness, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_TRANSMISSION: {
      AkMaterialTransmissionFeature *f = (AkMaterialTransmissionFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->factor, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_SHEEN: {
      AkMaterialSheenFeature *f = (AkMaterialSheenFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->color, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->roughness, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_IRIDESCENCE: {
      AkMaterialIridescenceFeature *f = (AkMaterialIridescenceFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->factor, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->thickness, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_VOLUME: {
      AkMaterialVolumeFeature *f = (AkMaterialVolumeFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->thickness, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_ANISOTROPY: {
      AkMaterialAnisotropyFeature *f = (AkMaterialAnisotropyFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->strength, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->rotation, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION: {
      AkMaterialDiffuseTransmissionFeature *f;
      f = (AkMaterialDiffuseTransmissionFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->factor, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->color, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_SUBSURFACE: {
      AkMaterialSubsurfaceFeature *f = (AkMaterialSubsurfaceFeature *)feature;
      return gltf_material_input_compatible(st, prim, inst, f->weight, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->color, plannedOnly)
             && gltf_material_input_compatible(st, prim, inst, f->radius, plannedOnly);
    }
    case AK_MATERIAL_FEATURE_CLASSIC: {
      return true;
    }
    default:
      break;
  }

  return true;
}

bool
gltf_material_surface_compatible(GLTFExpState       * __restrict st,
                                 AkMeshPrimitive    * __restrict prim,
                                 AkInstanceGeometry * __restrict inst,
                                 AkMaterialSurface  * __restrict surface,
                                 bool                             plannedOnly) {
  AkMaterialFeature *feature;

  if (!surface)
    return true;

  if (!gltf_material_input_compatible(st, prim, inst, surface->baseColor, plannedOnly)
      || !gltf_material_input_compatible(st, prim, inst, surface->metallic, plannedOnly)
      || !gltf_material_input_compatible(st, prim, inst, surface->roughness, plannedOnly)
      || !gltf_material_input_compatible(st, prim, inst, surface->normal, plannedOnly)
      || !gltf_material_input_compatible(st, prim, inst, surface->occlusion, plannedOnly)
      || !gltf_material_input_compatible(st, prim, inst, surface->emissive, plannedOnly))
    return false;

  for (feature = surface->features; feature; feature = feature->next) {
    if (!gltf_material_feature_compatible(st, prim, inst, feature, plannedOnly))
      return false;
  }

  return true;
}

static
bool
gltf_variant_mapping_writable(GLTFExpState              * __restrict st,
                              AkMeshPrimitive           * __restrict prim,
                              AkInstanceGeometry        * __restrict inst,
                              AkMaterialVariantMapping  * __restrict mapping,
                              GLTFExpIndex              * __restrict materialIndex) {
  if (gltf_material_is_default_noop(mapping->material))
    return false;

  if (mapping->material
      && !gltf_material_surface_compatible(st,
                                           prim,
                                           inst,
                                           mapping->material->surface,
                                           true))
    return false;

  if (mapping->variantIndex >= st->materialVariantCount) {
    st->failed     = true;
    st->failResult = AK_EINVAL;
    return false;
  }

  *materialIndex = gltf_material_index(st, mapping->material, prim, inst);
  if (*materialIndex == GLTF_EXP_INDEX_NONE) {
    st->failed = true;
    return false;
  }

  return true;
}

static
bool
gltf_write_primitive_variants(GLTFExpWriter      * __restrict w,
                              GLTFExpState       * __restrict st,
                              AkMeshPrimitive    * __restrict prim,
                              AkInstanceGeometry * __restrict inst,
                              bool               * __restrict outerComma) {
  AkMaterialVariantMapping *mapping;
  GLTFExpIndex              materialIndex;
  bool                      comma;
  bool                      any;

  if (!prim->variantMappings)
    return false;

  if (!gltf_has_material_variants(st))
    return false;

  any = false;
  for (mapping = prim->variantMappings; mapping; mapping = mapping->next) {
    if (gltf_variant_mapping_writable(st,
                                      prim,
                                      inst,
                                      mapping,
                                      &materialIndex))
      any = true;
    if (st->failed) {
      w->result = st->failResult != AK_OK ? st->failResult : AK_ERR;
      return false;
    }
  }

  if (!any)
    return false;

  if (*outerComma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '{');
  gltf_w_key(w,
             _s_gltf_KHR_materials_variants,
             _s_gltf_KHR_materials_variants_len);
  gltf_w_ch(w, '{');
  gltf_w_key(w, _s_gltf_mappings, _s_gltf_mappings_len);
  gltf_w_ch(w, '[');

  comma = false;
  for (mapping = prim->variantMappings; mapping; mapping = mapping->next) {
    if (!gltf_variant_mapping_writable(st,
                                       prim,
                                       inst,
                                       mapping,
                                       &materialIndex)) {
      if (st->failed) {
        w->result = st->failResult != AK_OK ? st->failResult : AK_ERR;
        return false;
      }
      continue;
    }

    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_ch(w, '{');
    gltf_w_key_uint(w, _s_gltf_material, _s_gltf_material_len,
                    materialIndex);
    gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_variants, _s_gltf_variants_len);
    gltf_w_ch(w, '[');
    gltf_w_uint(w, mapping->variantIndex);
    gltf_w_ch(w, ']');
    gltf_w_ch(w, '}');
    comma = true;
  }

  gltf_w_ch(w, ']');
  gltf_w_ch(w, '}');
  gltf_write_extra_extension_entries(w,
                                     prim->extra,
                                     gltf_primitive_core_extension_skip,
                                     NULL,
                                     &comma);
  gltf_w_ch(w, '}');
  *outerComma = true;

  return true;
}

static
void
gltf_write_primitive(GLTFExpWriter      * __restrict w,
                     GLTFExpState       * __restrict st,
                     AkMeshPrimitive    * __restrict prim,
                     AkInstanceGeometry * __restrict inst,
                     AkNode             * __restrict bakeNode,
                     AkMorph            * __restrict morph,
                     GLTFExpSkinAttrOut * __restrict skinAttr,
                     GLTFExpMorphAttrOut * __restrict morphAttrs,
                     uint32_t                         morphAttrPrimCount,
                     uint32_t                         primIndex) {
  AkResolvedMaterial resolved;
  GLTFExpIndex      mode;
  bool               comma;
  GLTFExpIndex      idx;

  comma = false;
  gltf_w_ch(w, '{');

  if (!gltf_write_primitive_attributes(w, st, prim, bakeNode, skinAttr)) {
    w->result = AK_ERR;
    gltf_w_ch(w, '}');
    return;
  }
  comma = true;

  idx = prim->indexAccessor
        ? gltf_accessor_index(&st->accessors, prim->indexAccessor)
        : gltf_prim_index_accessor_index(&st->accessors, prim);
  if (idx != GLTF_EXP_INDEX_NONE) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key_uint(w, _s_gltf_indices, _s_gltf_indices_len, idx);
    comma = true;
  }

  if (!gltf_primitive_mode(prim, &mode)) {
    w->result = AK_ERR;
    gltf_w_ch(w, '}');
    return;
  }

  if (mode != 4) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key_uint(w, _s_gltf_mode, _s_gltf_mode_len, mode);
    comma = true;
  }

  if (ak_materialResolve(prim, inst, UINT32_MAX, &resolved)
      && !gltf_material_is_default_noop(resolved.material)
      && gltf_material_surface_compatible(st,
                                          prim,
                                          inst,
                                          resolved.surface,
                                          true)) {
    idx = gltf_material_index(st, resolved.material, prim, inst);
    if (idx != GLTF_EXP_INDEX_NONE) {
      if (comma)
        gltf_w_ch(w, ',');
      gltf_w_key_uint(w, _s_gltf_material, _s_gltf_material_len, idx);
      comma = true;
    }
  }

  gltf_write_primitive_targets(w,
                               st,
                               morph,
                               primIndex,
                               morphAttrs,
                               morphAttrPrimCount,
                               &comma);
  if (!gltf_write_primitive_variants(w, st, prim, inst, &comma))
    gltf_write_extra_extensions_member(w,
                                       &comma,
                                       prim->extra,
                                       gltf_primitive_core_extension_skip,
                                       NULL);

  if (gltf_extra_has_json_extras(prim->extra)) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_extras, _s_gltf_extras_len);
    gltf_write_extra_json_extras(w, prim->extra);
  }

  gltf_w_ch(w, '}');
}

static
AkFloatArray*
gltf_mesh_weights(AkMesh * __restrict mesh, AkMorph * __restrict morph) {
  if (morph && morph->defaultWeights)
    return morph->defaultWeights;

  return mesh ? mesh->weights : NULL;
}

static
bool
gltf_write_weights(GLTFExpWriter * __restrict w,
                   AkFloatArray  * __restrict weights,
                   uint32_t                   count) {
  uint32_t i;

  if (!weights)
    return false;

  if (weights->count < count) {
    w->result = AK_ERR;
    return false;
  }

  gltf_w_key(w, _s_gltf_weights, _s_gltf_weights_len);
  gltf_w_ch(w, '[');
  for (i = 0; i < count; i++) {
    if (i > 0)
      gltf_w_ch(w, ',');
    gltf_w_float(w, weights->items[i]);
  }
  gltf_w_ch(w, ']');

  return true;
}

static
bool
gltf_write_morph_extras(GLTFExpWriter * __restrict w,
                        AkMorph       * __restrict morph) {
  uint32_t i;
  bool     comma;

  if (!morph || (!morph->targetNames && !morph->presets))
    return false;

  gltf_w_key(w, _s_gltf_extras, _s_gltf_extras_len);
  gltf_w_ch(w, '{');
  comma = false;

  if (morph->targetNames) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_targetNames, _s_gltf_targetNames_len);
    gltf_w_ch(w, '[');
    for (i = 0; i < morph->targetCount; i++) {
      if (i > 0)
        gltf_w_ch(w, ',');
      gltf_w_qstr(w, morph->targetNames[i]);
    }
    gltf_w_ch(w, ']');
    comma = true;
  }

  if (morph->presets && morph->presetCount > 0) {
    uint32_t presetIndex;

    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_morphPresets, _s_gltf_morphPresets_len);
    gltf_w_ch(w, '[');
    for (presetIndex = 0; presetIndex < morph->presetCount; presetIndex++) {
      AkMorphPreset *preset;
      bool           presetComma;

      preset = &morph->presets[presetIndex];
      if (presetIndex > 0)
        gltf_w_ch(w, ',');
      gltf_w_ch(w, '{');
      presetComma = false;
      if (preset->name) {
        gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len, preset->name);
        presetComma = true;
      }
      if (preset->weights) {
        if (presetComma)
          gltf_w_ch(w, ',');
        gltf_write_weights(w, preset->weights, morph->targetCount);
      }
      gltf_w_ch(w, '}');
    }
    gltf_w_ch(w, ']');
  }

  gltf_w_ch(w, '}');

  return true;
}

void
gltf_write_meshes(GLTFExpWriter * __restrict w,
                  GLTFExpState  * __restrict st) {
  size_t i;

  if (st->meshes.count == 0)
    return;

  gltf_w_key(w, _s_gltf_meshes, _s_gltf_meshes_len);
  gltf_w_ch(w, '[');

  for (i = 0; i < st->meshes.count; i++) {
    GLTFExpMeshOut *meshOut;
    AkGeometry     *geom;
    AkMesh         *mesh;
    AkMeshPrimitive *prim;
    AkMorph        *morph;
    bool            comma;
    bool            primComma;
    uint32_t        primIndex;

    meshOut = &st->meshes.items[i];
    geom    = meshOut->geom;
    mesh    = ak_objGet(geom->gdata);
    morph   = meshOut->instance && meshOut->instance->morpher
              ? meshOut->instance->morpher->morph
              : NULL;

    if (i > 0)
      gltf_w_ch(w, ',');

    comma = false;
    gltf_w_ch(w, '{');

    if (geom->name || mesh->name) {
      gltf_w_key_str(w, _s_gltf_name, _s_gltf_name_len,
                     geom->name ? geom->name : mesh->name);
      comma = true;
    }

    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, _s_gltf_primitives, _s_gltf_primitives_len);
    gltf_w_ch(w, '[');

    primComma = false;
    primIndex = 0;
    for (prim = mesh->primitive; prim; prim = prim->next, primIndex++) {
      GLTFExpSkinAttrOut *skinAttr;
      GLTFExpMorphAttrOut *morphAttrs;

      skinAttr = NULL;
      if (meshOut->skinAttrOffset != GLTF_EXP_INDEX_NONE
          && primIndex < meshOut->skinAttrCount)
        skinAttr = &st->skinAttrs.items[meshOut->skinAttrOffset + primIndex];

      morphAttrs = meshOut->morphAttrOffset != GLTF_EXP_INDEX_NONE
                   ? &st->morphAttrs.items[meshOut->morphAttrOffset]
                   : NULL;

      if (primComma)
        gltf_w_ch(w, ',');
      primComma = true;
      gltf_write_primitive(w,
                           st,
                           prim,
                           meshOut->instance,
                           meshOut->bakeNode,
                           morph,
                           skinAttr,
                           morphAttrs,
                           meshOut->morphAttrPrimCount,
                           primIndex);
    }

    gltf_w_ch(w, ']');
    comma = true;

    if (morph) {
      AkFloatArray *weights;

      weights = gltf_mesh_weights(mesh, morph);
      if (weights) {
        if (comma)
          gltf_w_ch(w, ',');
        gltf_write_weights(w, weights, morph->targetCount);
        comma = true;
      }

      if (morph && (morph->targetNames || morph->presets)) {
        if (comma)
          gltf_w_ch(w, ',');
        gltf_write_morph_extras(w, morph);
        comma = true;
      }
    }

    {
      AkTree *extra;

      extra = mesh ? mesh->extra : NULL;
      if (!extra && geom)
        extra = geom->extra;

      gltf_write_extra_extensions_member(w, &comma, extra, NULL, NULL);

      if (gltf_extra_has_json_extras(extra)) {
        if (comma)
          gltf_w_ch(w, ',');
        gltf_w_key(w, _s_gltf_extras, _s_gltf_extras_len);
        gltf_write_extra_json_extras(w, extra);
      }
    }

    gltf_w_ch(w, '}');
  }

  gltf_w_ch(w, ']');
}
