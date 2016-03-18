/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__assetkit_states__h_
#define __libassetkit__assetkit_states__h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ak_render_state_s ak_render_state;
struct ak_render_state_s {
  ak_render_state_type   state_type;
  ak_render_state      * next;
};

typedef struct ak_alpha_func_s ak_alpha_func;
struct ak_alpha_func_s {
  ak_render_state base;

  struct {
    ak_gl_func   val;
    const char  * param;
  } func;

  struct {
    float        val;
    const char * param;
  } val;
};

typedef struct ak_blend_func_s ak_blend_func;
struct ak_blend_func_s {
  ak_render_state base;

  struct {
    ak_gl_blend val;
    const char * param;
  } src;

  struct {
    ak_gl_blend val;
    const char * param;
  } dest;
};

typedef struct ak_blend_func_separate_s ak_blend_func_separate;
struct ak_blend_func_separate_s {
  ak_render_state base;

  struct {
    ak_gl_blend val;
    const char * param;
  } src_rgb;

  struct {
    ak_gl_blend val;
    const char * param;
  } dest_rgb;

  struct {
    ak_gl_blend val;
    const char * param;
  } src_alpha;

  struct {
    ak_gl_blend val;
    const char * param;
  } dest_alpha;
};

typedef struct ak_blend_equation_separate_s ak_blend_equation_separate;
struct ak_blend_equation_separate_s {
  ak_render_state base;

  struct {
    ak_gl_blend_equation val;
    const char * param;
  } rgb;

  struct {
    ak_gl_blend_equation val;
    const char * param;
  } alpha;
};

typedef struct ak_color_material_s ak_color_material;
struct ak_color_material_s {
  ak_render_state base;

  struct {
    ak_gl_face  val;
    const char * param;
  } face;

  struct {
    ak_gl_material val;
    const char * param;
  } mode;
};

typedef struct ak_polygon_mode_s ak_polygon_mode;
struct ak_polygon_mode_s {
  ak_render_state base;

  struct{
    ak_gl_face  val;
    const char * param;
  } face;

  struct{
    ak_gl_polygon_mode val;
    const char * param;
  } mode;
};

typedef struct ak_stencil_func_s ak_stencil_func;
struct ak_stencil_func_s {
  ak_render_state base;

  struct{
    ak_gl_func  val;
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

typedef struct ak_stencil_op_s ak_stencil_op;
struct ak_stencil_op_s {
  ak_render_state base;

  struct{
    ak_gl_stencil_op val;
    const char * param;
  } fail;

  struct{
    ak_gl_stencil_op val;
    const char * param;
  } zfail;

  struct{
    ak_gl_stencil_op val;
    const char * param;
  } zpass;
};

typedef struct ak_stencil_func_separate_s ak_stencil_func_separate;
struct ak_stencil_func_separate_s {
  ak_render_state base;

  struct{
    ak_gl_func  val;
    const char * param;
  } front;

  struct{
    ak_gl_func  val;
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

typedef struct ak_stencil_op_separate_s ak_stencil_op_separate;
struct ak_stencil_op_separate_s {
  ak_render_state base;

  struct{
    ak_gl_face  val;
    const char * param;
  } face;

  struct{
    ak_gl_stencil_op val;
    const char * param;
  } fail;

  struct{
    ak_gl_stencil_op val;
    const char * param;
  } zfail;

  struct{
    ak_gl_stencil_op val;
    const char * param;
  } zpass;
};

typedef struct ak_stencil_mask_separate_s ak_stencil_mask_separate;
struct ak_stencil_mask_separate_s {
  ak_render_state base;

  struct{
    ak_gl_face  val;
    const char * param;
  } face;

  struct{
    unsigned long val;
    const char * param;
  } mask;
};

#define _ak_DEF_STATE_T1(P, T)                                               \
  typedef struct ak_ ## P ## _s ak_ ## P;                                   \
  struct ak_ ## P ## _s {                                                    \
    ak_render_state base;                                                    \
    T val;                                                                    \
    const char * param;                                                       \
  };

#define _ak_DEF_STATE_T2(P, T)                                               \
  typedef struct ak_ ## P ## _s ak_ ## P;                                   \
  struct ak_ ## P ## _s {                                                    \
    ak_render_state base;                                                    \
    T val;                                                                    \
    const char * param;                                                       \
    unsigned long index;                                                      \
  };

_ak_DEF_STATE_T1(state_t_bool4,              ak_bool4)
_ak_DEF_STATE_T1(state_t_int2,               ak_int2)
_ak_DEF_STATE_T1(state_t_int4,               ak_int4)
_ak_DEF_STATE_T1(state_t_ul,                 unsigned long )
_ak_DEF_STATE_T2(state_t_ul_i,               unsigned long )
_ak_DEF_STATE_T1(state_t_float,              ak_float)
_ak_DEF_STATE_T2(state_t_float_i,            ak_float)
_ak_DEF_STATE_T1(state_t_float2,             ak_float2)
_ak_DEF_STATE_T1(state_t_float3,             ak_float3)
_ak_DEF_STATE_T2(state_t_float3_i,           ak_float3)
_ak_DEF_STATE_T1(state_t_float4,             ak_float4)
_ak_DEF_STATE_T2(state_t_float4_i,           ak_float4)
_ak_DEF_STATE_T1(state_t_float4x4,           ak_float4x4)
_ak_DEF_STATE_T2(state_t_sampler,            ak_fx_sampler_common *)
_ak_DEF_STATE_T2(state_t_str,                const char *)

#undef _ak_DEF_STATE_T
#undef _ak_DEF_STATE_T2

typedef ak_state_t_str       ak_texture_env_mode;
typedef ak_state_t_sampler   ak_texture1D;
typedef ak_state_t_sampler   ak_texture2D;
typedef ak_state_t_sampler   ak_texture3D;
typedef ak_state_t_sampler   ak_textureCUBE;
typedef ak_state_t_sampler   ak_textureRECT;
typedef ak_state_t_sampler   ak_textureDEPTH;

typedef ak_state_t_int2      ak_depth_line_stipple;
typedef ak_state_t_int4      ak_depth_scissor;
typedef ak_state_t_ul        ak_depth_mask;
typedef ak_state_t_bool4     ak_color_mask;
typedef ak_state_t_ul        ak_blend_equation;
typedef ak_state_t_ul        ak_cull_face;
typedef ak_state_t_ul        ak_depth_func;
typedef ak_state_t_ul        ak_fog_mode;
typedef ak_state_t_ul        ak_fog_coord_src;
typedef ak_state_t_ul        ak_front_face;
typedef ak_state_t_ul        ak_light_model_color_control;
typedef ak_state_t_ul        ak_logic_op;
typedef ak_state_t_ul        ak_shade_model;
typedef ak_state_t_ul        ak_stencil_mask;
typedef ak_state_t_ul        ak_state_t_enable;
typedef ak_state_t_ul_i      ak_state_t_enable2;
typedef ak_state_t_float     ak_fog_density;
typedef ak_state_t_float     ak_fog_start;
typedef ak_state_t_float     ak_fog_end;
typedef ak_state_t_float     ak_line_width;
typedef ak_state_t_float     ak_material_shininess;
typedef ak_state_t_float     ak_point_fade_threshold_size;
typedef ak_state_t_float     ak_point_size;
typedef ak_state_t_float     ak_point_size_min;
typedef ak_state_t_float     ak_point_size_max;
typedef ak_state_t_float_i   ak_light_linear_attenuation;
typedef ak_state_t_float_i   ak_light_quadratic_attenuation;
typedef ak_state_t_float_i   ak_light_spot_cutoff;
typedef ak_state_t_float_i   ak_light_spot_exponent;
typedef ak_state_t_float2    ak_polygon_offset;
typedef ak_state_t_float2    ak_depth_range;
typedef ak_state_t_float2    ak_depth_bounds;
typedef ak_state_t_float3    ak_point_distance_attenuation;
typedef ak_state_t_float3_i  ak_light_spot_direction;
typedef ak_state_t_float4    ak_blend_color;
typedef ak_state_t_float4    ak_fog_color;
typedef ak_state_t_float4    ak_light_model_ambient;
typedef ak_state_t_float4    ak_material_ambient;
typedef ak_state_t_float4    ak_material_diffuse;
typedef ak_state_t_float4    ak_material_emission;
typedef ak_state_t_float4    ak_material_specular;
typedef ak_state_t_float4_i  ak_texture_env_color;
typedef ak_state_t_float4_i  ak_clip_plane;
typedef ak_state_t_float4_i  ak_light_ambient;
typedef ak_state_t_float4_i  ak_light_diffuse;
typedef ak_state_t_float4_i  ak_light_specular;
typedef ak_state_t_float4_i  ak_light_position;
typedef ak_state_t_float4_i  ak_light_constant_attenuation;
typedef ak_state_t_float4x4  ak_model_view_matrix;
typedef ak_state_t_float4x4  ak_projection_matrix;
typedef ak_state_t_enable    ak_lighting_enable;
typedef ak_state_t_enable    ak_alpha_test_enable;
typedef ak_state_t_enable    ak_blend_enable;
typedef ak_state_t_enable    ak_color_logic_op_enable;
typedef ak_state_t_enable    ak_color_material_enable;
typedef ak_state_t_enable    ak_cull_face_enable;
typedef ak_state_t_enable    ak_depth_bounds_enable;
typedef ak_state_t_enable    ak_depth_clamp_enable;
typedef ak_state_t_enable    ak_depth_test_enable;
typedef ak_state_t_enable    ak_dither_enable;
typedef ak_state_t_enable    ak_fog_enable;
typedef ak_state_t_enable    ak_light_model_local_viewer_enable;
typedef ak_state_t_enable    ak_light_model_two_side_enable;
typedef ak_state_t_enable    ak_line_smooth_enable;
typedef ak_state_t_enable    ak_line_stipple_enable;
typedef ak_state_t_enable    ak_logic_op_enable;
typedef ak_state_t_enable    ak_multisample_enable;
typedef ak_state_t_enable    ak_normalize_enable;
typedef ak_state_t_enable    ak_point_smooth_enable;
typedef ak_state_t_enable    ak_polygon_offset_fill_enable;
typedef ak_state_t_enable    ak_polygon_offset_line_enable;
typedef ak_state_t_enable    ak_polygon_offset_point_enable;
typedef ak_state_t_enable    ak_polygon_smooth_enable;
typedef ak_state_t_enable    ak_polygon_stipple_enable;
typedef ak_state_t_enable    ak_rescale_normal_enable;
typedef ak_state_t_enable    ak_sample_alpha_to_coverage_enable;
typedef ak_state_t_enable    ak_sample_alpha_to_one_enable;
typedef ak_state_t_enable    ak_sample_coverage_enable;
typedef ak_state_t_enable    ak_scissor_test_enable;
typedef ak_state_t_enable    ak_stencil_test_enable;
typedef ak_state_t_enable2   ak_texture1D_enable;
typedef ak_state_t_enable2   ak_texture2D_enable;
typedef ak_state_t_enable2   ak_texture3D_enable;
typedef ak_state_t_enable2   ak_textureCUBE_enable;
typedef ak_state_t_enable2   ak_textureRECT_enable;
typedef ak_state_t_enable2   ak_textureDEPTH_enable;
typedef ak_state_t_enable2   ak_light_enable;
typedef ak_state_t_enable2   ak_clip_plane_enable;

#ifdef __cplusplus
}
#endif
#endif /* __libassetkit__assetkit_states__h_ */
