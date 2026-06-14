/*
 * Material inspection sample.
 *
 * Shows the canonical AssetKit material surface: base PBR/classic type,
 * scalar factors, texture inputs, channel masks, variants, and 3MF-style
 * material/property sets when present.
 */

#include "sample_common.h"

#include <inttypes.h>
#include <stdio.h>

static void
sample_print_texture_ref(const AkTextureRef *texref) {
  const AkTexture *texture;
  const AkImage *image;
  const AkImageSource *source;

  if (!texref || !texref->texture) {
    printf(" texture=no");
    return;
  }

  texture = texref->texture;
  image   = texture->image;
  source  = image ? image->source : NULL;

  printf(" texture=yes");
  printf(" texture_name=%s", sample_or_unnamed(texture->name));
  printf(" image=%s", image ? sample_or_unnamed(image->name) : "(none)");
  printf(" slot=%d", texref->slot);
  printf(" texcoord=%s",
         texref->coordInputName ? texref->coordInputName
                                : (texref->texcoord ? texref->texcoord : "(default)"));
  printf(" channels=%s", sample_texture_channels_name(texref->channels));
  printf(" color_space=%s", sample_texture_color_space_name(texref->colorSpace));
  if (source && source->uri)
    printf(" uri=%s", source->uri);
  else if (source && source->resolvedPath)
    printf(" path=%s", source->resolvedPath);
  else if (source && source->buffer)
    printf(" source=buffer");
}

static void
sample_print_input(const char *label, const AkMaterialInput *input) {
  if (!input) {
    printf("    %-10s none\n", label);
    return;
  }

  printf("    %-10s source=%s value_type=%s semantic=%s source_name=%s",
         label,
         sample_material_input_source_name(input->source),
         sample_material_input_value_name(input->valueType),
         input->semantic ? input->semantic : "(none)",
         input->sourceName ? input->sourceName : "(none)");
  printf(" channels=%s", sample_texture_channels_name(input->channels));
  printf(" color_space=%s", sample_texture_color_space_name(input->colorSpace));
  if (input->flags)
    printf(" flags=0x%x", (unsigned)input->flags);

  switch (input->valueType) {
    case AK_MATERIAL_VALUE_FLOAT:
      printf(" value=%.6g", input->value[0]);
      break;
    case AK_MATERIAL_VALUE_FLOAT2:
      printf(" value=(%.6g, %.6g)", input->value[0], input->value[1]);
      break;
    case AK_MATERIAL_VALUE_FLOAT3:
      printf(" value=(%.6g, %.6g, %.6g)",
             input->value[0], input->value[1], input->value[2]);
      break;
    case AK_MATERIAL_VALUE_FLOAT4:
      printf(" value=(%.6g, %.6g, %.6g, %.6g)",
             input->value[0], input->value[1], input->value[2], input->value[3]);
      break;
    case AK_MATERIAL_VALUE_COLOR:
      printf(" color=(%.6g, %.6g, %.6g, %.6g)",
             input->color.rgba.R,
             input->color.rgba.G,
             input->color.rgba.B,
             input->color.rgba.A);
      break;
    case AK_MATERIAL_VALUE_INDEX:
      printf(" index=%u", input->index);
      break;
    default:
      break;
  }

  sample_print_texture_ref(input->texture);
  printf("\n");
}

static void
sample_print_feature_inputs(const AkMaterialFeature *feature) {
  switch (feature->type) {
    case AK_MATERIAL_FEATURE_CLEARCOAT: {
      const AkMaterialClearcoatFeature *f = (const AkMaterialClearcoatFeature *)feature;
      sample_print_input("factor", f->factor);
      sample_print_input("roughness", f->roughness);
      sample_print_input("normal", f->normal);
      printf("      normal_scale=%.6g\n", f->normalScale);
      break;
    }
    case AK_MATERIAL_FEATURE_SPECULAR: {
      const AkMaterialSpecularFeature *f = (const AkMaterialSpecularFeature *)feature;
      sample_print_input("factor", f->factor);
      sample_print_input("color", f->color);
      break;
    }
    case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS: {
      const AkMaterialSpecularGlossinessFeature *f =
        (const AkMaterialSpecularGlossinessFeature *)feature;
      sample_print_input("diffuse", f->diffuse);
      sample_print_input("specular", f->specular);
      sample_print_input("glossiness", f->glossiness);
      break;
    }
    case AK_MATERIAL_FEATURE_TRANSMISSION: {
      const AkMaterialTransmissionFeature *f = (const AkMaterialTransmissionFeature *)feature;
      sample_print_input("factor", f->factor);
      break;
    }
    case AK_MATERIAL_FEATURE_SHEEN: {
      const AkMaterialSheenFeature *f = (const AkMaterialSheenFeature *)feature;
      sample_print_input("color", f->color);
      sample_print_input("roughness", f->roughness);
      break;
    }
    case AK_MATERIAL_FEATURE_IRIDESCENCE: {
      const AkMaterialIridescenceFeature *f = (const AkMaterialIridescenceFeature *)feature;
      sample_print_input("factor", f->factor);
      sample_print_input("thickness", f->thickness);
      printf("      ior=%.6g thickness_min=%.6g thickness_max=%.6g\n",
             f->ior, f->thicknessMinimum, f->thicknessMaximum);
      break;
    }
    case AK_MATERIAL_FEATURE_VOLUME: {
      const AkMaterialVolumeFeature *f = (const AkMaterialVolumeFeature *)feature;
      sample_print_input("thickness", f->thickness);
      printf("      attenuation_color=(%.6g, %.6g, %.6g, %.6g) attenuation_distance=%.6g\n",
             f->attenuationColor.rgba.R,
             f->attenuationColor.rgba.G,
             f->attenuationColor.rgba.B,
             f->attenuationColor.rgba.A,
             f->attenuationDistance);
      break;
    }
    case AK_MATERIAL_FEATURE_ANISOTROPY: {
      const AkMaterialAnisotropyFeature *f = (const AkMaterialAnisotropyFeature *)feature;
      sample_print_input("strength", f->strength);
      sample_print_input("rotation", f->rotation);
      break;
    }
    case AK_MATERIAL_FEATURE_DISPERSION: {
      const AkMaterialDispersionFeature *f = (const AkMaterialDispersionFeature *)feature;
      printf("      dispersion=%.6g\n", f->dispersion);
      break;
    }
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION: {
      const AkMaterialDiffuseTransmissionFeature *f =
        (const AkMaterialDiffuseTransmissionFeature *)feature;
      sample_print_input("factor", f->factor);
      sample_print_input("color", f->color);
      break;
    }
    case AK_MATERIAL_FEATURE_SUBSURFACE: {
      const AkMaterialSubsurfaceFeature *f = (const AkMaterialSubsurfaceFeature *)feature;
      sample_print_input("weight", f->weight);
      sample_print_input("color", f->color);
      sample_print_input("radius", f->radius);
      printf("      anisotropy=%.6g\n", f->anisotropy);
      break;
    }
    case AK_MATERIAL_FEATURE_CLASSIC: {
      const AkMaterialClassicFeature *f = (const AkMaterialClassicFeature *)feature;
      sample_print_input("ambient", f->ambient);
      sample_print_input("diffuse", f->diffuse);
      sample_print_input("specular", f->specular);
      sample_print_input("emission", f->emission);
      sample_print_input("reflective", f->reflective);
      sample_print_input("transparency", f->transparency);
      printf("      shininess=%.6g reflectivity=%.6g ior=%.6g illum=%u\n",
             f->shininess, f->reflectivity, f->ior, f->illum);
      break;
    }
    default:
      break;
  }
}

static void
sample_print_surface(const AkMaterialSurface *surface) {
  const AkMaterialFeature *feature;

  if (!surface) {
    printf("    surface: none\n");
    return;
  }

  printf("    surface: type=%s flags=0x%x feature_mask=0x%x\n",
         sample_material_type_name(surface->type),
         (unsigned)surface->flags,
         (unsigned)surface->featureMask);
  printf("    factors: metallic=%.6g roughness=%.6g opacity=%.6g alpha_cutoff=%.6g ior=%.6g emissive_strength=%.6g\n",
         ak_materialMetallicFactor(surface),
         ak_materialRoughnessFactor(surface),
         ak_materialOpacityFactor(surface),
         ak_materialAlphaCutoff(surface),
         ak_materialIor(surface),
         ak_materialEmissiveStrength(surface));
  printf("    render: double_sided=%s unlit=%s alpha_blend=%s alpha_mask=%s\n",
         ak_materialDoubleSided(surface) ? "yes" : "no",
         ak_materialUnlit(surface) ? "yes" : "no",
         ak_materialAlphaBlend(surface) ? "yes" : "no",
         ak_materialAlphaMask(surface) ? "yes" : "no");

  sample_print_input("baseColor", surface->baseColor);
  sample_print_input("opacity", surface->opacity);
  sample_print_input("metallic", surface->metallic);
  sample_print_input("roughness", surface->roughness);
  sample_print_input("normal", surface->normal);
  sample_print_input("occlusion", surface->occlusion);
  sample_print_input("emissive", surface->emissive);

  for (feature = surface->features; feature; feature = feature->next) {
    printf("    feature: %s flags=0x%x\n",
           sample_material_feature_name(feature->type),
           (unsigned)feature->flags);
    sample_print_feature_inputs(feature);
  }
}

static void
sample_print_materials(AkDoc *doc) {
  const AkMaterial *mat;
  const AkMaterialVariant *variant;
  const AkMaterialPropertySet *set;
  uint64_t index;

  printf("materials=%u variants=%u property_sets=%u\n",
         doc->lib.materials.count,
         doc->materialVariantCount,
         doc->materialProperties.count);

  index = 0;
  for (variant = doc->materialVariants; variant; variant = variant->next, index++)
    printf("variant %" PRIu64 ": %s\n", index, sample_or_unnamed(variant->name));

  index = 0;
  for (set = doc->materialProperties.sets; set; set = set->next, index++) {
    uint32_t i;

    printf("property_set %" PRIu64 ": id=%u name=%s type=%s count=%u\n",
           index,
           set->id,
           sample_or_unnamed(set->name),
           sample_material_property_set_type_name(set->type),
           set->count);
    if (!set->properties)
      continue;

    for (i = 0; i < set->count; i++) {
      const AkMaterialProperty *prop = &set->properties[i];

      printf("  property %u: name=%s material_index=%u flags=0x%x display=(%.6g, %.6g, %.6g, %.6g)\n",
             i,
             sample_or_unnamed(prop->name),
             prop->materialIndex,
             (unsigned)prop->flags,
             prop->displayColor.rgba.R,
             prop->displayColor.rgba.G,
             prop->displayColor.rgba.B,
             prop->displayColor.rgba.A);
      sample_print_input("baseColor", prop->baseColor);
      sample_print_input("metallic", prop->metallic);
      sample_print_input("roughness", prop->roughness);
    }
  }

  index = 0;
  for (mat = doc->lib.materials.first; mat; mat = mat->next, index++) {
    printf("material %" PRIu64 ": name=%s flags=0x%x\n",
           index,
           sample_or_unnamed(mat->name),
           (unsigned)mat->flags);
    sample_print_surface(mat->surface);
  }
}

int
main(int argc, char **argv) {
  AkDoc *doc;

  if (argc != 2) {
    fprintf(stderr, "usage: %s path/to/model\n", argv[0]);
    return 2;
  }

  if (!sample_load_doc(&doc, argv[1]))
    return 1;

  sample_print_materials(doc);
  ak_free(doc);

  return 0;
}
