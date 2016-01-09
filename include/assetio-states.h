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

typedef struct aio_render_state_s aio_render_state;
struct aio_render_state_s {
  aio_render_state_type   state_type;
  aio_render_state      * next;
};

typedef struct aio_alpha_func_s aio_alpha_func;
struct aio_alpha_func_s {
  aio_render_state base;

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
  aio_render_state base;

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
  aio_render_state base;

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

typedef struct aio_blend_equation_separate_s aio_blend_equation_separate;
struct aio_blend_equation_separate_s {
  aio_render_state base;

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
  aio_render_state base;

  struct {
    aio_gl_face  val;
    const char * param;
  } face;

  struct {
    aio_gl_material val;
    const char * param;
  } mode;
};

typedef struct aio_polygon_mode_s aio_polygon_mode;
struct aio_polygon_mode_s {
  aio_render_state base;

  struct{
    aio_gl_face  val;
    const char * param;
  } face;

  struct{
    aio_gl_polygon_mode val;
    const char * param;
  } mode;
};

typedef struct aio_stencil_func_s aio_stencil_func;
struct aio_stencil_func_s {
  aio_render_state base;

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
  aio_render_state base;

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
  aio_render_state base;

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
  aio_render_state base;

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
  aio_render_state base;

  struct{
    aio_gl_face  val;
    const char * param;
  } face;

  struct{
    unsigned long val;
    const char * param;
  } mask;
};

#define _AIO_DEF_STATE_T1(P, T)                                               \
  typedef struct aio_ ## P ## _s aio_ ## P;                                   \
  struct aio_ ## P ## _s {                                                    \
    aio_render_state base;                                                    \
    T val;                                                                    \
    const char * param;                                                       \
  };

#define _AIO_DEF_STATE_T2(P, T)                                               \
  typedef struct aio_ ## P ## _s aio_ ## P;                                   \
  struct aio_ ## P ## _s {                                                    \
    aio_render_state base;                                                    \
    T val;                                                                    \
    const char * param;                                                       \
    unsigned long index;                                                      \
  };

_AIO_DEF_STATE_T1(state_t_bool4,              aio_bool4)
_AIO_DEF_STATE_T1(state_t_int2,               aio_int2)
_AIO_DEF_STATE_T1(state_t_int4,               aio_int4)
_AIO_DEF_STATE_T1(state_t_ul,                 unsigned long )
_AIO_DEF_STATE_T2(state_t_ul_i,               unsigned long )
_AIO_DEF_STATE_T1(state_t_float,              aio_float)
_AIO_DEF_STATE_T2(state_t_float_i,            aio_float)
_AIO_DEF_STATE_T1(state_t_float2,             aio_float2)
_AIO_DEF_STATE_T1(state_t_float3,             aio_float3)
_AIO_DEF_STATE_T2(state_t_float3_i,           aio_float3)
_AIO_DEF_STATE_T1(state_t_float4,             aio_float4)
_AIO_DEF_STATE_T2(state_t_float4_i,           aio_float4)
_AIO_DEF_STATE_T1(state_t_float4x4,           aio_float4x4)
_AIO_DEF_STATE_T2(state_t_sampler,            aio_fx_sampler_common *)
_AIO_DEF_STATE_T2(state_t_str,                const char *)

#undef _AIO_DEF_STATE_T
#undef _AIO_DEF_STATE_T2

typedef aio_state_t_str       aio_texture_env_mode;
typedef aio_state_t_sampler   aio_texture1D;
typedef aio_state_t_sampler   aio_texture2D;
typedef aio_state_t_sampler   aio_texture3D;
typedef aio_state_t_sampler   aio_textureCUBE;
typedef aio_state_t_sampler   aio_textureRECT;
typedef aio_state_t_sampler   aio_textureDEPTH;

typedef aio_state_t_int2      aio_depth_line_stipple;
typedef aio_state_t_int4      aio_depth_scissor;
typedef aio_state_t_ul        aio_depth_mask;
typedef aio_state_t_bool4     aio_color_mask;
typedef aio_state_t_ul        aio_blend_equation;
typedef aio_state_t_ul        aio_cull_face;
typedef aio_state_t_ul        aio_depth_func;
typedef aio_state_t_ul        aio_fog_mode;
typedef aio_state_t_ul        aio_fog_coord_src;
typedef aio_state_t_ul        aio_front_face;
typedef aio_state_t_ul        aio_light_model_color_control;
typedef aio_state_t_ul        aio_logic_op;
typedef aio_state_t_ul        aio_shade_model;
typedef aio_state_t_ul        aio_stencil_mask;
typedef aio_state_t_ul        aio_state_t_enable;
typedef aio_state_t_ul_i      aio_state_t_enable2;
typedef aio_state_t_float     aio_fog_density;
typedef aio_state_t_float     aio_fog_start;
typedef aio_state_t_float     aio_fog_end;
typedef aio_state_t_float     aio_line_width;
typedef aio_state_t_float     aio_material_shininess;
typedef aio_state_t_float     aio_point_fade_threshold_size;
typedef aio_state_t_float     aio_point_size;
typedef aio_state_t_float     aio_point_size_min;
typedef aio_state_t_float     aio_point_size_max;
typedef aio_state_t_float_i   aio_light_linear_attenuation;
typedef aio_state_t_float_i   aio_light_quadratic_attenuation;
typedef aio_state_t_float_i   aio_light_spot_cutoff;
typedef aio_state_t_float_i   aio_light_spot_exponent;
typedef aio_state_t_float2    aio_polygon_offset;
typedef aio_state_t_float2    aio_depth_range;
typedef aio_state_t_float2    aio_depth_bounds;
typedef aio_state_t_float3    aio_point_distance_attenuation;
typedef aio_state_t_float3_i  aio_light_spot_direction;
typedef aio_state_t_float4    aio_blend_color;
typedef aio_state_t_float4    aio_fog_color;
typedef aio_state_t_float4    aio_light_model_ambient;
typedef aio_state_t_float4    aio_material_ambient;
typedef aio_state_t_float4    aio_material_diffuse;
typedef aio_state_t_float4    aio_material_emission;
typedef aio_state_t_float4    aio_material_specular;
typedef aio_state_t_float4_i  aio_texture_env_color;
typedef aio_state_t_float4_i  aio_clip_plane;
typedef aio_state_t_float4_i  aio_light_ambient;
typedef aio_state_t_float4_i  aio_light_diffuse;
typedef aio_state_t_float4_i  aio_light_specular;
typedef aio_state_t_float4_i  aio_light_position;
typedef aio_state_t_float4_i  aio_light_constant_attenuation;
typedef aio_state_t_float4x4  aio_model_view_matrix;
typedef aio_state_t_float4x4  aio_projection_matrix;
typedef aio_state_t_enable    aio_lighting_enable;
typedef aio_state_t_enable    aio_alpha_test_enable;
typedef aio_state_t_enable    aio_blend_enable;
typedef aio_state_t_enable    aio_color_logic_op_enable;
typedef aio_state_t_enable    aio_color_material_enable;
typedef aio_state_t_enable    aio_cull_face_enable;
typedef aio_state_t_enable    aio_depth_bounds_enable;
typedef aio_state_t_enable    aio_depth_clamp_enable;
typedef aio_state_t_enable    aio_depth_test_enable;
typedef aio_state_t_enable    aio_dither_enable;
typedef aio_state_t_enable    aio_fog_enable;
typedef aio_state_t_enable    aio_light_model_local_viewer_enable;
typedef aio_state_t_enable    aio_light_model_two_side_enable;
typedef aio_state_t_enable    aio_line_smooth_enable;
typedef aio_state_t_enable    aio_line_stipple_enable;
typedef aio_state_t_enable    aio_logic_op_enable;
typedef aio_state_t_enable    aio_multisample_enable;
typedef aio_state_t_enable    aio_normalize_enable;
typedef aio_state_t_enable    aio_point_smooth_enable;
typedef aio_state_t_enable    aio_polygon_offset_fill_enable;
typedef aio_state_t_enable    aio_polygon_offset_line_enable;
typedef aio_state_t_enable    aio_polygon_offset_point_enable;
typedef aio_state_t_enable    aio_polygon_smooth_enable;
typedef aio_state_t_enable    aio_polygon_stipple_enable;
typedef aio_state_t_enable    aio_rescale_normal_enable;
typedef aio_state_t_enable    aio_sample_alpha_to_coverage_enable;
typedef aio_state_t_enable    aio_sample_alpha_to_one_enable;
typedef aio_state_t_enable    aio_sample_coverage_enable;
typedef aio_state_t_enable    aio_scissor_test_enable;
typedef aio_state_t_enable    aio_stencil_test_enable;
typedef aio_state_t_enable2   aio_texture1D_enable;
typedef aio_state_t_enable2   aio_texture2D_enable;
typedef aio_state_t_enable2   aio_texture3D_enable;
typedef aio_state_t_enable2   aio_textureCUBE_enable;
typedef aio_state_t_enable2   aio_textureRECT_enable;
typedef aio_state_t_enable2   aio_textureDEPTH_enable;
typedef aio_state_t_enable2   aio_light_enable;
typedef aio_state_t_enable2   aio_clip_plane_enable;

#ifdef __cplusplus
}
#endif
#endif /* __libassetio__assetio_states__h_ */
