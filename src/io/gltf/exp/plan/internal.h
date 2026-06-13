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

#ifndef assetkit_gltf_exp_plan_internal_h
#define assetkit_gltf_exp_plan_internal_h

#include "../plan.h"
#include "../extra.h"
#include "../image.h"
#include "../material.h"
#include "../mesh.h"
#include "../../../common/primitive.h"
#include "../../strpool.h"
#include "../../../../image/export.h"

#include <cglm/cglm.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#  define PATH_MAX 260
#endif

typedef enum GLTFExpAnimPlanResult {
  GLTF_EXP_ANIM_PLAN_SKIP,
  GLTF_EXP_ANIM_PLAN_OK,
  GLTF_EXP_ANIM_PLAN_ERROR
} GLTFExpAnimPlanResult;

enum {
  GLTF_EXP_BUFFER_VIEW_TARGET_ARRAY         = 34962,
  GLTF_EXP_BUFFER_VIEW_TARGET_ELEMENT_ARRAY = 34963
};

#define GLTF_EXP_PI      3.14159265358979323846f
#define GLTF_EXP_HALF_PI 1.57079632679489661923f

static inline
bool
gltf_plan_ext_name_eq(const char * __restrict name,
                      size_t                  nameLen,
                      const char * __restrict expected,
                      size_t                  expectedLen) {
  return name
         && nameLen == expectedLen
         && memcmp(name, expected, expectedLen) == 0;
}

static inline
bool
gltf_plan_skip_texture_info_core_extension(const char * __restrict name,
                                           size_t                  nameLen,
                                           void * __restrict       userdata) {
  (void)userdata;
  return gltf_plan_ext_name_eq(name,
                               nameLen,
                               _s_gltf_KHR_texture_transform,
                               _s_gltf_KHR_texture_transform_len);
}

static inline
bool
gltf_plan_skip_texture_core_extension(const char * __restrict name,
                                      size_t                  nameLen,
                                      void * __restrict       userdata) {
  (void)userdata;
  return gltf_plan_ext_name_eq(name,
                               nameLen,
                               _s_gltf_KHR_texture_basisu,
                               _s_gltf_KHR_texture_basisu_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_EXT_texture_webp,
                                  _s_gltf_EXT_texture_webp_len);
}

static inline
bool
gltf_plan_skip_primitive_core_extension(const char * __restrict name,
                                        size_t                  nameLen,
                                        void * __restrict       userdata) {
  (void)userdata;
  return gltf_plan_ext_name_eq(name,
                               nameLen,
                               _s_gltf_KHR_materials_variants,
                               _s_gltf_KHR_materials_variants_len);
}

static inline
bool
gltf_plan_skip_node_core_extension(const char * __restrict name,
                                   size_t                  nameLen,
                                   void * __restrict       userdata) {
  (void)userdata;
  return gltf_plan_ext_name_eq(name,
                               nameLen,
                               _s_gltf_KHR_lights_punctual,
                               _s_gltf_KHR_lights_punctual_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_EXT_mesh_gpu_instancing,
                                  _s_gltf_EXT_mesh_gpu_instancing_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_node_visibility,
                                  _s_gltf_KHR_node_visibility_len);
}

static inline
bool
gltf_plan_skip_material_core_extension(const char * __restrict name,
                                       size_t                  nameLen,
                                       void * __restrict       userdata) {
  (void)userdata;
  return gltf_plan_ext_name_eq(name,
                               nameLen,
                               _s_gltf_KHR_materials_unlit,
                               _s_gltf_KHR_materials_unlit_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_emissive_strength,
                                  _s_gltf_KHR_materials_emissive_strength_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_ior,
                                  _s_gltf_KHR_materials_ior_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_clearcoat,
                                  _s_gltf_KHR_materials_clearcoat_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_specular,
                                  _s_gltf_KHR_materials_specular_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_pbrSpecularGlossiness,
                                  _s_gltf_KHR_materials_pbrSpecularGlossiness_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_transmission,
                                  _s_gltf_KHR_materials_transmission_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_sheen,
                                  _s_gltf_KHR_materials_sheen_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_iridescence,
                                  _s_gltf_KHR_materials_iridescence_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_volume,
                                  _s_gltf_KHR_materials_volume_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_anisotropy,
                                  _s_gltf_KHR_materials_anisotropy_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_dispersion,
                                  _s_gltf_KHR_materials_dispersion_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_diffuse_transmission,
                                  _s_gltf_KHR_materials_diffuse_transmission_len)
         || gltf_plan_ext_name_eq(name,
                                  nameLen,
                                  _s_gltf_KHR_materials_volume_scatter,
                                  _s_gltf_KHR_materials_volume_scatter_len);
}

AK_HIDE
bool
gltf_float_positive(float val) ;

AK_HIDE
bool
gltf_float_nonnegative(float val) ;

AK_HIDE
void*
gltf_realloc_array(void *ptr, size_t count, size_t elemSize) ;

AK_HIDE
bool
gltf_next_capacity(size_t capacity, size_t initial, size_t * __restrict out) ;

AK_HIDE
bool
gltf_nodes_reserve(GLTFExpNodeTable * __restrict table, size_t capacity) ;

AK_HIDE
bool
gltf_indices_reserve(GLTFExpIndexList * __restrict list, size_t capacity) ;

AK_HIDE
bool
gltf_indices_add(GLTFExpIndexList * __restrict list, GLTFExpIndex index) ;

AK_HIDE
bool
gltf_scenes_reserve(GLTFExpSceneTable * __restrict table, size_t capacity) ;

AK_HIDE
bool
gltf_materials_reserve(GLTFExpMaterialTable * __restrict table,
                       size_t                            capacity) ;

AK_HIDE
bool
gltf_materials_add(GLTFExpMaterialTable * __restrict table,
                   AkMaterial           * __restrict material,
                   AkMeshPrimitive      * __restrict prim,
                   AkInstanceGeometry   * __restrict inst) ;

AK_HIDE
bool
gltf_ptrs_reserve(GLTFExpPtrTable * __restrict table, size_t capacity) ;

AK_HIDE
bool
gltf_strings_reserve(GLTFExpStringTable * __restrict table, size_t capacity) ;

AK_HIDE
bool
gltf_strings_add(GLTFExpStringTable * __restrict table,
                 const char         * __restrict name,
                 size_t                          nameLen) ;

AK_HIDE
bool
gltf_plan_extra_extensions(GLTFExpState            * __restrict st,
                           AkTreeNode              * __restrict extra,
                           GLTFExtraExtensionSkipFn             skip,
                           void                    * __restrict userdata) ;

AK_HIDE
bool
gltf_meshes_reserve(GLTFExpMeshTable * __restrict table, size_t capacity) ;

AK_HIDE
bool
gltf_skins_reserve(GLTFExpSkinTable * __restrict table, size_t capacity) ;

AK_HIDE
bool
gltf_skin_attrs_reserve(GLTFExpSkinAttrTable * __restrict table,
                        size_t                            capacity) ;

AK_HIDE
bool
gltf_skin_attrs_reserve_span(GLTFExpSkinAttrTable * __restrict table,
                             size_t                            count,
                             GLTFExpIndex        * __restrict offset) ;

AK_HIDE
bool
gltf_morph_attrs_reserve(GLTFExpMorphAttrTable * __restrict table,
                         size_t                             capacity) ;

AK_HIDE
bool
gltf_morph_attrs_reserve_span(GLTFExpMorphAttrTable * __restrict table,
                              size_t                             count,
                              GLTFExpIndex         * __restrict offset) ;

AK_HIDE
bool
gltf_position_attrs_reserve(GLTFExpPositionAttrTable * __restrict table,
                            size_t                                capacity) ;

AK_HIDE
GLTFExpPositionAttrOut*
gltf_position_attrs_add(GLTFExpPositionAttrTable * __restrict table,
                        AkMeshPrimitive          * __restrict prim) ;

AK_HIDE
bool
gltf_baked_attrs_reserve(GLTFExpBakedAttrTable * __restrict table,
                         size_t                             capacity) ;

AK_HIDE
GLTFExpBakedPrimAttrOut*
gltf_baked_attrs_add(GLTFExpBakedAttrTable * __restrict table,
                     AkNode                * __restrict node,
                     AkMeshPrimitive       * __restrict prim) ;

AK_HIDE
bool
gltf_anims_reserve(GLTFExpAnimTable * __restrict table, size_t capacity) ;

AK_HIDE
bool
gltf_anim_samplers_reserve(GLTFExpAnimSamplerTable * __restrict table,
                           size_t                               capacity) ;

AK_HIDE
bool
gltf_anim_channels_reserve(GLTFExpAnimChannelTable * __restrict table,
                           size_t                               capacity) ;

AK_HIDE
void*
gltf_mesh_key(AkGeometry         * __restrict geom,
              AkInstanceGeometry * __restrict inst) ;

AK_HIDE
GLTFExpIndex
gltf_mesh_index(GLTFExpMeshTable * __restrict table,
                void             * __restrict key) ;

AK_HIDE
bool
gltf_meshes_add(GLTFExpMeshTable  * __restrict table,
                void              * __restrict key,
                AkGeometry        * __restrict geom,
                AkInstanceGeometry * __restrict inst,
                AkNode            * __restrict bakeNode,
                GLTFExpIndex                    skinAttrOffset,
                uint32_t                        skinAttrCount,
                GLTFExpIndex                    morphAttrOffset,
                uint32_t                        morphAttrCount,
                uint32_t                        morphAttrPrimCount) ;

AK_HIDE
void*
gltf_skin_key(AkInstanceSkin * __restrict skinner) ;

AK_HIDE
size_t
gltf_component_type_size(AkTypeId type) ;

AK_HIDE
uint32_t
gltf_component_size_count(AkComponentSize componentSize) ;

AK_HIDE
bool
gltf_accessor_supported(AkAccessor * __restrict accessor) ;

AK_HIDE
bool
gltf_plan_image(GLTFExpState * __restrict st,
                AkImage      * __restrict image) ;

AK_HIDE
bool
gltf_plan_texture_ref(GLTFExpState * __restrict st,
                      AkTextureRef * __restrict texref) ;

AK_HIDE
AkTextureRef*
gltf_plan_texture_ref_if_exportable(GLTFExpState * __restrict st,
                                    AkTextureRef * __restrict texref) ;

AK_HIDE
bool
gltf_plan_material_input(GLTFExpState          * __restrict st,
                         const AkMaterialInput * __restrict input) ;

AK_HIDE
bool
gltf_material_input_empty(const AkMaterialInput * __restrict input) ;

AK_HIDE
bool
gltf_plan_material_feature(GLTFExpState      * __restrict st,
                           AkMaterialFeature * __restrict feature) ;

AK_HIDE
bool
gltf_plan_material_features(GLTFExpState       * __restrict st,
                            AkMaterialSurface  * __restrict surface) ;

AK_HIDE
bool
gltf_plan_material(GLTFExpState * __restrict st,
                   AkMaterial   * __restrict material,
                   AkMeshPrimitive * __restrict prim,
                   AkInstanceGeometry * __restrict inst) ;

AK_HIDE
bool
gltf_primitive_has_exportable_skin_inputs(AkMeshPrimitive * __restrict prim,
                                          AkInput         * __restrict posInput) ;

AK_HIDE
bool
gltf_mesh_needs_generated_skin_attrs(AkMesh * __restrict mesh) ;

AK_HIDE
bool
gltf_skin_attr_empty(GLTFExpSkinAttrOut * __restrict attr) ;

AK_HIDE
bool
gltf_plan_generated_skin_attrs(GLTFExpState       * __restrict st,
                               AkInstanceSkin     * __restrict skinner,
                               AkMeshPrimitive    * __restrict prim,
                               AkInput            * __restrict posInput,
                               uint32_t                         primIndex,
                               GLTFExpSkinAttrOut * __restrict attr) ;

AK_HIDE
AkNode**
gltf_skin_joints(AkInstanceSkin * __restrict skinner, size_t * __restrict count) ;

AK_HIDE
bool
gltf_skin_valid(AkInstanceSkin * __restrict skinner) ;

AK_HIDE
AkNode*
gltf_skin_unscened_joint_root(AkNode * __restrict node) ;

AK_HIDE
bool
gltf_collect_skin_joint_roots(GLTFExpState   * __restrict st,
                              AkInstanceSkin * __restrict skinner) ;

AK_HIDE
bool
gltf_plan_deferred_skin_joint_roots(GLTFExpState * __restrict st) ;

AK_HIDE
bool
gltf_skins_add(GLTFExpState    * __restrict st,
               AkInstanceSkin  * __restrict skinner,
               GLTFExpIndex    * __restrict skinIndex) ;

AK_HIDE
bool
gltf_camera_supported(AkCamera * __restrict camera) ;

AK_HIDE
bool
gltf_plan_camera(GLTFExpState * __restrict st,
                 AkCamera     * __restrict camera) ;

AK_HIDE
bool
gltf_light_supported(AkLight * __restrict light) ;

AK_HIDE
bool
gltf_plan_light(GLTFExpState * __restrict st,
                AkLight      * __restrict light) ;

AK_HIDE
bool
gltf_accessors_reserve(GLTFExpAccessorTable * __restrict table,
                       size_t                            capacity) ;

AK_HIDE
bool
gltf_accessors_add_out(GLTFExpAccessorTable * __restrict table,
                       GLTFExpAccessorOut   * __restrict out,
                       void                 * __restrict key,
                       RBTree               * __restrict map) ;

AK_HIDE
bool
gltf_accessors_add_accessor_target_flags(GLTFExpAccessorTable * __restrict table,
                                         AkAccessor           * __restrict accessor,
                                         uint32_t                          target,
                                         bool                              normalizeVec3) ;

AK_HIDE
bool
gltf_accessors_add_accessor_target(GLTFExpAccessorTable * __restrict table,
                                   AkAccessor           * __restrict accessor,
                                   uint32_t                          target) ;

AK_HIDE
bool
gltf_accessors_add_accessor(GLTFExpAccessorTable * __restrict table,
                            AkAccessor           * __restrict accessor) ;

AK_HIDE
bool
gltf_accessors_require_minmax_target(GLTFExpAccessorTable * __restrict table,
                                     AkAccessor           * __restrict accessor,
                                     uint32_t                          target) ;

AK_HIDE
bool
gltf_accessors_require_minmax(GLTFExpAccessorTable * __restrict table,
                              AkAccessor           * __restrict accessor) ;

AK_HIDE
AkTypeId
gltf_index_component_type_for_max(AkUInt maxIndex) ;

AK_HIDE
AkUInt
gltf_index_array_max(const AkIndexArray * __restrict indices) ;

AK_HIDE
bool
gltf_accessors_add_indices(GLTFExpAccessorTable * __restrict table,
                           AkMeshPrimitive      * __restrict prim) ;

AK_HIDE
bool
gltf_accessors_add_raw_target(GLTFExpAccessorTable * __restrict table,
                              const void           * __restrict key,
                              const void           * __restrict data,
                              size_t                            byteLength,
                              uint32_t                          count,
                              AkTypeId                          componentType,
                              AkComponentSize                   componentSize,
                              uint32_t                          componentCount,
                              uint32_t                          target) ;

AK_HIDE
bool
gltf_accessors_add_raw(GLTFExpAccessorTable * __restrict table,
                       const void           * __restrict key,
                       const void           * __restrict data,
                       size_t                            byteLength,
                       uint32_t                          count,
                       AkTypeId                          componentType,
                       AkComponentSize                   componentSize,
                       uint32_t                          componentCount) ;

AK_HIDE
bool
gltf_accessors_add_raw_view(GLTFExpAccessorTable * __restrict table,
                            const void           * __restrict key,
                            const void           * __restrict data,
                            size_t                            byteLength) ;

AK_HIDE
bool
gltf_accessors_add_file_view(GLTFExpAccessorTable * __restrict table,
                             const void           * __restrict key,
                             const char           * __restrict path,
                             size_t                            byteLength) ;

AK_HIDE
GLTFExpIndex
gltf_raw_accessor_index(GLTFExpAccessorTable * __restrict table,
                        const void           * __restrict key) ;

AK_HIDE
bool
gltf_raw_accessor_require_minmax(GLTFExpAccessorTable * __restrict table,
                                 const void           * __restrict key) ;

AK_HIDE
GLTFExpIndex
gltf_raw_buffer_view_index(GLTFExpAccessorTable * __restrict table,
                           const void           * __restrict key) ;

AK_HIDE
bool
gltf_index_accessor_supported(AkAccessor * __restrict accessor) ;

AK_HIDE
uint32_t
gltf_accessor_export_component_count(AkAccessor * __restrict acc) ;

AK_HIDE
bool
gltf_attr_float_vec(AkAccessor * __restrict acc, uint32_t count) ;

AK_HIDE
bool
gltf_attr_uint_norm_vec(AkAccessor * __restrict acc, uint32_t count) ;

AK_HIDE
bool
gltf_accessor_range_ok(AkAccessor * __restrict acc,
                       size_t                  fillSize,
                       size_t                  stride) ;

AK_HIDE
uint32_t
gltf_input_set_kind(AkInput * __restrict input) ;

AK_HIDE
bool
gltf_input_has_source_set_before(AkMeshPrimitive * __restrict prim,
                                 AkInput         * __restrict first,
                                 AkInput         * __restrict limit,
                                 AkInput         * __restrict posInput,
                                 uint32_t                     kind,
                                 uint32_t                     sourceSet) ;

AK_HIDE
bool
gltf_morph_input_supported(AkInput * __restrict input);

AK_HIDE
AkMorphable*
gltf_morphable_at(AkMorphTarget * __restrict target, uint32_t primIndex) ;

AK_HIDE
AkMeshPrimitive*
gltf_geometry_primitive_at(AkGeometry * __restrict geom, uint32_t primIndex) ;

AK_HIDE
AkInput*
gltf_primitive_input_by_semantic(AkMeshPrimitive * __restrict prim,
                                 AkInputSemantic               semantic) ;

AK_HIDE
uint32_t
gltf_morph_semantic_component_count(AkInputSemantic semantic) ;

AK_HIDE
bool
gltf_accessor_float_vec_supported(AkAccessor * __restrict acc,
                                  uint32_t                componentCount) ;

AK_HIDE
const float*
gltf_accessor_float_row(AkAccessor * __restrict acc, uint32_t index) ;

AK_HIDE
bool
gltf_float_close(float a, float b) ;

AK_HIDE
void
gltf_node_matrix(AkNode * __restrict node, mat4 matrix) ;

AK_HIDE
bool
gltf_node_matrix_decomposable(AkNode * __restrict node) ;

AK_HIDE
bool
gltf_node_can_bake_local_mesh(AkNode             * __restrict node,
                              AkInstanceGeometry * __restrict inst) ;

AK_HIDE
bool
gltf_position_input_needs_vec3_expansion(AkInput * __restrict input) ;

AK_HIDE
bool
gltf_plan_position_vec2(GLTFExpState    * __restrict st,
                        AkMeshPrimitive * __restrict prim,
                        AkInput         * __restrict input) ;

AK_HIDE
bool
gltf_plan_baked_position(GLTFExpState            * __restrict st,
                         AkNode                  * __restrict bakeNode,
                         AkMeshPrimitive         * __restrict prim,
                         AkInput                 * __restrict input,
                         GLTFExpBakedPrimAttrOut * __restrict attr,
                         mat4                                 matrix) ;

AK_HIDE
bool
gltf_plan_baked_normal(GLTFExpState            * __restrict st,
                       AkInput                 * __restrict input,
                       GLTFExpBakedPrimAttrOut * __restrict attr,
                       mat4                                 matrix) ;

AK_HIDE
bool
gltf_plan_baked_primitive_attrs(GLTFExpState    * __restrict st,
                                AkNode          * __restrict bakeNode,
                                AkMeshPrimitive * __restrict prim,
                                AkInput         * __restrict posInput) ;

AK_HIDE
bool
gltf_plan_normalized_morph_input(GLTFExpState       * __restrict st,
                                 AkInput            * __restrict baseInput,
                                 AkInput            * __restrict targetInput,
                                 AkInputSemantic                  semantic,
                                 GLTFExpMorphAttrOut * __restrict attr) ;

AK_HIDE
bool
gltf_plan_normalized_morph_target(GLTFExpState       * __restrict st,
                                  AkMeshPrimitive    * __restrict basePrim,
                                  AkMeshPrimitive    * __restrict targetPrim,
                                  GLTFExpMorphAttrOut * __restrict attr) ;

AK_HIDE
bool
gltf_plan_morph_target_accessors(GLTFExpState * __restrict st,
                                 AkMorph      * __restrict morph,
                                 AkMeshPrimitive * __restrict basePrim,
                                 uint32_t                  primIndex,
                                 GLTFExpMorphAttrOut * __restrict morphAttrs,
                                 uint32_t                  morphAttrPrimCount) ;

AK_HIDE
void
gltf_plan_mesh_quantization_input(GLTFExpState * __restrict st,
                                  AkInput      * __restrict input) ;

AK_HIDE
bool
gltf_plan_mesh_accessors(GLTFExpState       * __restrict st,
                         AkGeometry         * __restrict geom,
                         AkInstanceGeometry * __restrict inst,
                         AkNode             * __restrict bakeNode,
                         GLTFExpIndex       * __restrict skinAttrOffset,
                         uint32_t           * __restrict skinAttrCount,
                         GLTFExpIndex       * __restrict morphAttrOffset,
                         uint32_t           * __restrict morphAttrCount,
                         uint32_t           * __restrict morphAttrPrimCount) ;

AK_HIDE
bool
gltf_plan_mesh(GLTFExpState       * __restrict st,
               AkInstanceGeometry * __restrict inst,
               AkNode             * __restrict bakeNode,
               GLTFExpIndex       * __restrict meshIndex) ;

AK_HIDE
bool
gltf_indices_reserve_span(GLTFExpIndexList * __restrict list,
                          size_t                        count,
                          GLTFExpIndex     * __restrict offset) ;

AK_HIDE
size_t
gltf_node_direct_child_count(AkNode * __restrict node) ;

AK_HIDE
bool
gltf_plan_child_index(GLTFExpState   * __restrict st,
                      GLTFExpNodeOut * __restrict out,
                      uint32_t       * __restrict writeIndex,
                      GLTFExpIndex                 childIndex) ;

AK_HIDE
AkInstanceGeometry*
gltf_node_geometry(AkNode * __restrict node, bool * __restrict ok) ;

AK_HIDE
bool
gltf_plan_node_camera(GLTFExpState   * __restrict st,
                      AkNode         * __restrict node,
                      GLTFExpNodeOut * __restrict out) ;

AK_HIDE
bool
gltf_plan_node_light(GLTFExpState   * __restrict st,
                     AkNode         * __restrict node,
                     GLTFExpNodeOut * __restrict out) ;

AK_HIDE
bool
gltf_gpu_instancing_accessor_ok(AkAccessor * __restrict acc,
                                uint32_t                componentCount,
                                uint32_t                count) ;

AK_HIDE
bool
gltf_plan_gpu_instancing_accessor(GLTFExpState * __restrict st,
                                  AkAccessor   * __restrict acc,
                                  uint32_t                  componentCount,
                                  uint32_t                  count) ;

AK_HIDE
bool
gltf_plan_node_gpu_instancing(GLTFExpState   * __restrict st,
                              AkNode         * __restrict node,
                              GLTFExpNodeOut * __restrict out) ;

AK_HIDE
GLTFExpIndex
gltf_plan_node(GLTFExpState * __restrict st,
               AkNode       * __restrict node,
               const char   * __restrict name) ;

AK_HIDE
bool
gltf_plan_scene_root(GLTFExpState * __restrict st,
                     GLTFExpSceneOut * __restrict out,
                     GLTFExpIndex              rootIndex) ;

AK_HIDE
void
gltf_scene_skin_joint_roots_reset(GLTFExpState * __restrict st) ;

AK_HIDE
bool
gltf_scene_has_root_index(GLTFExpState    * __restrict st,
                          GLTFExpSceneOut * __restrict out,
                          GLTFExpIndex                  rootIndex) ;

AK_HIDE
bool
gltf_node_subtree_has_index(GLTFExpState * __restrict st,
                            GLTFExpIndex              nodeIndex,
                            GLTFExpIndex              targetIndex) ;

AK_HIDE
bool
gltf_scene_has_node_index(GLTFExpState    * __restrict st,
                          GLTFExpSceneOut * __restrict out,
                          GLTFExpIndex                  nodeIndex) ;

AK_HIDE
bool
gltf_plan_scene_skin_joint_roots(GLTFExpState    * __restrict st,
                                 GLTFExpSceneOut * __restrict out) ;

AK_HIDE
bool
gltf_scene_entrypoint_needed(AkNode * __restrict node) ;

AK_HIDE
bool
gltf_plan_scene(GLTFExpState * __restrict st,
                AkScene      * __restrict scene) ;

AK_HIDE
AkInput*
gltf_anim_sampler_input(AkAnimSampler    * __restrict sampler,
                        AkInputSemantic               semantic) ;

AK_HIDE
uint32_t
gltf_accessor_component_count(AkAccessor * __restrict accessor) ;

AK_HIDE
AkInterpolationType
gltf_anim_sampler_interpolation(AkAnimSampler * __restrict sampler) ;

AK_HIDE
bool
gltf_anim_sampler_packed_cubic(AkAnimSampler * __restrict sampler,
                               AkAccessor    * __restrict inputAccessor,
                               AkAccessor    * __restrict outputAccessor,
                               uint32_t                    targetValueCount) ;

AK_HIDE
bool
gltf_anim_input_accessor_supported(AkAccessor * __restrict accessor) ;

AK_HIDE
bool
gltf_anim_output_accessor_supported(GLTFExpAnimPath path,
                                    AkInterpolationType interpolation,
                                    AkAccessor * __restrict inputAccessor,
                                    AkAccessor * __restrict outputAccessor,
                                    AkResolvedTarget * __restrict target) ;

AK_HIDE
uint32_t
gltf_anim_target_value_count(GLTFExpAnimPath path,
                             AkResolvedTarget * __restrict target) ;

AK_HIDE
bool
gltf_anim_path(AkChannel        * __restrict channel,
               AkResolvedTarget * __restrict target,
               GLTFExpAnimPath  * __restrict path) ;

AK_HIDE
AkObject*
gltf_node_transform(AkNode * __restrict node, AkTypeId type) ;

AK_HIDE
bool
gltf_node_transform_chain_trs(AkNode * __restrict node) ;

AK_HIDE
bool
gltf_node_has_morpher(AkNode * __restrict node,
                      void   * __restrict target) ;

AK_HIDE
bool
gltf_anim_node_matches(GLTFExpNodeOut  * __restrict out,
                       AkResolvedTarget * __restrict target,
                       GLTFExpAnimPath               path) ;

AK_HIDE
bool
gltf_anim_add_channel(GLTFExpState   * __restrict st,
                      GLTFExpIndex                samplerIndex,
                      GLTFExpIndex                nodeIndex,
                      GLTFExpAnimPath             path) ;

AK_HIDE
GLTFExpIndex
gltf_anim_find_sampler(GLTFExpState * __restrict st,
                       GLTFExpAnimOut * __restrict anim,
                       AkAnimSampler * __restrict sampler) ;

AK_HIDE
bool
gltf_anim_add_sampler(GLTFExpState     * __restrict st,
                      GLTFExpAnimOut   * __restrict anim,
                      AkAnimSampler    * __restrict sampler,
                      AkAccessor       * __restrict inputAccessor,
                      AkAccessor       * __restrict outputAccessor,
                      GLTFExpIndex     * __restrict samplerIndex) ;

AK_HIDE
GLTFExpAnimPlanResult
gltf_plan_anim_channel(GLTFExpState   * __restrict st,
                       GLTFExpAnimOut * __restrict anim,
                       AkChannel      * __restrict channel) ;

AK_HIDE
bool
gltf_plan_animation_one(GLTFExpState * __restrict st,
                        AkAnimation  * __restrict animation) ;

AK_HIDE
bool
gltf_plan_animation_tree(GLTFExpState * __restrict st,
                         AkAnimation  * __restrict animation) ;

AK_HIDE
bool
gltf_plan_animations(GLTFExpState * __restrict st) ;

AK_HIDE
bool
gltf_plan_image_payload(GLTFExpState * __restrict st,
                        AkImage      * __restrict image,
                        GLTFExpIndex              imageIndex) ;

AK_HIDE
bool
gltf_plan_image_buffer_views(GLTFExpState * __restrict st) ;

#endif /* assetkit_gltf_exp_plan_internal_h */
