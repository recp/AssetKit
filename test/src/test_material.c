/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "../include/common.h"

#include <ak/assetkit.h>
#include <ak/material.h>
#include <ak/geom.h>

#include <limits.h>
#include <unistd.h>

static
bool
test_write_material_dae(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><contributor><authoring_tool>SceneKit Collada Exporter v1.0</authoring_tool></contributor><up_axis>Y_UP</up_axis></asset>\n"
        "<library_images><image id=\"img_alpha\" name=\"alpha\"><init_from>alpha.png</init_from></image></library_images>\n"
        "<library_effects>\n"
        "<effect id=\"effect_edge\"><profile_COMMON><technique sid=\"common\"><constant>"
        "<transparent opaque=\"A_ONE\"><color>0.9 0.9 0.9 1</color></transparent>"
        "<transparency><float>1</float></transparency>"
        "</constant><extra><technique profile=\"SceneKit\"><constant_diffuse>"
        "<color>0 0 0 1</color>"
        "</constant_diffuse></technique></extra></technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_a_one\"><profile_COMMON><technique sid=\"common\"><lambert>"
        "<diffuse><color>1 1 1 1</color></diffuse>"
        "<transparent opaque=\"A_ONE\"><color>1 1 1 0.25</color></transparent>"
        "<transparency><float>0.5</float></transparency>"
        "</lambert></technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_a_zero\"><profile_COMMON><technique sid=\"common\"><lambert>"
        "<diffuse><color>1 1 1 1</color></diffuse>"
        "<transparent opaque=\"A_ZERO\"><color>1 1 1 0.25</color></transparent>"
        "<transparency><float>0.5</float></transparency>"
        "</lambert></technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_rgb_one\"><profile_COMMON><technique sid=\"common\"><lambert>"
        "<diffuse><color>1 1 1 1</color></diffuse>"
        "<transparent opaque=\"RGB_ONE\"><color>0.2 0.3 0.4 1</color></transparent>"
        "<transparency><float>1</float></transparency>"
        "</lambert></technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_rgb_zero\"><profile_COMMON><technique sid=\"common\"><lambert>"
        "<diffuse><color>1 1 1 1</color></diffuse>"
        "<transparent opaque=\"RGB_ZERO\"><color>0.2 0.3 0.4 1</color></transparent>"
        "<transparency><float>1</float></transparency>"
        "</lambert></technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_tex_a_one\"><profile_COMMON>"
        "<newparam sid=\"surface_alpha\"><surface type=\"2D\"><init_from>img_alpha</init_from></surface></newparam>"
        "<newparam sid=\"sampler_alpha\"><sampler2D><source>surface_alpha</source></sampler2D></newparam>"
        "<technique sid=\"common\"><lambert><diffuse><color>1 1 1 1</color></diffuse>"
        "<transparent opaque=\"A_ONE\"><texture texture=\"sampler_alpha\" texcoord=\"UVSET0\"/></transparent>"
        "<transparency><float>0.5</float></transparency>"
        "</lambert></technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_tex_a_zero\"><profile_COMMON>"
        "<newparam sid=\"surface_alpha\"><surface type=\"2D\"><init_from>img_alpha</init_from></surface></newparam>"
        "<newparam sid=\"sampler_alpha\"><sampler2D><source>surface_alpha</source></sampler2D></newparam>"
        "<technique sid=\"common\"><lambert><diffuse><color>1 1 1 1</color></diffuse>"
        "<transparent opaque=\"A_ZERO\"><texture texture=\"sampler_alpha\" texcoord=\"UVSET0\"/></transparent>"
        "<transparency><float>0.5</float></transparency>"
        "</lambert></technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_tex_rgb_one\"><profile_COMMON>"
        "<newparam sid=\"surface_alpha\"><surface type=\"2D\"><init_from>img_alpha</init_from></surface></newparam>"
        "<newparam sid=\"sampler_alpha\"><sampler2D><source>surface_alpha</source></sampler2D></newparam>"
        "<technique sid=\"common\"><lambert><diffuse><color>1 1 1 1</color></diffuse>"
        "<transparent opaque=\"RGB_ONE\"><texture texture=\"sampler_alpha\" texcoord=\"UVSET0\"/></transparent>"
        "<transparency><float>0.5</float></transparency>"
        "</lambert></technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_tex_rgb_zero\"><profile_COMMON>"
        "<newparam sid=\"surface_alpha\"><surface type=\"2D\"><init_from>img_alpha</init_from></surface></newparam>"
        "<newparam sid=\"sampler_alpha\"><sampler2D><source>surface_alpha</source></sampler2D></newparam>"
        "<technique sid=\"common\"><lambert><diffuse><color>1 1 1 1</color></diffuse>"
        "<transparent opaque=\"RGB_ZERO\"><texture texture=\"sampler_alpha\" texcoord=\"UVSET0\"/></transparent>"
        "<transparency><float>0.5</float></transparency>"
        "</lambert></technique></profile_COMMON></effect>\n"
        "</library_effects>\n"
        "<library_materials>"
        "<material id=\"mat_edge\" name=\"edge\"><instance_effect url=\"#effect_edge\"/></material>"
        "<material id=\"mat_a_one\" name=\"a_one\"><instance_effect url=\"#effect_a_one\"/></material>"
        "<material id=\"mat_a_zero\" name=\"a_zero\"><instance_effect url=\"#effect_a_zero\"/></material>"
        "<material id=\"mat_rgb_one\" name=\"rgb_one\"><instance_effect url=\"#effect_rgb_one\"/></material>"
        "<material id=\"mat_rgb_zero\" name=\"rgb_zero\"><instance_effect url=\"#effect_rgb_zero\"/></material>"
        "<material id=\"mat_tex_a_one\" name=\"tex_a_one\"><instance_effect url=\"#effect_tex_a_one\"/></material>"
        "<material id=\"mat_tex_a_zero\" name=\"tex_a_zero\"><instance_effect url=\"#effect_tex_a_zero\"/></material>"
        "<material id=\"mat_tex_rgb_one\" name=\"tex_rgb_one\"><instance_effect url=\"#effect_tex_rgb_one\"/></material>"
        "<material id=\"mat_tex_rgb_zero\" name=\"tex_rgb_zero\"><instance_effect url=\"#effect_tex_rgb_zero\"/></material>"
        "</library_materials>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\"/></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
test_write_obj_material_files(const char *objPath,
                              const char *mtlPath) {
  FILE *file;

  file = fopen(mtlPath, "wb");
  if (!file)
    return false;

  fputs("newmtl obj_opacity\n"
        "Kd 1 1 1\n"
        "Tf 0.2 0.3 0.4\n"
        "d 0.5\n"
        "map_d alpha.png\n",
        file);
  if (fclose(file) != 0)
    return false;

  file = fopen(objPath, "wb");
  if (!file)
    return false;

  fputs("mtllib material_adapter.mtl\n"
        "o tri\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 0 1\n"
        "usemtl obj_opacity\n"
        "f 1/1 2/2 3/3\n",
        file);

  return fclose(file) == 0;
}

static
bool
test_write_obj_vertex_alpha(const char *objPath) {
  FILE *file;

  file = fopen(objPath, "wb");
  if (!file)
    return false;

  fputs("o rgba\n"
        "v 0 0 0 1 0 0 0.25\n"
        "v 1 0 0 0 1 0 0.50\n"
        "v 0 1 0 0 0 1 0.75\n"
        "f 1 2 3\n",
        file);

  return fclose(file) == 0;
}

static
bool
test_write_gltf_alpha_modes(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"materials\":["
        "{\"name\":\"opaque_alpha\","
        "\"pbrMetallicRoughness\":{\"baseColorFactor\":[1,1,1,0.25]}},"
        "{\"name\":\"explicit_opaque_alpha\",\"alphaMode\":\"OPAQUE\","
        "\"pbrMetallicRoughness\":{\"baseColorFactor\":[1,1,1,0.25]}},"
        "{\"name\":\"blend_alpha\",\"alphaMode\":\"BLEND\","
        "\"pbrMetallicRoughness\":{\"baseColorFactor\":[1,1,1,0.5]}},"
        "{\"name\":\"mask_alpha\",\"alphaMode\":\"MASK\",\"alphaCutoff\":0.33,"
        "\"pbrMetallicRoughness\":{\"baseColorFactor\":[1,1,1,0.5]}},"
        "{\"name\":\"volume_scatter\","
        "\"extensions\":{\"KHR_materials_volume_scatter\":{"
        "\"multiscatterColor\":[0.1,0.2,0.3],\"scatterAnisotropy\":0.4}}}"
        "],"
        "\"scenes\":[{\"nodes\":[]}],"
        "\"scene\":0"
        "}\n",
        file);

  return fclose(file) == 0;
}

static
AkMaterial*
test_material_by_name(AkDoc *doc, const char *name) {
  AkMaterial *mat;

  if (!doc || !doc->lib.materials.first)
    return NULL;

  for (mat = doc->lib.materials.first; mat; mat = mat->next) {
    if (mat->name && strcmp(mat->name, name) == 0)
      return mat;
  }

  return NULL;
}

static
AkMaterial*
test_first_primitive_material(AkDoc *doc) {
  AkGeometry *geom;

  for (geom = doc ? doc->lib.geometries.first : NULL; geom; geom = geom->next) {
    AkMesh *mesh;

    if (!geom->gdata)
      continue;

    mesh = ak_objGet(geom->gdata);
    if (mesh && mesh->primitive && mesh->primitive->material)
      return mesh->primitive->material;
  }

  return NULL;
}

static
bool
test_material_opacity(AkMaterial         *mat,
                      AkTextureChannels  channels,
                      float              minValue,
                      float              maxValue,
                      bool               textured,
                      bool               inverted) {
  AkMaterialInput *opacity;

  if (!mat || !mat->surface || !(opacity = mat->surface->opacity))
    return false;

  return opacity->channels == channels
         && opacity->value[0] > minValue
         && opacity->value[0] < maxValue
         && (opacity->texture != NULL) == textured
         && ak_materialInputFlag(opacity, AK_MATERIAL_INPUT_FLAG_INVERTED) == inverted;
}

TEST_IMPL(material_api) {
  AkMaterialSurface          baseSurface;
  AkMaterialSurface          variantSurface;
  AkMaterialInput            baseColor;
  AkMaterialInput            scalarInput;
  AkTextureRef               textureRef;
  AkMaterialClearcoatFeature clearcoat;
  AkMaterial                 baseMaterial;
  AkMaterial                 variantMaterial;
  AkMaterialBinding          fallbackBinding;
  AkMaterialBinding          variantBinding;
  AkMaterialBinding          propertyBinding;
  AkMaterialVariantMapping   legacyMapping;
  AkMaterialPropertySet      propertySet;
  AkMaterialProperty         properties[2];
  AkDoc                      doc;
  AkMeshPrimitive            prim;
  AkResolvedMaterial         resolved;

  memset(&baseSurface, 0, sizeof(baseSurface));
  memset(&variantSurface, 0, sizeof(variantSurface));
  memset(&baseColor, 0, sizeof(baseColor));
  memset(&scalarInput, 0, sizeof(scalarInput));
  memset(&textureRef, 0, sizeof(textureRef));
  memset(&clearcoat, 0, sizeof(clearcoat));
  memset(&baseMaterial, 0, sizeof(baseMaterial));
  memset(&variantMaterial, 0, sizeof(variantMaterial));
  memset(&fallbackBinding, 0, sizeof(fallbackBinding));
  memset(&variantBinding, 0, sizeof(variantBinding));
  memset(&propertyBinding, 0, sizeof(propertyBinding));
  memset(&legacyMapping, 0, sizeof(legacyMapping));
  memset(&propertySet, 0, sizeof(propertySet));
  memset(properties, 0, sizeof(properties));
  memset(&doc, 0, sizeof(doc));
  memset(&prim, 0, sizeof(prim));

  ASSERT(ak_materialTypeIsPBR(AK_MATERIAL_TYPE_PBR));
  ASSERT(ak_materialTypeIsPBR(AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS));
  ASSERT(ak_materialTypeIsClassic(AK_MATERIAL_TYPE_PHONG));
  ASSERT(!ak_materialTypeIsClassic(AK_MATERIAL_TYPE_PBR));
  ASSERT(!ak_materialTypeIsRenderable(AK_MATERIAL_TYPE_NONE));

  baseColor.semantic  = "baseColor";
  baseColor.source    = AK_MATERIAL_INPUT_CONSTANT;
  baseColor.valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor.flags     = AK_MATERIAL_INPUT_FLAG_INVERTED;
  baseSurface.type    = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  baseSurface.baseColor = &baseColor;

  clearcoat.base.type = AK_MATERIAL_FEATURE_CLEARCOAT;
  baseSurface.features = &clearcoat.base;
  baseSurface.featureMask = 1u << AK_MATERIAL_FEATURE_CLEARCOAT;
  scalarInput.semantic = "customSlot";
  clearcoat.factor = &scalarInput;

  ASSERT(ak_materialSemantic("baseColor") == AK_MATERIAL_SEMANTIC_BASE_COLOR);
  ASSERT(ak_materialSemantic("diffuse") == AK_MATERIAL_SEMANTIC_BASE_COLOR);
  ASSERT(ak_materialSemantic("alpha") == AK_MATERIAL_SEMANTIC_OPACITY);
  ASSERT(ak_materialSemantic("metalness") == AK_MATERIAL_SEMANTIC_METALLIC);
  ASSERT(ak_materialSemantic("customSlot") == AK_MATERIAL_SEMANTIC_UNKNOWN);
  ASSERT(strcmp(ak_materialSemanticName(AK_MATERIAL_SEMANTIC_BASE_COLOR), "baseColor") == 0);
  ASSERT(ak_materialSemanticName(AK_MATERIAL_SEMANTIC_UNKNOWN) == NULL);
  ASSERT(ak_materialInputBySemantic(&baseSurface, AK_MATERIAL_SEMANTIC_BASE_COLOR) == &baseColor);
  ASSERT(ak_materialInput(&baseSurface, "baseColor")
         == &baseColor);
  ASSERT(ak_materialInput(&baseSurface, "diffuse") == &baseColor);
  ASSERT(ak_materialInput(&baseSurface, "customSlot") == &scalarInput);
  ASSERT(ak_materialInputFlag(&baseColor, AK_MATERIAL_INPUT_FLAG_INVERTED));
  ASSERT(!ak_materialInputFlag(&baseColor, AK_MATERIAL_INPUT_FLAG_NORMALIZED));
  ASSERT(ak_materialInputTexture(NULL) == NULL);
  ASSERT(ak_materialInputScalar(NULL, 1.0f) == 1.0f);
  ASSERT(ak_materialInputChannels(NULL) == AK_TEXTURE_CHANNEL_NONE);
  ASSERT(ak_materialInputScalar(&baseColor, 0.5f) == 0.5f);
  ASSERT(ak_materialNormalScale(NULL) == 1.0f);
  ASSERT(ak_materialOcclusionStrength(NULL) == 1.0f);
  ASSERT(ak_materialOpacityFactor(NULL) == 1.0f);
  ASSERT(ak_materialMetallicFactor(NULL) == 0.0f);
  ASSERT(ak_materialRoughnessFactor(NULL) == 1.0f);
  ASSERT(ak_materialAlphaCutoff(NULL) == 0.5f);
  ASSERT(ak_materialIor(NULL) == 1.5f);
  ASSERT(ak_materialEmissiveStrength(NULL) == 1.0f);
  ASSERT(!ak_materialDoubleSided(NULL));
  ASSERT(!ak_materialUnlit(NULL));
  ASSERT(!ak_materialAlphaBlend(NULL));
  ASSERT(!ak_materialAlphaMask(NULL));
  ASSERT(ak_materialNormalScale(&baseSurface) == 1.0f);
  ASSERT(ak_materialOcclusionStrength(&baseSurface) == 1.0f);
  ASSERT(ak_materialOpacityFactor(&baseSurface) == 1.0f);
  ASSERT(ak_materialMetallicFactor(&baseSurface) == 0.0f);
  ASSERT(ak_materialRoughnessFactor(&baseSurface) == 1.0f);
  ASSERT(ak_materialAlphaCutoff(&baseSurface) == 0.0f);
  ASSERT(ak_materialIor(&baseSurface) == 0.0f);
  ASSERT(ak_materialEmissiveStrength(&baseSurface) == 0.0f);
  ASSERT(!ak_materialDoubleSided(&baseSurface));
  ASSERT(!ak_materialUnlit(&baseSurface));
  ASSERT(!ak_materialAlphaBlend(&baseSurface));
  ASSERT(!ak_materialAlphaMask(&baseSurface));

  scalarInput.texture = &textureRef;
  scalarInput.valueType = AK_MATERIAL_VALUE_FLOAT;
  scalarInput.value[0] = 0.75f;
  scalarInput.channels = AK_TEXTURE_CHANNEL_R;
  ASSERT(ak_materialInputTexture(&scalarInput) == &textureRef);
  ASSERT(ak_materialInputScalar(&scalarInput, 1.0f) == 0.75f);
  ASSERT(ak_materialInputChannels(&scalarInput) == AK_TEXTURE_CHANNEL_R);
  baseSurface.normal = &scalarInput;
  baseSurface.occlusion = &scalarInput;
  baseSurface.opacity = &scalarInput;
  baseSurface.metallic = &scalarInput;
  baseSurface.roughness = &scalarInput;
  baseSurface.alphaCutoff = 0.25f;
  baseSurface.ior = 1.45f;
  baseSurface.emissiveStrength = 2.0f;
  baseSurface.flags |= AK_MATERIAL_FLAG_DOUBLE_SIDED
                       | AK_MATERIAL_FLAG_UNLIT
                       | AK_MATERIAL_FLAG_ALPHA_BLEND
                       | AK_MATERIAL_FLAG_ALPHA_MASK;
  ASSERT(ak_materialNormalScale(&baseSurface) == 0.75f);
  ASSERT(ak_materialOcclusionStrength(&baseSurface) == 0.75f);
  ASSERT(ak_materialOpacityFactor(&baseSurface) == 0.75f);
  ASSERT(ak_materialMetallicFactor(&baseSurface) == 0.75f);
  ASSERT(ak_materialRoughnessFactor(&baseSurface) == 0.75f);
  ASSERT(ak_materialAlphaCutoff(&baseSurface) == 0.25f);
  ASSERT(ak_materialIor(&baseSurface) == 1.45f);
  ASSERT(ak_materialEmissiveStrength(&baseSurface) == 2.0f);
  ASSERT(ak_materialDoubleSided(&baseSurface));
  ASSERT(ak_materialUnlit(&baseSurface));
  ASSERT(ak_materialAlphaBlend(&baseSurface));
  ASSERT(ak_materialAlphaMask(&baseSurface));

  ASSERT(ak_materialFeature(&baseSurface, AK_MATERIAL_FEATURE_CLEARCOAT)
         == &clearcoat.base);
  ASSERT(ak_materialHasFeature(&baseSurface, AK_MATERIAL_FEATURE_CLEARCOAT));
  ASSERT(!ak_materialHasFeature(&baseSurface, AK_MATERIAL_FEATURE_SHEEN));

  baseMaterial.surface = &baseSurface;
  prim.material = &baseMaterial;

  ASSERT(ak_materialResolveForPrimitive(&prim, UINT32_MAX, &resolved));
  ASSERT(resolved.material == &baseMaterial);
  ASSERT(resolved.surface == &baseSurface);
  ASSERT(resolved.variantIndex == UINT32_MAX);

  variantSurface.type = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  variantMaterial.surface = &variantSurface;

  fallbackBinding.material = &baseMaterial;
  fallbackBinding.scope = AK_MATERIAL_BIND_PRIMITIVE;
  fallbackBinding.propertyIndex = UINT32_MAX;
  fallbackBinding.variantIndex = UINT32_MAX;

  variantBinding.material = &variantMaterial;
  variantBinding.next = &fallbackBinding;
  variantBinding.scope = AK_MATERIAL_BIND_PRIMITIVE;
  variantBinding.propertyIndex = UINT32_MAX;
  variantBinding.variantIndex = 0;

  prim.materialBindings = &variantBinding;

  ASSERT(ak_materialResolveForPrimitive(&prim, 0, &resolved));
  ASSERT(resolved.material == &variantMaterial);
  ASSERT(resolved.surface == &variantSurface);
  ASSERT(resolved.binding == &variantBinding);
  ASSERT(resolved.variantIndex == 0);

  ASSERT(ak_materialResolveForPrimitive(&prim, 1, &resolved));
  ASSERT(resolved.material == &baseMaterial);
  ASSERT(resolved.surface == &baseSurface);
  ASSERT(resolved.binding == &fallbackBinding);
  ASSERT(resolved.variantIndex == UINT32_MAX);

  prim.materialBindings = NULL;
  legacyMapping.material = &variantMaterial;
  legacyMapping.variantIndex = 2;
  prim.variantMappings = &legacyMapping;

  ASSERT(ak_materialResolveForPrimitive(&prim, 2, &resolved));
  ASSERT(resolved.material == &variantMaterial);
  ASSERT(resolved.surface == &variantSurface);
  ASSERT(resolved.binding == NULL);
  ASSERT(resolved.variantIndex == 2);

  propertySet.id = 42;
  propertySet.count = 2;
  propertySet.properties = properties;
  doc.materialProperties.sets = &propertySet;
  doc.materialProperties.count = 1;

  ASSERT(ak_materialPropertySetById(&doc, 42) == &propertySet);
  ASSERT(ak_materialPropertySetById(&doc, 43) == NULL);
  ASSERT(ak_materialProperty(&propertySet, 1) == &properties[1]);
  ASSERT(ak_materialProperty(&propertySet, 2) == NULL);

  propertyBinding.propertySet = &propertySet;
  propertyBinding.scope = AK_MATERIAL_BIND_PRIMITIVE;
  propertyBinding.propertyIndex = 1;
  propertyBinding.variantIndex = UINT32_MAX;

  memset(&prim, 0, sizeof(prim));
  prim.materialBindings = &propertyBinding;

  ASSERT(ak_materialResolveForPrimitive(&prim, UINT32_MAX, &resolved));
  ASSERT(resolved.material == NULL);
  ASSERT(resolved.surface == NULL);
  ASSERT(resolved.binding == &propertyBinding);
  ASSERT(resolved.propertyIndex == 1);
  ASSERT(ak_resolvedMaterialProperty(&resolved) == &properties[1]);

  TEST_SUCCESS
}

TEST_IMPL(material_dae_adapter) {
  AkDoc       *doc;
  AkMaterial  *edge;
  AkMaterial  *aOne;
  AkMaterial  *aZero;
  AkMaterial  *rgbOne;
  AkMaterial  *rgbZero;
  AkMaterial  *texAOne;
  AkMaterial  *texAZero;
  AkMaterial  *texRgbOne;
  AkMaterial  *texRgbZero;
  char          dirTemplate[PATH_MAX];
  char         *tmpdir;
  char          daePath[PATH_MAX];
  const char   *tmpBase;

  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate, sizeof(dirTemplate), "%s/assetkit-material-dae-XXXXXX", tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/material_adapter.dae", tmpdir);
  ASSERT(test_write_material_dae(daePath));

  doc = NULL;
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  edge = test_material_by_name(doc, "edge");
  ASSERT(edge && edge->surface && edge->surface->baseColor);
  ASSERT(edge->surface->baseColor->color.rgba.R == 0.0f);
  ASSERT(edge->surface->baseColor->color.rgba.G == 0.0f);
  ASSERT(edge->surface->baseColor->color.rgba.B == 0.0f);

  aOne = test_material_by_name(doc, "a_one");
  ASSERT(test_material_opacity(aOne, AK_TEXTURE_CHANNEL_A, 0.124f, 0.126f, false, false));

  aZero = test_material_by_name(doc, "a_zero");
  ASSERT(test_material_opacity(aZero, AK_TEXTURE_CHANNEL_A, 0.874f, 0.876f, false, false));

  rgbOne = test_material_by_name(doc, "rgb_one");
  ASSERT(test_material_opacity(rgbOne, AK_TEXTURE_CHANNEL_RGB, 0.068f, 0.070f, false, false));

  rgbZero = test_material_by_name(doc, "rgb_zero");
  ASSERT(test_material_opacity(rgbZero, AK_TEXTURE_CHANNEL_RGB, 0.930f, 0.932f, false, false));

  texAOne = test_material_by_name(doc, "tex_a_one");
  ASSERT(test_material_opacity(texAOne, AK_TEXTURE_CHANNEL_A, 0.499f, 0.501f, true, false));

  texAZero = test_material_by_name(doc, "tex_a_zero");
  ASSERT(test_material_opacity(texAZero, AK_TEXTURE_CHANNEL_A, 0.499f, 0.501f, true, true));

  texRgbOne = test_material_by_name(doc, "tex_rgb_one");
  ASSERT(test_material_opacity(texRgbOne, AK_TEXTURE_CHANNEL_RGB, 0.499f, 0.501f, true, false));

  texRgbZero = test_material_by_name(doc, "tex_rgb_zero");
  ASSERT(test_material_opacity(texRgbZero, AK_TEXTURE_CHANNEL_RGB, 0.499f, 0.501f, true, true));

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(material_gltf_alpha_modes) {
  AkDoc       *doc;
  AkMaterial  *opaque;
  AkMaterial  *explicitOpaque;
  AkMaterial  *blend;
  AkMaterial  *mask;
  AkMaterial  *scatter;
  AkMaterialSubsurfaceFeature *subsurface;
  char          dirTemplate[PATH_MAX];
  char         *tmpdir;
  char          gltfPath[PATH_MAX];
  const char   *tmpBase;

  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate, sizeof(dirTemplate), "%s/assetkit-material-gltf-XXXXXX", tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(gltfPath, sizeof(gltfPath), "%s/alpha_modes.gltf", tmpdir);
  ASSERT(test_write_gltf_alpha_modes(gltfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK && doc);

  opaque = test_material_by_name(doc, "opaque_alpha");
  ASSERT(opaque && opaque->surface && opaque->surface->baseColor);
  ASSERT(opaque->surface->baseColor->color.rgba.A > 0.249f);
  ASSERT(opaque->surface->baseColor->color.rgba.A < 0.251f);
  ASSERT((opaque->surface->flags & (AK_MATERIAL_FLAG_ALPHA_BLEND | AK_MATERIAL_FLAG_ALPHA_MASK)) == 0);
  ASSERT(opaque->surface->opacity == NULL);

  explicitOpaque = test_material_by_name(doc, "explicit_opaque_alpha");
  ASSERT(explicitOpaque && explicitOpaque->surface);
  ASSERT((explicitOpaque->surface->flags & (AK_MATERIAL_FLAG_ALPHA_BLEND | AK_MATERIAL_FLAG_ALPHA_MASK)) == 0);
  ASSERT(explicitOpaque->surface->opacity == NULL);

  blend = test_material_by_name(doc, "blend_alpha");
  ASSERT(blend && blend->surface);
  ASSERT(blend->surface->flags & AK_MATERIAL_FLAG_ALPHA_BLEND);
  ASSERT(!(blend->surface->flags & AK_MATERIAL_FLAG_ALPHA_MASK));

  mask = test_material_by_name(doc, "mask_alpha");
  ASSERT(mask && mask->surface);
  ASSERT(mask->surface->flags & AK_MATERIAL_FLAG_ALPHA_MASK);
  ASSERT(!(mask->surface->flags & AK_MATERIAL_FLAG_ALPHA_BLEND));
  ASSERT(mask->surface->alphaCutoff > 0.329f);
  ASSERT(mask->surface->alphaCutoff < 0.331f);

  scatter = test_material_by_name(doc, "volume_scatter");
  ASSERT(scatter && scatter->surface);
  subsurface = (void*)ak_materialFeature(scatter->surface,
                                         AK_MATERIAL_FEATURE_SUBSURFACE);
  ASSERT(subsurface != NULL);
  ASSERT(subsurface->color != NULL);
  ASSERT(subsurface->color->color.rgba.R > 0.099f);
  ASSERT(subsurface->color->color.rgba.G > 0.199f);
  ASSERT(subsurface->color->color.rgba.B > 0.299f);
  ASSERT(subsurface->anisotropy > 0.399f && subsurface->anisotropy < 0.401f);

  ak_free(doc);
  unlink(gltfPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(material_obj_adapter) {
  AkDoc       *doc;
  AkMaterial  *mat;
  AkMaterialClassicFeature *classic;
  char          dirTemplate[PATH_MAX];
  char         *tmpdir;
  char          objPath[PATH_MAX];
  char          mtlPath[PATH_MAX];
  char          objAlphaPath[PATH_MAX];
  const char   *tmpBase;

  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate, sizeof(dirTemplate), "%s/assetkit-material-obj-XXXXXX", tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(objPath, sizeof(objPath), "%s/material_adapter.obj", tmpdir);
  snprintf(mtlPath, sizeof(mtlPath), "%s/material_adapter.mtl", tmpdir);
  ASSERT(test_write_obj_material_files(objPath, mtlPath));

  doc = NULL;
  ASSERT(ak_load(&doc, objPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  mat = test_material_by_name(doc, "obj_opacity");
  if (!mat)
    mat = test_first_primitive_material(doc);
  ASSERT(mat != NULL);
  ASSERT(mat->name && strcmp(mat->name, "obj_opacity") == 0);
  ASSERT(mat->surface != NULL);
  ASSERT(mat->surface->opacity != NULL);
  ASSERT(test_material_opacity(mat, AK_TEXTURE_CHANNEL_R, 0.499f, 0.501f, true, false));
  ASSERT(mat->surface->flags & AK_MATERIAL_FLAG_ALPHA_BLEND);
  classic = (void*)ak_materialFeature(mat->surface, AK_MATERIAL_FEATURE_CLASSIC);
  ASSERT(classic != NULL);
  ASSERT(classic->transparency != NULL);
  ASSERT(classic->transparency->channels == AK_TEXTURE_CHANNEL_RGB);
  ASSERT(classic->transparency->color.rgba.R > 0.199f);
  ASSERT(classic->transparency->color.rgba.G > 0.299f);
  ASSERT(classic->transparency->color.rgba.B > 0.399f);

  ak_free(doc);

  snprintf(objAlphaPath, sizeof(objAlphaPath), "%s/vertex_alpha.obj", tmpdir);
  ASSERT(test_write_obj_vertex_alpha(objAlphaPath));

  doc = NULL;
  ASSERT(ak_load(&doc, objAlphaPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  mat = test_first_primitive_material(doc);
  ASSERT(mat != NULL);
  ASSERT(mat->surface != NULL);
  ASSERT(mat->surface->baseColor != NULL);
  ASSERT(mat->surface->baseColor->source == AK_MATERIAL_INPUT_VERTEX_COLOR);
  ASSERT(mat->surface->flags & AK_MATERIAL_FLAG_ALPHA_BLEND);

  ak_free(doc);
  unlink(objPath);
  unlink(mtlPath);
  unlink(objAlphaPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
