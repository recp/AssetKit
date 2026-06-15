/*
 * Texture and image inspection sample.
 *
 * Walks material inputs to show texture usage, channel masks, color space,
 * texcoord bindings, and image source kind. Then prints the texture/image
 * libraries and marks entries that are not referenced by material inputs.
 */

#include "sample_common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct SamplePtrSet {
  const void **items;
  size_t      count;
  size_t      capacity;
} SamplePtrSet;

static void
sample_ptr_set_free(SamplePtrSet *set) {
  free(set->items);
  set->items = NULL;
  set->count = 0;
  set->capacity = 0;
}

static int
sample_ptr_set_contains(const SamplePtrSet *set, const void *ptr) {
  size_t i;

  if (!ptr)
    return 0;
  for (i = 0; i < set->count; i++) {
    if (set->items[i] == ptr)
      return 1;
  }
  return 0;
}

static int
sample_ptr_set_add(SamplePtrSet *set, const void *ptr) {
  const void **items;
  size_t capacity;

  if (!ptr || sample_ptr_set_contains(set, ptr))
    return 1;

  if (set->count == set->capacity) {
    capacity = set->capacity ? set->capacity * 2u : 16u;
    items = realloc(set->items, capacity * sizeof(*items));
    if (!items)
      return 0;
    set->items = items;
    set->capacity = capacity;
  }

  set->items[set->count++] = ptr;
  return 1;
}

static const AkImageSource *
sample_image_source(const AkImage *image) {
  if (!image)
    return NULL;
  if (image->source)
    return image->source;
  if (image->image)
    return image->image->source;
  return NULL;
}

static const char *
sample_image_source_location(const AkImageSource *source) {
  if (!source)
    return "(none)";
  if (source->resolvedPath)
    return source->resolvedPath;
  if (source->uri)
    return source->uri;
  if (source->buffer)
    return "(buffer)";
  if (source->hex)
    return "(hex)";
  return "(none)";
}

static const char *
sample_image_storage_name(const AkImageSource *source) {
  if (!source)
    return "none";
  switch (source->type) {
    case AK_IMAGE_SOURCE_URI:
      return source->uri && source->uri[0] == 'd' && source->uri[1] == 'a'
             && source->uri[2] == 't' && source->uri[3] == 'a'
             && source->uri[4] == ':'
             ? "embedded-data-uri"
             : "file-or-uri";
    case AK_IMAGE_SOURCE_BUFFER:
    case AK_IMAGE_SOURCE_HEX:
      return "embedded";
    case AK_IMAGE_SOURCE_NONE:
    default:
      return "none";
  }
}

static void
sample_print_texture_usage(const AkMaterial *material,
                           const char *label,
                           const AkMaterialInput *input,
                           SamplePtrSet *used_textures,
                           SamplePtrSet *used_images,
                           uint64_t *usage_count) {
  const AkTextureRef *texref;
  const AkTexture *texture;
  const AkImage *image;
  const AkImageSource *source;

  if (!input || !input->texture || !input->texture->texture)
    return;

  texref = input->texture;
  texture = texref->texture;
  image = texture->image;
  source = sample_image_source(image);

  sample_ptr_set_add(used_textures, texture);
  sample_ptr_set_add(used_images, image);
  (*usage_count)++;

  printf("texture_usage %" PRIu64 ": material=%s input=%s texture=%s image=%s",
         *usage_count,
         sample_or_unnamed(material ? material->name : NULL),
         label,
         sample_or_unnamed(texture->name),
         image ? sample_or_unnamed(image->name) : "(none)");
  printf(" channels=%s color_space=%s texcoord=%s slot=%d",
         sample_texture_channels_name(texref->channels),
         sample_texture_color_space_name(texref->colorSpace),
         texref->coordInputName ? texref->coordInputName
                                : (texref->texcoord ? texref->texcoord : "(default)"),
         texref->slot);
  printf(" source=%s storage=%s location=%s",
         sample_image_source_type_name(source ? source->type : AK_IMAGE_SOURCE_NONE),
         sample_image_storage_name(source),
         sample_image_source_location(source));
  if (source && source->mimeType)
    printf(" mime=%s", source->mimeType);
  printf("\n");
}

static void
sample_collect_feature_textures(const AkMaterial *material,
                                const AkMaterialFeature *feature,
                                SamplePtrSet *used_textures,
                                SamplePtrSet *used_images,
                                uint64_t *usage_count) {
  switch (feature->type) {
    case AK_MATERIAL_FEATURE_CLEARCOAT: {
      const AkMaterialClearcoatFeature *f = (const AkMaterialClearcoatFeature *)feature;
      sample_print_texture_usage(material, "clearcoat.factor", f->factor, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "clearcoat.roughness", f->roughness, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "clearcoat.normal", f->normal, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_SPECULAR: {
      const AkMaterialSpecularFeature *f = (const AkMaterialSpecularFeature *)feature;
      sample_print_texture_usage(material, "specular.factor", f->factor, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "specular.color", f->color, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS: {
      const AkMaterialSpecularGlossinessFeature *f =
        (const AkMaterialSpecularGlossinessFeature *)feature;
      sample_print_texture_usage(material, "specGloss.diffuse", f->diffuse, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "specGloss.specular", f->specular, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "specGloss.glossiness", f->glossiness, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_TRANSMISSION: {
      const AkMaterialTransmissionFeature *f = (const AkMaterialTransmissionFeature *)feature;
      sample_print_texture_usage(material, "transmission.factor", f->factor, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_SHEEN: {
      const AkMaterialSheenFeature *f = (const AkMaterialSheenFeature *)feature;
      sample_print_texture_usage(material, "sheen.color", f->color, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "sheen.roughness", f->roughness, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_IRIDESCENCE: {
      const AkMaterialIridescenceFeature *f = (const AkMaterialIridescenceFeature *)feature;
      sample_print_texture_usage(material, "iridescence.factor", f->factor, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "iridescence.thickness", f->thickness, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_VOLUME: {
      const AkMaterialVolumeFeature *f = (const AkMaterialVolumeFeature *)feature;
      sample_print_texture_usage(material, "volume.thickness", f->thickness, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_ANISOTROPY: {
      const AkMaterialAnisotropyFeature *f = (const AkMaterialAnisotropyFeature *)feature;
      sample_print_texture_usage(material, "anisotropy.strength", f->strength, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "anisotropy.rotation", f->rotation, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION: {
      const AkMaterialDiffuseTransmissionFeature *f =
        (const AkMaterialDiffuseTransmissionFeature *)feature;
      sample_print_texture_usage(material, "diffuseTransmission.factor", f->factor, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "diffuseTransmission.color", f->color, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_SUBSURFACE: {
      const AkMaterialSubsurfaceFeature *f = (const AkMaterialSubsurfaceFeature *)feature;
      sample_print_texture_usage(material, "subsurface.weight", f->weight, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "subsurface.color", f->color, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "subsurface.radius", f->radius, used_textures, used_images, usage_count);
      break;
    }
    case AK_MATERIAL_FEATURE_CLASSIC: {
      const AkMaterialClassicFeature *f = (const AkMaterialClassicFeature *)feature;
      sample_print_texture_usage(material, "classic.ambient", f->ambient, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "classic.diffuse", f->diffuse, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "classic.specular", f->specular, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "classic.emission", f->emission, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "classic.reflective", f->reflective, used_textures, used_images, usage_count);
      sample_print_texture_usage(material, "classic.transparency", f->transparency, used_textures, used_images, usage_count);
      break;
    }
    default:
      break;
  }
}

static void
sample_collect_material_textures(AkDoc *doc,
                                 SamplePtrSet *used_textures,
                                 SamplePtrSet *used_images,
                                 uint64_t *usage_count) {
  const AkMaterial *material;

  for (material = doc->lib.materials.first; material; material = material->next) {
    const AkMaterialSurface *surface;
    const AkMaterialFeature *feature;

    surface = material->surface;
    if (!surface)
      continue;

    sample_print_texture_usage(material, "baseColor", surface->baseColor, used_textures, used_images, usage_count);
    sample_print_texture_usage(material, "opacity", surface->opacity, used_textures, used_images, usage_count);
    sample_print_texture_usage(material, "metallic", surface->metallic, used_textures, used_images, usage_count);
    sample_print_texture_usage(material, "roughness", surface->roughness, used_textures, used_images, usage_count);
    sample_print_texture_usage(material, "normal", surface->normal, used_textures, used_images, usage_count);
    sample_print_texture_usage(material, "occlusion", surface->occlusion, used_textures, used_images, usage_count);
    sample_print_texture_usage(material, "emissive", surface->emissive, used_textures, used_images, usage_count);

    for (feature = surface->features; feature; feature = feature->next)
      sample_collect_feature_textures(material, feature, used_textures, used_images, usage_count);
  }
}

static void
sample_print_texture_library(AkDoc *doc, const SamplePtrSet *used_textures) {
  const AkTexture *texture;
  uint64_t index;

  index = 0;
  for (texture = doc->lib.textures.first; texture; texture = texture->next, index++) {
    const AkImage *image;

    image = texture->image;
    printf("texture %" PRIu64 ": name=%s image=%s sampler=%s used=%s\n",
           index,
           sample_or_unnamed(texture->name),
           image ? sample_or_unnamed(image->name) : "(none)",
           texture->sampler ? sample_or_unnamed(texture->sampler->name) : "(none)",
           sample_ptr_set_contains(used_textures, texture) ? "yes" : "no");
  }
}

static void
sample_print_image_library(AkDoc *doc, const SamplePtrSet *used_images) {
  const AkImage *image;
  uint64_t index;

  index = 0;
  for (image = doc->lib.images.first; image; image = image->next, index++) {
    const AkImageSource *source;
    AkImageType type;

    source = sample_image_source(image);
    type = image->image ? image->image->type : AK_IMAGE_TYPE_2D;
    printf("image %" PRIu64 ": name=%s type=%s source=%s storage=%s used=%s location=%s",
           index,
           sample_or_unnamed(image->name),
           sample_image_type_name(type),
           sample_image_source_type_name(source ? source->type : AK_IMAGE_SOURCE_NONE),
           sample_image_storage_name(source),
           sample_ptr_set_contains(used_images, image) ? "yes" : "no",
           sample_image_source_location(source));
    if (source && source->mimeType)
      printf(" mime=%s", source->mimeType);
    if (image->data)
      printf(" loaded=%ux%u comp=%d", image->data->width, image->data->height, image->data->comp);
    printf("\n");
  }
}

int
main(int argc, char **argv) {
  AkDoc *doc;
  SamplePtrSet used_textures = {0};
  SamplePtrSet used_images = {0};
  uint64_t usage_count;

  if (argc != 2) {
    fprintf(stderr, "usage: %s path/to/model\n", argv[0]);
    return 2;
  }

  if (!sample_load_doc(&doc, argv[1]))
    return 1;

  printf("texture_summary: materials=%u textures=%u images=%u samplers=%u\n",
         doc->lib.materials.count,
         doc->lib.textures.count,
         doc->lib.images.count,
         doc->lib.samplers.count);

  usage_count = 0;
  sample_collect_material_textures(doc, &used_textures, &used_images, &usage_count);
  printf("texture_usages=%" PRIu64 " unique_textures=%zu unique_images=%zu\n",
         usage_count,
         used_textures.count,
         used_images.count);

  sample_print_texture_library(doc, &used_textures);
  sample_print_image_library(doc, &used_images);

  sample_ptr_set_free(&used_textures);
  sample_ptr_set_free(&used_images);
  ak_free(doc);

  return 0;
}
