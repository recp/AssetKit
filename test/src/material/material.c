/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "../../include/common.h"

#include <ak/assetkit.h>
#include <ak/material.h>
#include <ak/geom.h>
#include <ak/options.h>

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
        "<library_images>"
        "<image id=\"img_alpha\" name=\"alpha\"><init_from>alpha.png</init_from></image>"
        "<image id=\"img_normal\" name=\"normal\"><init_from>normal.png</init_from></image>"
        "<image id=\"img_height\" name=\"height\"><init_from>height.png</init_from></image>"
        "<image id=\"img_specular_level\" name=\"specular_level\"><init_from>specular_level.png</init_from></image>"
        "</library_images>\n"
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
        "<diffuse><color>0.2 0.3 0.4 1</color></diffuse>"
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
        "<effect id=\"effect_diffuse_tex\"><profile_COMMON>"
        "<newparam sid=\"surface_diffuse\"><surface type=\"2D\"><init_from>img_alpha</init_from></surface></newparam>"
        "<newparam sid=\"sampler_diffuse\"><sampler2D><source>surface_diffuse</source></sampler2D></newparam>"
        "<technique sid=\"common\"><lambert>"
        "<diffuse><texture texture=\"sampler_diffuse\" texcoord=\"UVSET0\">"
        "<extra><technique profile=\"MAYA\">"
        "<mirrorU>1</mirrorU><mirrorV>0</mirrorV><wrapU>1</wrapU><wrapV>0</wrapV>"
        "<repeatU>3</repeatU><repeatV>2</repeatV>"
        "<offsetU>0.25</offsetU><offsetV>0.5</offsetV><rotateUV>0.75</rotateUV>"
        "<blend_mode>MULTIPLY</blend_mode>"
        "</technique><technique profile=\"MAX3D\"><amount>0.6</amount></technique>"
        "<technique profile=\"OKINO\"><mix_with_previous_layer>1</mix_with_previous_layer></technique>"
        "</extra></texture></diffuse>"
        "</lambert></technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_fcollada_normal\"><profile_COMMON>"
        "<newparam sid=\"surface_normal\"><surface type=\"2D\"><init_from>img_normal</init_from></surface></newparam>"
        "<newparam sid=\"sampler_normal\"><sampler2D><source>surface_normal</source></sampler2D></newparam>"
        "<technique sid=\"common\">"
        "<extra><technique profile=\"FCOLLADA\"><bump bumptype=\"NORMALMAP\">"
        "<texture texture=\"sampler_normal\" texcoord=\"UVSET0\"/>"
        "</bump></technique></extra>"
        "<phong><diffuse><color>1 1 1 1</color></diffuse></phong>"
        "</technique></profile_COMMON></effect>\n"
        "<effect id=\"effect_max_height_specular\"><profile_COMMON>"
        "<newparam sid=\"surface_height\"><surface type=\"2D\"><init_from>img_height</init_from></surface></newparam>"
        "<newparam sid=\"sampler_height\"><sampler2D><source>surface_height</source></sampler2D></newparam>"
        "<newparam sid=\"surface_specular_level\"><surface type=\"2D\"><init_from>img_specular_level</init_from></surface></newparam>"
        "<newparam sid=\"sampler_specular_level\"><sampler2D><source>surface_specular_level</source></sampler2D></newparam>"
        "<technique sid=\"common\"><phong>"
        "<diffuse><color>1 1 1 1</color></diffuse>"
        "<specular><color>0.4 0.3 0.2 1</color></specular>"
        "</phong><extra><technique profile=\"OpenCOLLADA3dsMax\">"
        "<specularLevel><texture texture=\"sampler_specular_level\" texcoord=\"UVSET1\">"
        "<extra><technique profile=\"MAX3D\"><amount>0.8</amount></technique></extra>"
        "</texture></specularLevel>"
        "<bump bumptype=\"HEIGHTFIELD\"><texture texture=\"sampler_height\" texcoord=\"UVSET0\">"
        "<extra><technique profile=\"MAX3D\"><amount>0.65</amount></technique></extra>"
        "</texture></bump>"
        "</technique></extra></technique></profile_COMMON>"
        "<extra><technique profile=\"MAX3D\">"
        "<faceted>1</faceted><double_sided>1</double_sided><wireframe>1</wireframe>"
        "</technique></extra></effect>\n"
        "<effect id=\"effect_direct_bump\"><profile_COMMON>"
        "<newparam sid=\"surface_normal\"><surface type=\"2D\"><init_from>img_normal</init_from></surface></newparam>"
        "<newparam sid=\"sampler_normal\"><sampler2D><source>surface_normal</source></sampler2D></newparam>"
        "<technique sid=\"common\"><phong>"
        "<diffuse><color>1 1 1 1</color></diffuse>"
        "<bump><texture texture=\"sampler_normal\" texcoord=\"UVSET0\"/></bump>"
        "</phong></technique></profile_COMMON></effect>\n"
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
        "<material id=\"mat_diffuse_tex\" name=\"diffuse_tex\"><instance_effect url=\"#effect_diffuse_tex\"/></material>"
        "<material id=\"mat_fcollada_normal\" name=\"fcollada_normal\"><instance_effect url=\"#effect_fcollada_normal\"/></material>"
        "<material id=\"mat_max_height_specular\" name=\"max_height_specular\"><instance_effect url=\"#effect_max_height_specular\"/></material>"
        "<material id=\"mat_direct_bump\" name=\"direct_bump\"><instance_effect url=\"#effect_direct_bump\"/></material>"
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
        "Pr 0.35\n"
        "Pm 0.65\n"
        "map_Pr roughness.png\n"
        "map_Pm metallic.png\n"
        "Ps 0.25\n"
        "Pc 2\n"
        "Pcr 0.15\n"
        "aniso 0.45\n"
        "anisor 0.55\n"
        "map_Ps sheen.png\n"
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
test_write_transparency_bugfix_dae(const char *path,
                                   const char *authoringTool,
                                   const char *opaqueAttr,
                                   float       amount) {
  FILE *file;
  int   written;
  bool  ok;

  file = fopen(path, "wb");
  if (!file)
    return false;

  written = fprintf(
    file,
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
    "<asset><contributor><authoring_tool>%s</authoring_tool></contributor>"
    "<up_axis>Y_UP</up_axis></asset>\n"
    "<library_effects><effect id=\"effect\"><profile_COMMON>"
    "<technique sid=\"common\"><lambert>"
    "<diffuse><color>1 1 1 1</color></diffuse>"
    "<transparent%s><color>0 0 0 1</color></transparent>"
    "<transparency><float>%.9g</float></transparency>"
    "</lambert></technique></profile_COMMON></effect></library_effects>\n"
    "<library_materials><material id=\"material\" name=\"material\">"
    "<instance_effect url=\"#effect\"/></material></library_materials>\n"
    "<library_visual_scenes><visual_scene id=\"Scene\"/></library_visual_scenes>"
    "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
    "</COLLADA>\n",
    authoringTool,
    opaqueAttr ? opaqueAttr : "",
    amount);

  ok = written >= 0;
  if (fclose(file) != 0)
    ok = false;

  return ok;
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
        "{\"name\":\"textured_roles\","
        "\"pbrMetallicRoughness\":{\"baseColorTexture\":{\"index\":0},"
        "\"metallicRoughnessTexture\":{\"index\":1}}},"
        "{\"name\":\"specular_glossiness_texture\","
        "\"extensions\":{\"KHR_materials_pbrSpecularGlossiness\":{"
        "\"specularGlossinessTexture\":{\"index\":0}}}},"
        "{\"name\":\"volume_scatter\","
        "\"extras\":{\"authorNote\":\"preserve only when requested\"},"
        "\"extensions\":{\"KHR_materials_volume_scatter\":{"
        "\"multiscatterColor\":[0.1,0.2,0.3],\"scatterAnisotropy\":0.4}}}"
        "],"
        "\"images\":[{\"uri\":\"pixel.png\"}],"
        "\"textures\":[{\"source\":0},{\"source\":0}],"
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
test_tree_has_name(AkTree *tree, const char *name) {
  AkTree *child;

  if (!tree || !name)
    return false;

  if (tree->name && strcmp(tree->name, name) == 0)
    return true;

  for (child = tree->chld; child; child = child->next) {
    if (test_tree_has_name(child, name))
      return true;
  }

  return false;
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

static
bool
test_load_material_opacity(const char *path,
                           bool        bugfixes,
                           float      *opacityOut,
                           uint32_t   *flagsOut) {
  AkDoc      *doc;
  AkMaterial *mat;
  uintptr_t   previousBugfixes;
  AkResult    result;
  bool        ok;

  doc              = NULL;
  previousBugfixes = ak_opt_get(AK_OPT_BUGFIXES);
  ak_opt_set(AK_OPT_BUGFIXES, bugfixes);
  result = ak_load(&doc, path, AK_FILE_TYPE_AUTO);
  ak_opt_set(AK_OPT_BUGFIXES, previousBugfixes);

  mat = result == AK_OK ? test_material_by_name(doc, "material") : NULL;
  ok  = mat && mat->surface && mat->surface->opacity;
  if (ok && opacityOut)
    *opacityOut = ak_materialOpacityFactor(mat->surface);
  if (ok && flagsOut)
    *flagsOut = mat->surface->flags;

  ak_free(doc);
  return ok;
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
  AkMaterialBinding          objectBinding;
  AkMaterialInputBinding     inputBinding;
  AkMaterialVariantMapping   legacyMapping;
  AkMaterialPropertySet      propertySet;
  AkMaterialProperty         properties[2];
  AkDoc                      doc;
  AkMeshPrimitive            prim;
  AkMeshPrimitive            otherPrim;
  AkInstanceGeometry         instGeom;
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
  memset(&objectBinding, 0, sizeof(objectBinding));
  memset(&inputBinding, 0, sizeof(inputBinding));
  memset(&legacyMapping, 0, sizeof(legacyMapping));
  memset(&propertySet, 0, sizeof(propertySet));
  memset(properties, 0, sizeof(properties));
  memset(&doc, 0, sizeof(doc));
  memset(&prim, 0, sizeof(prim));
  memset(&otherPrim, 0, sizeof(otherPrim));
  memset(&instGeom, 0, sizeof(instGeom));

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

  objectBinding.material = &variantMaterial;
  objectBinding.primitive = &prim;
  objectBinding.scope = AK_MATERIAL_BIND_OBJECT;
  objectBinding.propertyIndex = UINT32_MAX;
  objectBinding.variantIndex = UINT32_MAX;
  inputBinding.semantic = "UVSET0";
  inputBinding.inputSemantic = "TEXCOORD";
  inputBinding.inputSet = 2;
  objectBinding.inputBindings = &inputBinding;
  instGeom.objectBindings = &objectBinding;

  memset(&prim, 0, sizeof(prim));
  ASSERT(ak_materialResolve(&prim, &instGeom, UINT32_MAX, &resolved));
  ASSERT(resolved.material == &variantMaterial);
  ASSERT(resolved.surface == &variantSurface);
  ASSERT(resolved.binding == &objectBinding);
  ASSERT(resolved.variantIndex == UINT32_MAX);
  ASSERT(!ak_materialResolve(&otherPrim, &instGeom, UINT32_MAX, &resolved));

  textureRef.texcoord = "UVSET0";
  textureRef.slot = 0;
  ASSERT(ak_materialTextureSlot(&prim, &instGeom, &textureRef) == 2);
  ASSERT(ak_materialTextureSlot(&otherPrim, &instGeom, &textureRef) == 0);

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
  AkMaterial  *diffuseTex;
  AkMaterial  *fcolladaNormal;
  AkMaterial  *maxHeightSpecular;
  AkMaterial  *directBump;
  AkMaterialSpecularFeature *specularLevel;
  AkDoc       *noExtraDoc;
  AkMaterial  *noExtraEdge;
  char          dirTemplate[PATH_MAX];
  char         *tmpdir;
  char          daePath[PATH_MAX];
  const char   *tmpBase;
  uintptr_t     preserveExtras;
  AkResult      loadResult;

  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate, sizeof(dirTemplate), "%s/assetkit-material-dae-XXXXXX", tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/material_adapter.dae", tmpdir);
  ASSERT(test_write_material_dae(daePath));

  noExtraDoc = NULL;
  ASSERT(ak_load(&noExtraDoc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && noExtraDoc);
  noExtraEdge = test_material_by_name(noExtraDoc, "edge");
  ASSERT(noExtraEdge && !ak_extra(noExtraEdge));
  ak_free(noExtraDoc);

  preserveExtras = ak_opt_get(AK_OPT_PRESERVE_EXTRAS);
  ak_opt_set(AK_OPT_PRESERVE_EXTRAS, true);
  doc = NULL;
  loadResult = ak_load(&doc, daePath, AK_FILE_TYPE_AUTO);
  ak_opt_set(AK_OPT_PRESERVE_EXTRAS, preserveExtras);
  ASSERT(loadResult == AK_OK && doc);

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
  ASSERT(rgbOne && rgbOne->surface && rgbOne->surface->baseColor);
  ASSERT(rgbOne->surface->baseColor->colorSpace == AK_TEXTURE_COLORSPACE_LINEAR);
  ASSERT(fabsf(rgbOne->surface->baseColor->color.rgba.R
               - ak_sRGB_linearf(0.2f)) < 0.001f);
  ASSERT(fabsf(rgbOne->surface->baseColor->color.rgba.G
               - ak_sRGB_linearf(0.3f)) < 0.001f);
  ASSERT(fabsf(rgbOne->surface->baseColor->color.rgba.B
               - ak_sRGB_linearf(0.4f)) < 0.001f);
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

  diffuseTex = test_material_by_name(doc, "diffuse_tex");
  ASSERT(diffuseTex && diffuseTex->surface && diffuseTex->surface->baseColor);
  ASSERT(diffuseTex->surface->baseColor->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(diffuseTex->surface->baseColor->channels == AK_TEXTURE_CHANNEL_RGB);
  ASSERT(diffuseTex->surface->baseColor->colorSpace == AK_TEXTURE_COLORSPACE_SRGB);
  ASSERT(diffuseTex->surface->baseColor->texture);
  ASSERT(diffuseTex->surface->baseColor->texture->texture);
  ASSERT(diffuseTex->surface->baseColor->texture->texture->sampler);
  ASSERT(diffuseTex->surface->baseColor->texture->texture->sampler->wrapS
         == AK_WRAP_MODE_MIRROR);
  ASSERT(diffuseTex->surface->baseColor->texture->texture->sampler->wrapT
         == AK_WRAP_MODE_CLAMP);
  ASSERT(diffuseTex->surface->baseColor->texture->transform);
  ASSERT(fabsf(diffuseTex->surface->baseColor->texture->transform->scale[0]
               - 3.0f) < 0.001f);
  ASSERT(fabsf(diffuseTex->surface->baseColor->texture->transform->scale[1]
               - 2.0f) < 0.001f);
  ASSERT(fabsf(diffuseTex->surface->baseColor->texture->transform->offset[0]
               - 0.25f) < 0.001f);
  ASSERT(fabsf(diffuseTex->surface->baseColor->texture->transform->offset[1]
               - 0.5f) < 0.001f);
  ASSERT(fabsf(diffuseTex->surface->baseColor->texture->transform->rotation
               - 0.75f) < 0.001f);
  ASSERT(ak_extra(diffuseTex->surface->baseColor->texture) != NULL);

  fcolladaNormal = test_material_by_name(doc, "fcollada_normal");
  ASSERT(fcolladaNormal && fcolladaNormal->surface);
  ASSERT(fcolladaNormal->surface->normal);
  ASSERT(fcolladaNormal->surface->normal->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(fcolladaNormal->surface->normal->colorSpace
         == AK_TEXTURE_COLORSPACE_LINEAR);
  ASSERT(fcolladaNormal->surface->normal->channels == AK_TEXTURE_CHANNEL_RGB);
  ASSERT(!(fcolladaNormal->surface->normal->flags
           & AK_MATERIAL_INPUT_FLAG_HEIGHT));

  maxHeightSpecular = test_material_by_name(doc, "max_height_specular");
  ASSERT(maxHeightSpecular && maxHeightSpecular->surface);
  ASSERT(maxHeightSpecular->surface->normal);
  ASSERT(maxHeightSpecular->surface->normal->source
         == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(maxHeightSpecular->surface->normal->colorSpace
         == AK_TEXTURE_COLORSPACE_LINEAR);
  ASSERT(maxHeightSpecular->surface->normal->channels == AK_TEXTURE_CHANNEL_R);
  ASSERT(maxHeightSpecular->surface->normal->flags
         & AK_MATERIAL_INPUT_FLAG_HEIGHT);
  ASSERT(fabsf(ak_materialInputScalar(maxHeightSpecular->surface->normal, 1.0f)
               - 0.65f) < 0.001f);
  ASSERT(ak_extra(maxHeightSpecular->surface->normal->texture) != NULL);
  ASSERT(maxHeightSpecular->surface->flags
         & AK_MATERIAL_FLAG_DOUBLE_SIDED);
  specularLevel = (AkMaterialSpecularFeature *)ak_materialFeature(
                    maxHeightSpecular->surface,
                    AK_MATERIAL_FEATURE_SPECULAR);
  ASSERT(specularLevel && specularLevel->factor);
  ASSERT(specularLevel->factor->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(specularLevel->factor->colorSpace == AK_TEXTURE_COLORSPACE_LINEAR);
  ASSERT(specularLevel->factor->channels == AK_TEXTURE_CHANNEL_R);
  ASSERT(fabsf(ak_materialInputScalar(specularLevel->factor, 1.0f)
               - 0.8f) < 0.001f);
  ASSERT(ak_extra(specularLevel->factor->texture) != NULL);

  directBump = test_material_by_name(doc, "direct_bump");
  ASSERT(directBump && directBump->surface && directBump->surface->normal);
  ASSERT(directBump->surface->normal->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(!(directBump->surface->normal->flags
           & AK_MATERIAL_INPUT_FLAG_HEIGHT));
  ASSERT(ak_extra(edge));
  ASSERT(test_tree_has_name(ak_extra(edge), "profile_COMMON"));
  ASSERT(test_tree_has_name(ak_extra(edge), "technique"));

  for (AkMaterial *mat = doc->lib.materials.first; mat; mat = mat->next)
    ASSERT(ak_userData(mat) == NULL);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(material_dae_transparency_bugfixes) {
  char        dirTemplate[PATH_MAX];
  char        daePath[PATH_MAX];
  char       *tmpdir;
  const char *tmpBase;
  float       opacity;
  uint32_t    flags;

  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-material-dae-transparency-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  snprintf(daePath, sizeof(daePath), "%s/transparency.dae", tmpdir);

  ASSERT(test_write_transparency_bugfix_dae(
    daePath,
    "Maya 7.0 | ColladaMaya v2.03b Jul 27 2006 at 18:43:34 | FCollada v1.13",
    NULL,
    0.0f));
  ASSERT(test_load_material_opacity(daePath, false, &opacity, &flags));
  ASSERT(opacity == 0.0f);
  ASSERT(flags & AK_MATERIAL_FLAG_ALPHA_BLEND);
  ASSERT(!(flags & AK_MATERIAL_FLAG_ALPHA_MASK));
  ASSERT(test_load_material_opacity(daePath, true, &opacity, &flags));
  ASSERT(opacity == 1.0f);
  ASSERT(!(flags & (AK_MATERIAL_FLAG_ALPHA_BLEND
                    | AK_MATERIAL_FLAG_ALPHA_MASK)));

  /* An explicit A_ONE is authored intent, not the exporter sentinel. */
  ASSERT(test_write_transparency_bugfix_dae(
    daePath,
    "Maya 7.0 | ColladaMaya v2.03b Jul 27 2006 at 18:43:34 | FCollada v1.13",
    " opaque=\"A_ONE\"",
    0.0f));
  ASSERT(test_load_material_opacity(daePath, true, &opacity, &flags));
  ASSERT(opacity == 0.0f);
  ASSERT(flags & AK_MATERIAL_FLAG_ALPHA_BLEND);

  /* Version prefixes are not the affected producer releases. */
  ASSERT(test_write_transparency_bugfix_dae(
    daePath,
    "Maya 7.0 | ColladaMaya v2.03beta | FCollada v1.13",
    NULL,
    0.0f));
  ASSERT(test_load_material_opacity(daePath, true, &opacity, &flags));
  ASSERT(opacity == 0.0f);
  ASSERT(flags & AK_MATERIAL_FLAG_ALPHA_BLEND);

  ASSERT(test_write_transparency_bugfix_dae(
    daePath,
    "Maya 7.0 | ColladaMaya v2.03b | FCollada v1.130",
    NULL,
    0.0f));
  ASSERT(test_load_material_opacity(daePath, true, &opacity, &flags));
  ASSERT(opacity == 0.0f);
  ASSERT(flags & AK_MATERIAL_FLAG_ALPHA_BLEND);

  /* Omitted A_ONE remains spec-defined for unrelated exporters. */
  ASSERT(test_write_transparency_bugfix_dae(daePath,
                                            "Generic DAE Exporter 1.0",
                                            NULL,
                                            0.0f));
  ASSERT(test_load_material_opacity(daePath, true, &opacity, &flags));
  ASSERT(opacity == 0.0f);
  ASSERT(flags & AK_MATERIAL_FLAG_ALPHA_BLEND);

  /* ColladaMaya v2.01 / FCollada v1.11 did not emit Seymour's sentinel. */
  ASSERT(test_write_transparency_bugfix_dae(
    daePath,
    "Maya 7.0 | ColladaMaya v2.01 Jun 9 2006 at 16:08:19 | FCollada v1.11",
    NULL,
    0.0f));
  ASSERT(test_load_material_opacity(daePath, true, &opacity, &flags));
  ASSERT(opacity == 0.0f);
  ASSERT(flags & AK_MATERIAL_FLAG_ALPHA_BLEND);

  ASSERT(test_write_transparency_bugfix_dae(daePath,
                                            "Google SketchUp 6.4.112",
                                            NULL,
                                            0.25f));
  ASSERT(test_load_material_opacity(daePath, true, &opacity, &flags));
  ASSERT(fabsf(opacity - 0.75f) < 0.0001f);

  ASSERT(test_write_transparency_bugfix_dae(daePath,
                                            "Google SketchUp 7.1.0",
                                            NULL,
                                            0.25f));
  ASSERT(test_load_material_opacity(daePath, true, &opacity, &flags));
  ASSERT(fabsf(opacity - 0.75f) < 0.0001f);

  ASSERT(test_write_transparency_bugfix_dae(daePath,
                                            "Google SketchUp 7.1.1",
                                            NULL,
                                            0.25f));
  ASSERT(test_load_material_opacity(daePath, true, &opacity, &flags));
  ASSERT(fabsf(opacity - 0.25f) < 0.0001f);

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
  AkMaterial  *textured;
  AkMaterial  *specularGlossiness;
  AkMaterial  *scatter;
  AkDoc       *docWithExtras;
  AkMaterial  *scatterWithExtras;
  AkMaterialSubsurfaceFeature *subsurface;
  AkMaterialSpecularGlossinessFeature *sg;
  char          dirTemplate[PATH_MAX];
  char         *tmpdir;
  char          gltfPath[PATH_MAX];
  const char   *tmpBase;
  uintptr_t     preserveExtras;
  AkResult      loadResult;

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

  ASSERT(opaque->surface->baseColor->colorSpace == AK_TEXTURE_COLORSPACE_LINEAR);
  ASSERT(opaque->surface->metallic->colorSpace == AK_TEXTURE_COLORSPACE_LINEAR);
  ASSERT(opaque->surface->roughness->colorSpace == AK_TEXTURE_COLORSPACE_LINEAR);

  textured = test_material_by_name(doc, "textured_roles");
  ASSERT(textured && textured->surface);
  ASSERT(textured->surface->baseColor->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(textured->surface->baseColor->colorSpace == AK_TEXTURE_COLORSPACE_SRGB);
  ASSERT(textured->surface->metallic->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(textured->surface->metallic->colorSpace == AK_TEXTURE_COLORSPACE_LINEAR);
  ASSERT(textured->surface->roughness->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(textured->surface->roughness->colorSpace == AK_TEXTURE_COLORSPACE_LINEAR);

  specularGlossiness = test_material_by_name(doc, "specular_glossiness_texture");
  ASSERT(specularGlossiness && specularGlossiness->surface);
  sg = (void*)ak_materialFeature(specularGlossiness->surface,
                                 AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS);
  ASSERT(sg && sg->specular && sg->glossiness);
  ASSERT(sg->specular->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(sg->specular->colorSpace == AK_TEXTURE_COLORSPACE_SRGB);
  ASSERT(sg->glossiness->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(sg->glossiness->colorSpace == AK_TEXTURE_COLORSPACE_LINEAR);

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
  ASSERT(ak_extra(scatter));
  ASSERT(test_tree_has_name(ak_extra(scatter), "extensions"));
  ASSERT(!test_tree_has_name(ak_extra(scatter), "extras"));

  ak_free(doc);

  preserveExtras = ak_opt_get(AK_OPT_PRESERVE_EXTRAS);
  ak_opt_set(AK_OPT_PRESERVE_EXTRAS, true);
  docWithExtras = NULL;
  loadResult = ak_load(&docWithExtras, gltfPath, AK_FILE_TYPE_GLTF);
  ak_opt_set(AK_OPT_PRESERVE_EXTRAS, preserveExtras);
  ASSERT(loadResult == AK_OK && docWithExtras);
  scatterWithExtras = test_material_by_name(docWithExtras, "volume_scatter");
  ASSERT(scatterWithExtras && ak_extra(scatterWithExtras));
  ASSERT(test_tree_has_name(ak_extra(scatterWithExtras), "extensions"));
  ASSERT(test_tree_has_name(ak_extra(scatterWithExtras), "extras"));

  ak_free(docWithExtras);
  unlink(gltfPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(material_obj_adapter) {
  AkDoc       *doc;
  AkMaterial  *mat;
  AkMaterialClassicFeature *classic;
  AkMaterialClearcoatFeature *clearcoat;
  AkMaterialSheenFeature *sheen;
  AkMaterialAnisotropyFeature *anisotropy;
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
  ASSERT(mat->surface->roughness != NULL);
  ASSERT(mat->surface->metallic != NULL);
  ASSERT(mat->surface->roughness->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(mat->surface->metallic->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(ak_materialRoughnessFactor(mat->surface) > 0.349f);
  ASSERT(ak_materialRoughnessFactor(mat->surface) < 0.351f);
  ASSERT(ak_materialMetallicFactor(mat->surface) > 0.649f);
  ASSERT(ak_materialMetallicFactor(mat->surface) < 0.651f);
  clearcoat = (AkMaterialClearcoatFeature *)ak_materialFeature(
    mat->surface, AK_MATERIAL_FEATURE_CLEARCOAT);
  sheen = (AkMaterialSheenFeature *)ak_materialFeature(
    mat->surface, AK_MATERIAL_FEATURE_SHEEN);
  anisotropy = (AkMaterialAnisotropyFeature *)ak_materialFeature(
    mat->surface, AK_MATERIAL_FEATURE_ANISOTROPY);
  ASSERT(clearcoat != NULL);
  ASSERT(sheen != NULL);
  ASSERT(anisotropy != NULL);
  ASSERT(clearcoat->factor != NULL);
  ASSERT(clearcoat->roughness != NULL);
  ASSERT(sheen->color != NULL);
  ASSERT(sheen->color->source == AK_MATERIAL_INPUT_TEXTURE);
  ASSERT(anisotropy->strength != NULL);
  ASSERT(anisotropy->rotation != NULL);
  ASSERT(ak_materialInputScalar(clearcoat->factor, 0.0f) > 0.499f);
  ASSERT(ak_materialInputScalar(clearcoat->factor, 0.0f) < 0.501f);
  ASSERT(ak_materialInputScalar(clearcoat->roughness, 0.0f) > 0.149f);
  ASSERT(ak_materialInputScalar(clearcoat->roughness, 0.0f) < 0.151f);
  ASSERT(ak_materialInputScalar(sheen->color, 0.0f) > 0.249f);
  ASSERT(ak_materialInputScalar(sheen->color, 0.0f) < 0.251f);
  ASSERT(ak_materialInputScalar(anisotropy->strength, 0.0f) > 0.449f);
  ASSERT(ak_materialInputScalar(anisotropy->strength, 0.0f) < 0.451f);
  ASSERT(ak_materialInputScalar(anisotropy->rotation, 0.0f) > 0.549f);
  ASSERT(ak_materialInputScalar(anisotropy->rotation, 0.0f) < 0.551f);
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
