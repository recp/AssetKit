/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__assetio_states__h_
#define __libassetio__assetio_states__h_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _AIO_RENDER_STATE_BASE_
#  undef _AIO_RENDER_STATE_BASE_
#endif

typedef struct aio_render_state_s aio_render_state;

#define _AIO_RENDER_STATE_BASE_                                               \
  aio_render_state_type state_type;                                           \
  aio_render_state * next;

struct aio_render_state_s {
  _AIO_RENDER_STATE_BASE_;
};

typedef struct aio_alpha_func_s aio_alpha_func;
struct aio_alpha_func_s {
  _AIO_RENDER_STATE_BASE_;

  struct {
    aio_gl_func   val;
    const char  * param;
  } func;

  struct {
    float        val;
    const char * param;
  } val;
};

typedef struct aio_blend_func_s aio_blend_func;
struct aio_blend_func_s {
  _AIO_RENDER_STATE_BASE_;

  struct {
    aio_gl_blend val;
    const char * param;
  } src;

  struct {
    aio_gl_blend val;
    const char * param;
  } dest;
};

typedef struct aio_blend_func_separate_s aio_blend_func_separate;
struct aio_blend_func_separate_s {
  _AIO_RENDER_STATE_BASE_;

  struct {
    aio_gl_blend val;
    const char * param;
  } src_rgb;

  struct {
    aio_gl_blend val;
    const char * param;
  } dest_rgb;

  struct {
    aio_gl_blend val;
    const char * param;
  } src_alpha;

  struct {
    aio_gl_blend val;
    const char * param;
  } dest_alpha;
};

typedef struct aio_blend_equation_s aio_blend_equation;
struct aio_blend_equation_s {
  _AIO_RENDER_STATE_BASE_;

  aio_gl_blend_equation val;
  const char * param;
};

typedef struct aio_blend_equation_separate_s aio_blend_equation_separate;
struct aio_blend_equation_separate_s {
  _AIO_RENDER_STATE_BASE_;

  struct {
    aio_gl_blend_equation val;
    const char * param;
  } rgb;

  struct {
    aio_gl_blend_equation val;
    const char * param;
  } alpha;
};

typedef struct aio_color_material_s aio_color_material;
struct aio_color_material_s {
  _AIO_RENDER_STATE_BASE_;

  struct {
    aio_gl_face  val;
    const char * param;
  } face;

  struct {
    aio_gl_material val;
    const char * param;
  } mode;
};

typedef struct aio_cull_face_s aio_cull_face;
struct aio_cull_face_s {
  _AIO_RENDER_STATE_BASE_;

  aio_gl_face  val;
  const char * param;
};

typedef struct aio_depth_func_s aio_depth_func;
struct aio_depth_func_s {
  _AIO_RENDER_STATE_BASE_;

  aio_gl_func  val;
  const char * param;
};

typedef struct aio_fog_mode_s aio_fog_mode;
struct aio_fog_mode_s {
  _AIO_RENDER_STATE_BASE_;

  aio_gl_fog   val;
  const char * param;
};

typedef struct aio_fog_coord_src_s aio_fog_coord_src;
struct aio_fog_coord_src_s {
  _AIO_RENDER_STATE_BASE_;

  aio_gl_fog_coord_src val;
  const char * param;
};

typedef struct aio_front_face_s aio_front_face;
struct aio_front_face_s {
  _AIO_RENDER_STATE_BASE_;

  aio_gl_front_face val;
  const char * param;
};

typedef struct aio_light_model_color_control_s aio_light_model_color_control;
struct aio_light_model_color_control_s {
  _AIO_RENDER_STATE_BASE_;

  aio_gl_light_model_color_control val;
  const char * param;
};

typedef struct aio_logic_op_s aio_logic_op;
struct aio_logic_op_s {
  _AIO_RENDER_STATE_BASE_;

  aio_gl_logic_op val;
  const char * param;
};

typedef struct aio_polygon_mode_s aio_polygon_mode;
struct aio_polygon_mode_s {
  _AIO_RENDER_STATE_BASE_;

  struct{
    aio_gl_face  val;
    const char * param;
  } face;

  struct{
    aio_gl_polygon_mode val;
    const char * param;
  } mode;
};

typedef struct aio_shade_model_s aio_shade_model;
struct aio_shade_model_s {
  _AIO_RENDER_STATE_BASE_;

  aio_gl_shade_model val;
  const char * param;
};

typedef struct aio_stencil_func_s aio_stencil_func;
struct aio_stencil_func_s {
  _AIO_RENDER_STATE_BASE_;

  struct{
    aio_gl_func  val;
    const char * param;
  } func;

  struct{
    unsigned long val;
    const char * param;
  } ref;

  struct{
    unsigned long val;
    const char * param;
  } mask;
};

typedef struct aio_stencil_op_s aio_stencil_op;
struct aio_stencil_op_s {
  _AIO_RENDER_STATE_BASE_;

  struct{
    aio_gl_stencil_op val;
    const char * param;
  } fail;

  struct{
    aio_gl_stencil_op val;
    const char * param;
  } zfail;

  struct{
    aio_gl_stencil_op val;
    const char * param;
  } zpass;
};

typedef struct aio_stencil_func_separate_s aio_stencil_func_separate;
struct aio_stencil_func_separate_s {
  _AIO_RENDER_STATE_BASE_;

  struct{
    aio_gl_func  val;
    const char * param;
  } front;

  struct{
    aio_gl_func  val;
    const char * param;
  } back;

  struct{
    unsigned long val;
    const char * param;
  } ref;

  struct{
    unsigned long val;
    const char * param;
  } mask;
};

typedef struct aio_stencil_op_separate_s aio_stencil_op_separate;
struct aio_stencil_op_separate_s {
  _AIO_RENDER_STATE_BASE_;

  struct{
    aio_gl_face  val;
    const char * param;
  } face;

  struct{
    aio_gl_stencil_op val;
    const char * param;
  } fail;

  struct{
    aio_gl_stencil_op val;
    const char * param;
  } zfail;

  struct{
    aio_gl_stencil_op val;
    const char * param;
  } zpass;
};

typedef struct aio_stencil_mask_separate_s aio_stencil_mask_separate;
struct aio_stencil_mask_separate_s {
  _AIO_RENDER_STATE_BASE_;

  struct{
    aio_gl_face  val;
    const char * param;
  } face;

  struct{
    unsigned long val;
    const char * param;
  } mask;
};

typedef struct aio_light_enable_s aio_light_enable;
struct aio_light_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_ambient_s aio_light_ambient;
struct aio_light_ambient_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_diffuse_s aio_light_diffuse;
struct aio_light_diffuse_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_specular_s aio_light_specular;
struct aio_light_specular_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_position_s aio_light_position;
struct aio_light_position_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_constant_attenuation_s aio_light_constant_attenuation;
struct aio_light_constant_attenuation_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_linear_attenuation_s aio_light_linear_attenuation;
struct aio_light_linear_attenuation_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_quadratic_attenuation_s
  aio_light_quadratic_attenuation;
struct aio_light_quadratic_attenuation_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_spot_cutoff_s aio_light_spot_cutoff;
struct aio_light_spot_cutoff_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_spot_direction_s aio_light_spot_direction;
struct aio_light_spot_direction_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float3 val;
  const char * param;
  unsigned long index;
};

typedef struct aio_light_spot_exponent_s aio_light_spot_exponent;
struct aio_light_spot_exponent_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
  unsigned long index;
};

typedef struct aio_texture1D_s aio_texture1D;
struct aio_texture1D_s {
  _AIO_RENDER_STATE_BASE_;

  aio_sampler1D * val;
  const char * param;
  unsigned long index;
};

typedef struct aio_texture2D_s aio_texture2D;
struct aio_texture2D_s {
  _AIO_RENDER_STATE_BASE_;

  aio_sampler2D * val;
  const char * param;
  unsigned long index;
};

typedef struct aio_texture3D_s aio_texture3D;
struct aio_texture3D_s {
  _AIO_RENDER_STATE_BASE_;

  aio_sampler3D * val;
  const char * param;
  unsigned long index;
};

typedef struct aio_textureCUBE_s aio_textureCUBE;
struct aio_textureCUBE_s {
  _AIO_RENDER_STATE_BASE_;

  aio_samplerCUBE * val;
  const char * param;
  unsigned long index;
};

typedef struct aio_textureRECT_s aio_textureRECT;
struct aio_textureRECT_s {
  _AIO_RENDER_STATE_BASE_;

  aio_samplerRECT * val;
  const char * param;
  unsigned long index;
};

typedef struct aio_textureDEPTH_s aio_textureDEPTH;
struct aio_textureDEPTH_s {
  _AIO_RENDER_STATE_BASE_;

  aio_samplerDEPTH * val;
  const char * param;
  unsigned long index;
};

typedef struct aio_texture1D_enable_s aio_texture1D_enable;
struct aio_texture1D_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
  unsigned long index;
};

typedef struct aio_texture2D_enable_s aio_texture2D_enable;
struct aio_texture2D_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
  unsigned long index;
};

typedef struct aio_texture3D_enable_s aio_texture3D_enable;
struct aio_texture3D_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
  unsigned long index;
};

typedef struct aio_textureCUBE_enable_s aio_textureCUBE_enable;
struct aio_textureCUBE_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
  unsigned long index;
};

typedef struct aio_textureRECT_enable_s aio_textureRECT_enable;
struct aio_textureRECT_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
  unsigned long index;
};

typedef struct aio_textureDEPTH_enable_s aio_textureDEPTH_enable;
struct aio_textureDEPTH_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
  unsigned long index;
};

typedef struct aio_texture_env_color_s aio_texture_env_color;
struct aio_texture_env_color_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
  unsigned long index;
};

typedef struct aio_texture_env_mode_s aio_texture_env_mode;
struct aio_texture_env_mode_s {
  _AIO_RENDER_STATE_BASE_;

  const char * val;
  const char * param;
  unsigned long index;
};

typedef struct aio_clip_plane_s aio_clip_plane;
struct aio_clip_plane_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
  unsigned long index;
};

typedef struct aio_clip_plane_enable_s aio_clip_plane_enable;
struct aio_clip_plane_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
  unsigned long index;
};

typedef struct aio_blend_color_s aio_blend_color;
struct aio_blend_color_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
};

typedef struct aio_color_mask_s aio_color_mask;
struct aio_color_mask_s {
  _AIO_RENDER_STATE_BASE_;

  aio_bool4 val;
  const char * param;
};

typedef struct aio_depth_bounds_s aio_depth_bounds;
struct aio_depth_bounds_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float2 val;
  const char * param;
};

typedef struct aio_depth_mask_s aio_depth_mask;
struct aio_depth_mask_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_depth_range_s aio_depth_range;
struct aio_depth_range_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float2 val;
  const char * param;
};

typedef struct aio_fog_density_s aio_fog_density;
struct aio_fog_density_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
};

typedef struct aio_fog_start_s aio_fog_start;
struct aio_fog_start_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
};

typedef struct aio_fog_end_s aio_fog_end;
struct aio_fog_end_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
};

typedef struct aio_fog_color_s aio_fog_color;
struct aio_fog_color_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
};

typedef struct aio_light_model_ambient_s aio_light_model_ambient;
struct aio_light_model_ambient_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
};

typedef struct aio_lighting_enable_s aio_lighting_enable;
struct aio_lighting_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_line_stipple_s aio_line_stipple;
struct aio_line_stipple_s {
  _AIO_RENDER_STATE_BASE_;

  aio_int2 val;
  const char * param;
};

typedef struct aio_line_width_s aio_line_width;
struct aio_line_width_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
};

typedef struct aio_material_ambient_s aio_material_ambient;
struct aio_material_ambient_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
};

typedef struct aio_material_diffuse_s aio_material_diffuse;
struct aio_material_diffuse_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
};

typedef struct aio_material_emission_s aio_material_emission;
struct aio_material_emission_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
};

typedef struct aio_material_shininess_s aio_material_shininess;
struct aio_material_shininess_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
};

typedef struct aio_material_specular_s aio_material_specular;
struct aio_material_specular_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4 val;
  const char * param;
};

typedef struct aio_model_view_matrix_s aio_model_view_matrix;
struct aio_model_view_matrix_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4x4 val;
  const char * param;
};

typedef struct aio_point_distance_attenuation_s aio_point_distance_attenuation;
struct aio_point_distance_attenuation_s {
  _AIO_RENDER_STATE_BASE_;
  
  aio_float3 val;
  const char * param;
};

typedef struct aio_point_fade_threshold_size_s aio_point_fade_threshold_size;
struct aio_point_fade_threshold_size_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
};

typedef struct aio_point_size_s aio_point_size;
struct aio_point_size_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
};

typedef struct aio_point_size_min_s aio_point_size_min;
struct aio_point_size_min_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
};

typedef struct aio_point_size_max_s aio_point_size_max;
struct aio_point_size_max_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float val;
  const char * param;
};

typedef struct aio_polygon_offset_s aio_polygon_offset;
struct aio_polygon_offset_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float2 val;
  const char * param;
};

typedef struct aio_projection_matrix_s aio_projection_matrix;
struct aio_projection_matrix_s {
  _AIO_RENDER_STATE_BASE_;

  aio_float4x4 val;
  const char * param;
};

typedef struct aio_scissor_s aio_scissor;
struct aio_scissor_s {
  _AIO_RENDER_STATE_BASE_;

  aio_int4 val;
  const char * param;
};

typedef struct aio_stencil_mask_s aio_stencil_mask;
struct aio_stencil_mask_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_alpha_test_enable_s aio_alpha_test_enable;
struct aio_alpha_test_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_blend_enable_s aio_blend_enable;
struct aio_blend_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_color_logic_op_enable_s aio_color_logic_op_enable;
struct aio_color_logic_op_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_color_material_enable_s aio_color_material_enable;
struct aio_color_material_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_cull_face_enable_s aio_cull_face_enable;
struct aio_cull_face_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_depth_bounds_enable_s aio_depth_bounds_enable;
struct aio_depth_bounds_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_depth_clamp_enable_s aio_depth_clamp_enable;
struct aio_depth_clamp_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_depth_test_enable_s aio_depth_test_enable;
struct aio_depth_test_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_dither_enable_s aio_dither_enable;
struct aio_dither_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_fog_enable_s aio_fog_enable;
struct aio_fog_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_light_model_local_viewer_enable_s
  aio_light_model_local_viewer_enable;
struct aio_light_model_local_viewer_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_light_model_two_side_enable_s
  aio_light_model_two_side_enable;
struct aio_light_model_two_side_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_line_smooth_enable_s aio_line_smooth_enable;
struct aio_line_smooth_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_line_stipple_enable_s aio_line_stipple_enable;
struct aio_line_stipple_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_logic_op_enable_s aio_logic_op_enable;
struct aio_logic_op_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_multisample_enable_s aio_multisample_enable;
struct aio_multisample_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_normalize_enable_s aio_normalize_enable;
struct aio_normalize_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_point_smooth_enable_s aio_point_smooth_enable;
struct aio_point_smooth_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_polygon_offset_fill_enable_s
  aio_polygon_offset_fill_enable;
struct aio_polygon_offset_fill_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_polygon_offset_line_enable_s
  aio_polygon_offset_line_enable;
struct aio_polygon_offset_line_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_polygon_offset_point_enable_s
  aio_polygon_offset_point_enable;
struct aio_polygon_offset_point_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_polygon_smooth_enable_s aio_polygon_smooth_enable;
struct aio_polygon_smooth_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_polygon_stipple_enable_s aio_polygon_stipple_enable;
struct aio_polygon_stipple_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_rescale_normal_enable_s aio_rescale_normal_enable;
struct aio_rescale_normal_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_sample_alpha_to_coverage_enable_s
  aio_sample_alpha_to_coverage_enable;
struct aio_sample_alpha_to_coverage_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_sample_alpha_to_one_enable_s
  aio_sample_alpha_to_one_enable;
struct aio_sample_alpha_to_one_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_sample_coverage_enable_s aio_sample_coverage_enable;
struct aio_sample_coverage_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_scissor_test_enable_s aio_scissor_test_enable;
struct aio_scissor_test_enable_s {
  _AIO_RENDER_STATE_BASE_;

  unsigned long val;
  const char * param;
};

typedef struct aio_stencil_test_enable_s aio_stencil_test_enable;
struct aio_stencil_test_enable_s {
  _AIO_RENDER_STATE_BASE_;
  
  unsigned long val;
  const char * param;
};

#undef _AIO_RENDER_STATE_BASE_

#ifdef __cplusplus
}
#endif
#endif /* __libassetio__assetio_states__h_ */
