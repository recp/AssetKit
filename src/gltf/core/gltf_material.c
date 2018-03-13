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

void _assetkit_hide
gltf_materials(AkGLTFState * __restrict gst) {
  AkHeap     *heap;
  AkDoc      *doc;
  json_t     *jmaterials;
  AkLibItem  *libeffect, *libmat;
  AkMaterial *last_mat;
  size_t      jmatCount, i;

  heap       = gst->heap;
  doc        = gst->doc;
  libeffect  = ak_heap_calloc(heap, doc, sizeof(*libeffect));
  libmat     = ak_heap_calloc(heap, doc, sizeof(*libmat));
  last_mat   = NULL;

  jmaterials = json_object_get(gst->root, _s_gltf_materials);
  jmatCount  = json_array_size(jmaterials);

  for (i = 0; i < jmatCount; i++) {
    AkProfileCommon     *pcommon;
    AkTechniqueFx       *technfx;
    AkMaterial          *mat;
    AkEffect            *effect;
    AkMetallicRoughness *mtlrough;
    AkInstanceEffect    *ieff;
    json_t              *jmat, *jmtlrough, *ji;

    jmat     = json_array_get(jmaterials, i);
    pcommon  = gltf_cmnEffect(gst);
    effect   = ak_mem_parent(pcommon);
    technfx  = ak_heap_calloc(heap, pcommon, sizeof(*technfx));
    mtlrough = ak_heap_calloc(heap, technfx, sizeof(*mtlrough));
    mat      = ak_heap_calloc(heap, libmat,  sizeof(*mat));

    ak_setypeid(technfx, AKT_TECHNIQUE_FX);

    technfx->metallicRoughness = mtlrough;
    pcommon->technique         = technfx;

    ak_setId(mat, ak_id_gen(heap, mat, _s_gltf_id_metalrough));

    /* metallic roughness */

    /* default values */
    glm_vec4_copy(GLM_VEC4_ONE, mtlrough->baseColor.vec);
    mtlrough->metallic = mtlrough->roughness = 1.0f;

    if ((jmtlrough = json_object_get(jmat, _s_gltf_pbrMetalRough))) {
      if ((ji = json_object_get(jmtlrough, _s_gltf_baseColor))) {
        int32_t j;

        for (j = 0; j < 4; j++)
          mtlrough->baseColor.vec[j] = jsn_flt_at(ji, j);
      }

      jsn_flt_if(jmtlrough, _s_gltf_metalFac, &mtlrough->metallic);
      jsn_flt_if(jmtlrough, _s_gltf_roughFac, &mtlrough->roughness);

      if ((ji = json_object_get(jmtlrough, _s_gltf_metalRoughTex)))
        mtlrough->metalRoughTex = gltf_texref(gst, effect, mtlrough, ji);

      if ((ji = json_object_get(jmtlrough, _s_gltf_baseColorTex)))
        mtlrough->baseColorTex = gltf_texref(gst, effect, mtlrough, ji);

      /* TODO: extensions, extras */
    }

    ieff               = ak_heap_calloc(heap, mat, sizeof(*ieff));
    ieff->base.type    = AK_INSTANCE_EFFECT;
    ieff->base.url.ptr = effect;

    mat->effect = ieff;

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
