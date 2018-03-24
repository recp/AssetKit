/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf_material.h"
#include "gltf_profile.h"
#include "gltf_sampler.h"
#include "gltf_texture.h"
#include "gltf_enums.h"
#include "../../default/ak_def_material.h"

void _assetkit_hide
gltf_materials(AkGLTFState * __restrict gst) {
  AkHeap     *heap;
  AkDoc      *doc;
  json_t     *jmaterials;
  AkLibItem  *libeffect, *libmat;
  AkMaterial *last_mat;
  size_t      jmatCount, i;
  bool        specGlossExt;

  specGlossExt = ak_opt_get(AK_OPT_GLTF_EXT_SPEC_GLOSS);
  heap         = gst->heap;
  doc          = gst->doc;
  libeffect    = ak_heap_calloc(heap, doc, sizeof(*libeffect));
  libmat       = ak_heap_calloc(heap, doc, sizeof(*libmat));
  last_mat     = NULL;

  jmaterials = json_object_get(gst->root, _s_gltf_materials);
  jmatCount  = json_array_size(jmaterials);

  for (i = 0; i < jmatCount; i++) {
    AkProfileCommon      *pcommon;
    AkTechniqueFx        *technfx;
    AkTechniqueFxCommon  *cmnTechn;
    AkMaterial           *mat;
    AkEffect             *effect;
    AkMetallicRoughness  *metalRough;
    AkSpecularGlossiness *specGloss;
    AkInstanceEffect     *ieff;
    json_t               *jmat, *jmtlrough, *ji, *jext, *jsg;


    jmat       = json_array_get(jmaterials, i);
    pcommon    = gltf_cmnEffect(gst);
    effect     = ak_mem_parent(pcommon);
    technfx    = ak_heap_calloc(heap, pcommon, sizeof(*technfx));
    mat        = ak_heap_calloc(heap, libmat,  sizeof(*mat));
    cmnTechn   = NULL;
    metalRough = NULL;
    specGloss  = NULL;

    pcommon->technique = technfx;

    ak_setypeid(technfx, AKT_TECHNIQUE_FX);

    jext = json_object_get(jmat, _s_gltf_extensions);

    /* - PBR - */
    /* Specular Gloss Extension */
    if (specGlossExt
        && jext
        && (jsg = json_object_get(jext, _s_gltf_ext_pbrSpecGloss))) {
      int32_t j;

      specGloss = ak_heap_calloc(heap, technfx, sizeof(*specGloss));
      cmnTechn  = &specGloss->base;

      specGloss->base.type  = AK_MATERIAL_SPECULAR_GLOSSINES;
      specGloss->glossiness = 1.0f;
      glm_vec_copy(GLM_VEC4_ONE, specGloss->diffuse.vec);
      glm_vec_copy(GLM_VEC4_ONE, specGloss->specular.vec);

      if ((ji = json_object_get(jsg, _s_gltf_diffuseFactor))) {
        for (j = 0; j < 4; j++)
          specGloss->diffuse.vec[j] = jsn_flt_at(ji, j);
      }

      if ((ji = json_object_get(jsg, _s_gltf_specFactor))) {
        for (j = 0; j < 3; j++)
          specGloss->specular.vec[j] = jsn_flt_at(ji, j);
        specGloss->specular.vec[3] = 1.0f;
      }

      jsn_flt_if(jsg, _s_gltf_glossFactor, &specGloss->glossiness);

      if ((ji = json_object_get(jsg, _s_gltf_diffuseTexture)))
        specGloss->diffuseTex = gltf_texref(gst, effect, specGloss, ji);

      if ((ji = json_object_get(jsg, _s_gltf_specGlossTex)))
        specGloss->specGlossTex = gltf_texref(gst, effect, specGloss, ji);

      ak_setId(mat, ak_id_gen(heap, mat, _s_gltf_id_specgloss));
    }

    /* Metallic Roughness */
    if (!specGloss) {
      metalRough = ak_heap_calloc(heap, technfx, sizeof(*metalRough));
      cmnTechn   = &metalRough->base;

      metalRough->base.type = AK_MATERIAL_METALLIC_ROUGHNESS;
      metalRough->metallic  = metalRough->roughness = 1.0f;
      glm_vec4_copy(GLM_VEC4_ONE, metalRough->albedo.vec);

      if ((jmtlrough = json_object_get(jmat, _s_gltf_pbrMetalRough))) {
        if ((ji = json_object_get(jmtlrough, _s_gltf_baseColor))) {
          int32_t j;

          for (j = 0; j < 4; j++)
            metalRough->albedo.vec[j] = jsn_flt_at(ji, j);
        }

        jsn_flt_if(jmtlrough, _s_gltf_metalFac, &metalRough->metallic);
        jsn_flt_if(jmtlrough, _s_gltf_roughFac, &metalRough->roughness);

        if ((ji = json_object_get(jmtlrough, _s_gltf_metalRoughTex)))
          metalRough->metalRoughTex = gltf_texref(gst, effect, metalRough, ji);

        if ((ji = json_object_get(jmtlrough, _s_gltf_baseColorTex)))
          metalRough->albedoTex = gltf_texref(gst, effect, metalRough, ji);
      }

      ak_setId(mat, ak_id_gen(heap, mat, _s_gltf_id_metalrough));
    }

    /* Emission Map */
    if ((ji = json_object_get(jmat, _s_gltf_emissiveTex))) {
      AkColorDesc *colorDesc;

      colorDesc = ak_heap_calloc(heap, technfx, sizeof(*colorDesc));

      colorDesc->texture = gltf_texref(gst, effect, colorDesc, ji);
      cmnTechn->emission = colorDesc;
    }

    /* Occlusion Map */
    if ((ji = json_object_get(jmat, _s_gltf_occlusionTex))) {
      AkOcclusion *occlusion;

      occlusion           = ak_heap_alloc(heap, technfx, sizeof(*occlusion));
      occlusion->tex      = gltf_texref(gst, effect, occlusion, ji);
      occlusion->strength = 1.0f;

      jsn_flt_if(ji, _s_gltf_strength, &occlusion->strength);
      cmnTechn->occlusion = occlusion;
    }

    /* Normap Map */
    if ((ji = json_object_get(jmat, _s_gltf_normalTex))) {
      AkNormalMap *normal;

      normal        = ak_heap_alloc(heap, technfx, sizeof(*normal));
      normal->tex   = gltf_texref(gst, effect, normal, ji);
      normal->scale = 1.0f;

      jsn_flt_if(ji, _s_gltf_scale, &normal->scale);
      cmnTechn->normal = normal;
    }

    /* doubleSided */
    if ((ji = json_object_get(jmat, _s_gltf_doubleSided)))
      cmnTechn->doubleSided = json_boolean_value(ji);

    /* alphaMode */
    if ((ji = json_object_get(jmat, _s_gltf_alphaMode))) {
      AkOpaque opaque;
      opaque = gltf_alphaMode(json_string_value(ji));
      if (opaque != AK_OPAQUE_OPAQUE) {
        AkTransparent *transp;

        transp = ak_heap_calloc(heap, cmnTechn, sizeof(*transp));
        transp->amount = ak_def_transparency();
        transp->opaque = opaque;
        transp->cutoff = 0.5f;

        cmnTechn->transparent = transp;
      }
    }

    /* alphaCutoff */
    if (cmnTechn->transparent
        && (ji = json_object_get(jmat, _s_gltf_alphaCutoff))) {
      cmnTechn->transparent->cutoff = (float)json_number_value(ji);
    }

    technfx->common    = cmnTechn;
    ieff               = ak_heap_calloc(heap, mat, sizeof(*ieff));
    ieff->base.type    = AK_INSTANCE_EFFECT;
    ieff->base.url.ptr = effect;
    mat->effect        = ieff;

    if (last_mat)
      last_mat->next = mat;
    else
      libmat->chld = mat;
    last_mat = mat;

    libeffect->count++;
    libmat->count++;
  }

  doc->lib.effects   = libeffect;
  doc->lib.materials = libmat;
}
