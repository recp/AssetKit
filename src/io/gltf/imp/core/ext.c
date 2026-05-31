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

#include "ext.h"
#include "../ext/decoder.h"
#include "../ext/instancing.h"
#include "../ext/lights.h"
#include "../ext/variants.h"

typedef struct AkGLTFExtName {
  const char *name;
  size_t      nameSize;
} AkGLTFExtName;

#define GLTF_EXT_NAME(NAME) { _s_gltf_ ## NAME, sizeof(#NAME) - 1 }

static
int
gltf_extNameCmp(const void * __restrict a,
                const void * __restrict b) {
  const AkGLTFExtName *extname;
  const char *val;
  size_t      valSize;
  size_t      minSize;
  int         res;

  extname = b;
  if (!(val = json_string(a)))
    return -1;

  valSize = (size_t)((const json_t *)a)->valsize;
  minSize = valSize < extname->nameSize ? valSize : extname->nameSize;
  res     = strncmp(val, extname->name, minSize);

  if (res != 0)                    return res;
  if (valSize < extname->nameSize) return -1;
  if (valSize > extname->nameSize) return 1;

  return 0;
}

static inline
bool
gltf_extNameSearch(const json_t        * __restrict ext,
                   const AkGLTFExtName * __restrict names,
                   size_t                           count) {
  return ext && bsearch(ext,
                        names,
                        count,
                        sizeof(names[0]),
                        gltf_extNameCmp) != NULL;
}

static
bool
gltf_ext_preserved_supported(const json_t * __restrict ext) {
  static const AkGLTFExtName names[] = {
    GLTF_EXT_NAME(ADOBE_materials_clearcoat_specular),
    GLTF_EXT_NAME(ADOBE_materials_clearcoat_tint),
    GLTF_EXT_NAME(ADOBE_materials_thin_transparency),
    GLTF_EXT_NAME(AGI_articulations),
    GLTF_EXT_NAME(AGI_stk_metadata),
    GLTF_EXT_NAME(CESIUM_primitive_outline),
    GLTF_EXT_NAME(EXT_gsplat_compression_spz),
    GLTF_EXT_NAME(EXT_lights_ies),
    GLTF_EXT_NAME(EXT_lights_image_based),
    GLTF_EXT_NAME(EXT_mesh_manifold),
    GLTF_EXT_NAME(EXT_mesh_primitive_restart),
    GLTF_EXT_NAME(EXT_texture_astc),
    GLTF_EXT_NAME(FB_geometry_metadata),
    GLTF_EXT_NAME(GODOT_single_root),
    GLTF_EXT_NAME(GRIFFEL_bim_data),
    GLTF_EXT_NAME(KHR_techniques_webgl),
    GLTF_EXT_NAME(KHR_xmp),
    GLTF_EXT_NAME(MPEG_accessor_timed),
    GLTF_EXT_NAME(MPEG_animation_timing),
    GLTF_EXT_NAME(MPEG_audio_spatial),
    GLTF_EXT_NAME(MPEG_buffer_circular),
    GLTF_EXT_NAME(MPEG_media),
    GLTF_EXT_NAME(MPEG_mesh_linking),
    GLTF_EXT_NAME(MPEG_scene_dynamic),
    GLTF_EXT_NAME(MPEG_texture_video),
    GLTF_EXT_NAME(MPEG_viewport_recommended),
    GLTF_EXT_NAME(MSFT_lod),
    GLTF_EXT_NAME(MSFT_packing_normalRoughnessMetallic),
    GLTF_EXT_NAME(MSFT_packing_occlusionRoughnessMetallic),
    GLTF_EXT_NAME(MSFT_texture_dds),
    GLTF_EXT_NAME(NV_materials_mdl)
  };

  return gltf_extNameSearch(ext, names, AK_ARRAY_LEN(names));
}

static
bool
gltf_ext_supported(AkGLTFState      * __restrict gst,
                   const json_t     * __restrict ext) {
  static const AkGLTFExtName names[] = {
    GLTF_EXT_NAME(EXT_mesh_gpu_instancing),
    GLTF_EXT_NAME(EXT_texture_webp),
    GLTF_EXT_NAME(KHR_animation_pointer),
    GLTF_EXT_NAME(KHR_gaussian_splatting),
    GLTF_EXT_NAME(KHR_lights_punctual),
    GLTF_EXT_NAME(KHR_materials_anisotropy),
    GLTF_EXT_NAME(KHR_materials_clearcoat),
    GLTF_EXT_NAME(KHR_materials_diffuse_transmission),
    GLTF_EXT_NAME(KHR_materials_dispersion),
    GLTF_EXT_NAME(KHR_materials_emissive_strength),
    GLTF_EXT_NAME(KHR_materials_ior),
    GLTF_EXT_NAME(KHR_materials_iridescence),
    GLTF_EXT_NAME(KHR_materials_pbrSpecularGlossiness),
    GLTF_EXT_NAME(KHR_materials_sheen),
    GLTF_EXT_NAME(KHR_materials_specular),
    GLTF_EXT_NAME(KHR_materials_transmission),
    GLTF_EXT_NAME(KHR_materials_unlit),
    GLTF_EXT_NAME(KHR_materials_variants),
    GLTF_EXT_NAME(KHR_materials_volume),
    GLTF_EXT_NAME(KHR_materials_volume_scatter),
    GLTF_EXT_NAME(KHR_mesh_quantization),
    GLTF_EXT_NAME(KHR_node_visibility),
    GLTF_EXT_NAME(KHR_texture_transform),
    GLTF_EXT_NAME(KHR_xmp_json_ld)
  };

  if (!ext)
    return false;

  if (gltf_extNameSearch(ext, names, AK_ARRAY_LEN(names)))
    return true;

  if (GLTF_JSON_VAL_EQ(ext, EXT_meshopt_compression)
      || GLTF_JSON_VAL_EQ(ext, KHR_meshopt_compression))
    return gltf_ext_meshopt(gst);
  if (GLTF_JSON_VAL_EQ(ext, KHR_draco_mesh_compression))
    return gltf_ext_draco(gst);
  if (GLTF_JSON_VAL_EQ(ext, KHR_texture_basisu))
    return gltf_ext_ktx2(gst);
  if (gltf_ext_preserved_supported(ext))
    return true;

  return false;
}

AK_HIDE
void
gltf_exts(json_t * __restrict jext,
          void   * __restrict userdata) {
  AkGLTFState        *gst;
  const json_array_t *jexts;
  json_t             *it;

  gst = userdata;

  if (!(jexts = json_array(jext)))
    return;

  for (it = (void *)jexts->base.value; it; it = it->next) {
    if (GLTF_JSON_VAL_EQ(it, KHR_animation_pointer))
      gst->animPointerRequired = true;

    if (!gltf_ext_supported(gst, it)) {
      gst->stop = true;
      return;
    }
  }
}

AK_HIDE
void
gltf_ext_root(json_t * __restrict jext,
              void   * __restrict userdata) {
  AkGLTFState *gst;
  json_t      *jpunctual;
  json_t      *jlights;
  json_t      *jvariantsExt;
  json_t      *jvariants;

  gst = userdata;
  if (!jext)
    return;

  if ((jpunctual = GLTF_JSON_GET(jext, KHR_lights_punctual))
      && (jlights = GLTF_JSON_GET8(jpunctual, lights))) {
    gltf_ext_lights(gst, jlights);
  }

  if ((jvariantsExt = GLTF_JSON_GET(jext, KHR_materials_variants))
      && (jvariants = GLTF_JSON_GET8(jvariantsExt, variants))) {
    gltf_ext_materialVariants(gst, jvariants);
  }
}

AK_HIDE
bool
gltf_ext_node(AkGLTFState * __restrict gst,
              AkNode      * __restrict node,
              const json_t * __restrict jext) {
  json_t  *jvis;
  json_t  *jvisible;
  json_t  *jinstancing;

  if (!gst || !node || !jext)
    return true;

  jvis     = GLTF_JSON_GET(jext, KHR_node_visibility);
  jvisible = jvis ? GLTF_JSON_GET8(jvis, visible) : NULL;

  if (jvisible)
    node->visible = json_bool(jvisible, true);

  if ((jinstancing = GLTF_JSON_GET(jext, EXT_mesh_gpu_instancing))) {
    node->instancing = gltf_ext_meshGPUInstancing(gst, node, jinstancing);
    if (gst->stop)
      return false;
  }

  return gltf_ext_nodeLight(gst, node, jext);
}

AK_HIDE
void
gltf_ext_close(AkGLTFState * __restrict gst) {
  gltf_ext_decoderClose(gst);
}
