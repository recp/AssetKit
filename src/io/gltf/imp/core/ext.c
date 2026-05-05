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

static
bool
gltf_ext_supported(AkGLTFState      * __restrict gst,
                   const json_t     * __restrict ext) {
  if (!ext)
    return false;

  if (json_val_eq(ext, _s_gltf_KHR_texture_transform))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_clearcoat))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_emissive_strength))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_ior))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_sheen))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_iridescence))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_volume))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_anisotropy))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_dispersion))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_diffuse_transmission))
    return true;
  if (json_val_eq(ext, _s_gltf_ext_pbrSpecGloss))
    return true;
  if (json_val_eq(ext, _s_gltf_ext_KHR_materials_specular))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_transmission))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_unlit))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_animation_pointer))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_node_visibility))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_mesh_quantization))
    return true;
  if (json_val_eq(ext, _s_gltf_EXT_meshopt_compression)
      || json_val_eq(ext, _s_gltf_KHR_meshopt_compression))
    return gltf_ext_meshopt(gst);
  if (json_val_eq(ext, _s_gltf_KHR_draco_mesh_compression))
    return gltf_ext_draco(gst);
  if (json_val_eq(ext, _s_gltf_KHR_lights_punctual))
    return true;
  if (json_val_eq(ext, _s_gltf_EXT_mesh_gpu_instancing))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_variants))
    return true;
  if (json_val_eq(ext, _s_gltf_EXT_texture_webp))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_texture_basisu))
    return gltf_ext_ktx2(gst);
  if (json_val_eq(ext, _s_gltf_KHR_xmp_json_ld))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_gaussian_splatting))
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
    if (json_val_eq(it, _s_gltf_KHR_animation_pointer))
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

  if ((jpunctual = json_get(jext, _s_gltf_KHR_lights_punctual))
      && (jlights = json_get(jpunctual, _s_gltf_lights))) {
    gltf_ext_lights(gst, jlights);
  }

  if ((jvariantsExt = json_get(jext, _s_gltf_KHR_materials_variants))
      && (jvariants = json_get(jvariantsExt, _s_gltf_variants))) {
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

  jvis     = json_get(jext, _s_gltf_KHR_node_visibility);
  jvisible = jvis ? json_get(jvis, _s_gltf_visible) : NULL;
  if (jvisible)
    node->visible = json_bool(jvisible, true);

  jinstancing = json_get(jext, _s_gltf_EXT_mesh_gpu_instancing);
  if (jinstancing) {
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
