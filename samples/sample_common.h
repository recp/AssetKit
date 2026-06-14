/*
 * Small helpers shared by the sample programs.
 */

#ifndef assetkit_sample_common_h
#define assetkit_sample_common_h

#include <ak/assetkit.h>
#include <ak/instance.h>

#include <stdio.h>

static inline const char *
sample_or_unnamed(const char *name) {
  return name && *name ? name : "(unnamed)";
}

static inline AkMesh *
sample_mesh_from_geometry(AkGeometry *geom) {
  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return NULL;

  return ak_objGet(geom->gdata);
}

static inline const char *
sample_component_type_name(AkTypeId type) {
  switch (type) {
    case AKT_BYTE:
      return "byte";
    case AKT_UBYTE:
      return "ubyte";
    case AKT_SHORT:
      return "short";
    case AKT_USHORT:
      return "ushort";
    case AKT_INT:
      return "int";
    case AKT_UINT:
      return "uint";
    case AKT_FLOAT:
      return "float";
    case AKT_DOUBLE:
      return "double";
    default:
      return "unknown";
  }
}

static inline const char *
sample_material_type_name(AkMaterialType type) {
  switch (type) {
    case AK_MATERIAL_TYPE_NONE:
      return "none";
    case AK_MATERIAL_TYPE_PHONG:
      return "phong";
    case AK_MATERIAL_TYPE_BLINN:
      return "blinn";
    case AK_MATERIAL_TYPE_LAMBERT:
      return "lambert";
    case AK_MATERIAL_TYPE_CONSTANT:
      return "constant/unlit";
    case AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS:
      return "pbr metallic-roughness";
    case AK_MATERIAL_TYPE_PBR_SPECULAR_GLOSSINESS:
      return "pbr specular-glossiness";
    case AK_MATERIAL_TYPE_PBR:
      return "pbr";
    case AK_MATERIAL_TYPE_SHADER_NETWORK:
      return "shader network";
    default:
      return "unknown";
  }
}

static inline const char *
sample_material_input_source_name(AkMaterialInputSource source) {
  switch (source) {
    case AK_MATERIAL_INPUT_NONE:
      return "none";
    case AK_MATERIAL_INPUT_CONSTANT:
      return "constant";
    case AK_MATERIAL_INPUT_TEXTURE:
      return "texture";
    case AK_MATERIAL_INPUT_VERTEX_COLOR:
      return "vertex-color";
    case AK_MATERIAL_INPUT_FACE_COLOR:
      return "face-color";
    case AK_MATERIAL_INPUT_PROPERTY_INDEX:
      return "property-index";
    case AK_MATERIAL_INPUT_PARAM:
      return "param";
    case AK_MATERIAL_INPUT_SHADER_OUTPUT:
      return "shader-output";
    default:
      return "unknown";
  }
}

static inline const char *
sample_material_input_value_name(AkMaterialInputValue valueType) {
  switch (valueType) {
    case AK_MATERIAL_VALUE_NONE:
      return "none";
    case AK_MATERIAL_VALUE_FLOAT:
      return "float";
    case AK_MATERIAL_VALUE_FLOAT2:
      return "float2";
    case AK_MATERIAL_VALUE_FLOAT3:
      return "float3";
    case AK_MATERIAL_VALUE_FLOAT4:
      return "float4";
    case AK_MATERIAL_VALUE_COLOR:
      return "color";
    case AK_MATERIAL_VALUE_TEXTURE:
      return "texture";
    case AK_MATERIAL_VALUE_INDEX:
      return "index";
    default:
      return "unknown";
  }
}

static inline const char *
sample_material_feature_name(AkMaterialFeatureType type) {
  switch (type) {
    case AK_MATERIAL_FEATURE_CLEARCOAT:
      return "clearcoat";
    case AK_MATERIAL_FEATURE_SPECULAR:
      return "specular";
    case AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS:
      return "specular-glossiness";
    case AK_MATERIAL_FEATURE_TRANSMISSION:
      return "transmission";
    case AK_MATERIAL_FEATURE_SHEEN:
      return "sheen";
    case AK_MATERIAL_FEATURE_IRIDESCENCE:
      return "iridescence";
    case AK_MATERIAL_FEATURE_VOLUME:
      return "volume";
    case AK_MATERIAL_FEATURE_ANISOTROPY:
      return "anisotropy";
    case AK_MATERIAL_FEATURE_DISPERSION:
      return "dispersion";
    case AK_MATERIAL_FEATURE_DIFFUSE_TRANSMISSION:
      return "diffuse-transmission";
    case AK_MATERIAL_FEATURE_SUBSURFACE:
      return "subsurface";
    case AK_MATERIAL_FEATURE_CLASSIC:
      return "classic";
    case AK_MATERIAL_FEATURE_SHADER_NETWORK:
      return "shader-network";
    default:
      return "unknown";
  }
}

static inline const char *
sample_material_property_set_type_name(AkMaterialPropertySetType type) {
  switch (type) {
    case AK_MATERIAL_PROPERTY_BASE:
      return "base";
    case AK_MATERIAL_PROPERTY_COLOR:
      return "color";
    case AK_MATERIAL_PROPERTY_COMPOSITE:
      return "composite";
    case AK_MATERIAL_PROPERTY_MULTI:
      return "multi";
    case AK_MATERIAL_PROPERTY_DISPLAY:
      return "display";
    case AK_MATERIAL_PROPERTY_PHYSICAL:
      return "physical";
    case AK_MATERIAL_PROPERTY_FORMAT_NATIVE:
      return "format-native";
    default:
      return "unknown";
  }
}

static inline const char *
sample_texture_channels_name(AkTextureChannels channels) {
  switch (channels) {
    case AK_TEXTURE_CHANNEL_NONE:
      return "none";
    case AK_TEXTURE_CHANNEL_R:
      return "r";
    case AK_TEXTURE_CHANNEL_G:
      return "g";
    case AK_TEXTURE_CHANNEL_B:
      return "b";
    case AK_TEXTURE_CHANNEL_A:
      return "a";
    case AK_TEXTURE_CHANNEL_RGB:
      return "rgb";
    case AK_TEXTURE_CHANNEL_RGBA:
      return "rgba";
    case AK_TEXTURE_CHANNEL_GB:
      return "gb";
    default:
      return "custom";
  }
}

static inline const char *
sample_texture_color_space_name(AkTextureColorSpace colorSpace) {
  switch (colorSpace) {
    case AK_TEXTURE_COLORSPACE_UNSPECIFIED:
      return "unspecified";
    case AK_TEXTURE_COLORSPACE_LINEAR:
      return "linear";
    case AK_TEXTURE_COLORSPACE_SRGB:
      return "srgb";
    default:
      return "unknown";
  }
}

static inline const char *
sample_projection_type_name(AkProjectionType type) {
  switch (type) {
    case AK_PROJECTION_PERSPECTIVE:
      return "perspective";
    case AK_PROJECTION_ORTHOGRAPHIC:
      return "orthographic";
    case AK_PROJECTION_OTHER:
      return "other";
    default:
      return "unknown";
  }
}

static inline const char *
sample_light_type_name(AkLightType type) {
  switch (type) {
    case AK_LIGHT_TYPE_AMBIENT:
      return "ambient";
    case AK_LIGHT_TYPE_DIRECTIONAL:
      return "directional";
    case AK_LIGHT_TYPE_POINT:
      return "point";
    case AK_LIGHT_TYPE_SPOT:
      return "spot";
    case AK_LIGHT_TYPE_CUSTOM:
      return "custom";
    default:
      return "unknown";
  }
}

static inline const char *
sample_animation_target_name(AkTargetPropertyType type) {
  switch (type) {
    case AK_TARGET_X:
      return "x";
    case AK_TARGET_Y:
      return "y";
    case AK_TARGET_Z:
      return "z";
    case AK_TARGET_XY:
      return "xy";
    case AK_TARGET_XYZ:
      return "xyz";
    case AK_TARGET_ANGLE:
      return "angle";
    case AK_TARGET_POSITION:
      return "position";
    case AK_TARGET_SCALE:
      return "scale";
    case AK_TARGET_ROTATE:
      return "rotate";
    case AK_TARGET_QUAT:
      return "quat";
    case AK_TARGET_WEIGHTS:
      return "weights";
    case AK_TARGET_FLOAT:
      return "float";
    case AK_TARGET_VEC2:
      return "vec2";
    case AK_TARGET_VEC3:
      return "vec3";
    case AK_TARGET_VEC4:
      return "vec4";
    case AK_TARGET_COLOR:
      return "color";
    case AK_TARGET_BOOL:
      return "bool";
    case AK_TARGET_UNKNOWN:
    default:
      return "unknown";
  }
}

static inline const char *
sample_interpolation_name(AkInterpolationType type) {
  switch (type) {
    case AK_INTERPOLATION_LINEAR:
      return "linear";
    case AK_INTERPOLATION_BEZIER:
      return "bezier";
    case AK_INTERPOLATION_CARDINAL:
      return "cardinal";
    case AK_INTERPOLATION_HERMITE:
      return "hermite";
    case AK_INTERPOLATION_BSPLINE:
      return "bspline";
    case AK_INTERPOLATION_STEP:
      return "step";
    case AK_INTERPOLATION_UNKNOWN:
    default:
      return "unknown";
  }
}

static inline int
sample_load_doc(AkDoc **doc, const char *path) {
  AkResult result;

  *doc = NULL;
  result = ak_load(doc, path, AK_FILE_TYPE_AUTO);
  if (result != AK_OK || !*doc) {
    fprintf(stderr, "failed to load %s (result=%d)\n", path, result);
    return 0;
  }

  return 1;
}

#endif /* assetkit_sample_common_h */
