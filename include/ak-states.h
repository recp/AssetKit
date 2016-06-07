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

typedef enum AkGlFunc {
  AK_GL_FUNC_NEVER    = 1,
  AK_GL_FUNC_LESS     = 2,
  AK_GL_FUNC_EQUAL    = 3,
  AK_GL_FUNC_LEQUAL   = 4,
  AK_GL_FUNC_GREATER  = 5,
  AK_GL_FUNC_NOTEQUAL = 6,
  AK_GL_FUNC_GEQUAL   = 7,
  AK_GL_FUNC_ALWAYS   = 8
} AkGlFunc;

typedef enum AkGlBlend {
  AK_GL_BLEND_ZERO                     = 1,
  AK_GL_BLEND_ONE                      = 2,
  AK_GL_BLEND_SRC_COLOR                = 3,
  AK_GL_BLEND_ONE_MINUS_SRC_COLOR      = 4,
  AK_GL_BLEND_DEST_COLOR               = 5,
  AK_GL_BLEND_ONE_MINUS_DEST_COLOR     = 6,
  AK_GL_BLEND_SRC_ALPHA                = 7,
  AK_GL_BLEND_ONE_MINUS_SRC_ALPHA      = 8,
  AK_GL_BLEND_DST_ALPHA                = 9,
  AK_GL_BLEND_ONE_MINUS_DST_ALPHA      = 10,
  AK_GL_BLEND_CONSTANT_COLOR           = 11,
  AK_GL_BLEND_ONE_MINUS_CONSTANT_COLOR = 12,
  AK_GL_BLEND_CONSTANT_ALPHA           = 13,
  AK_GL_BLEND_ONE_MINUS_CONSTANT_ALPHA = 14,
  AK_GL_BLEND_SRC_ALPHA_SATURATE       = 15
} AkGlBlend;

typedef enum AkGlBlendEquation {
  AK_GL_BLEND_EQUATION_FUNC_ADD              = 1,
  AK_GL_BLEND_EQUATION_FUNC_SUBTRACT         = 2,
  AK_GL_BLEND_EQUATION_FUNC_REVERSE_SUBTRACT = 3,
  AK_GL_BLEND_EQUATION_MIN                   = 4,
  AK_GL_BLEND_EQUATION_MAX                   = 5
} AkGlBlendEquation;

typedef enum AkGlFace {
  AK_GL_FACE_FRONT          = 1,
  AK_GL_FACE_BACK           = 2,
  AK_GL_FACE_FRONT_AND_BACK = 3
} AkGlFace;

typedef enum AkGlFrontFace {
  AK_GL_FRONT_FACE_CW   = 1,
  AK_GL_FRONT_FACE_CCW  = 2
} AkGlFrontFace;

typedef enum AkGlMaterialType {
  AK_GL_MATERIAL_TYPE_EMISSION            = 1,
  AK_GL_MATERIAL_TYPE_AMBIENT             = 2,
  AK_GL_MATERIAL_TYPE_DIFFUSE             = 3,
  AK_GL_MATERIAL_TYPE_SPECULAR            = 4,
  AK_GL_MATERIAL_TYPE_AMBIENT_AND_DIFFUSE = 5
} AkGlMaterialType;

typedef enum AkGlFog {
  AK_GL_FOG_LINEAR = 1,
  AK_GL_FOG_EXP    = 2,
  AK_GL_FOG_EXP2   = 3
} AkGlFog;

typedef enum AkGlFogCoordSrc {
  AK_GL_FOG_COORD_SRC_FOG_COORDINATE = 1,
  AK_GL_FOG_COORD_SRC_FRAGMENT_DEPTH = 2
} AkGlFogCoordSrc;

typedef enum AkGlLightModelColorControl {
  AK_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR            = 1,
  AK_GL_LIGHT_MODEL_COLOR_CONTROL_SEPARATE_SPECULAR_COLOR = 2
} AkGlLightModelColorCtl;

typedef enum AkGlLogicOp {
  AK_GL_LOGIC_OP_CLEAR         = 1,
  AK_GL_LOGIC_OP_AND           = 2,
  AK_GL_LOGIC_OP_AND_REVERSE   = 3,
  AK_GL_LOGIC_OP_COPY          = 4,
  AK_GL_LOGIC_OP_AND_INVERTED  = 5,
  AK_GL_LOGIC_OP_NOOP          = 6,
  AK_GL_LOGIC_OP_XOR           = 7,
  AK_GL_LOGIC_OP_OR            = 8,
  AK_GL_LOGIC_OP_NOR           = 9,
  AK_GL_LOGIC_OP_EQUIV         = 10,
  AK_GL_LOGIC_OP_INVERT        = 11,
  AK_GL_LOGIC_OP_OR_REVERSE    = 12,
  AK_GL_LOGIC_OP_COPY_INVERTED = 13,
  AK_GL_LOGIC_OP_NAND          = 14,
  AK_GL_LOGIC_OP_SET           = 15
} AkGlLogicOp;

typedef enum AkGlPolygonMode {
  AK_GL_POLYGON_MODE_POINT = 1,
  AK_GL_POLYGON_MODE_LINE  = 2,
  AK_GL_POLYGON_MODE_FILL  = 3
} AkGlPolygonMode;

typedef enum AkGlShadeModel {
  AK_GL_SHADE_MODEL_FLAT   = 1,
  AK_GL_SHADE_MODEL_SMOOTH = 2
} AkGlShadeModel;

typedef enum AkGlStencilOp {
  AK_GL_STENCIL_OP_KEEP      = 1,
  AK_GL_STENCIL_OP_ZERO      = 2,
  AK_GL_STENCIL_OP_REPLACE   = 3,
  AK_GL_STENCIL_OP_INCR      = 4,
  AK_GL_STENCIL_OP_DECR      = 5,
  AK_GL_STENCIL_OP_INVERT    = 6,
  AK_GL_STENCIL_OP_INCR_WRAP = 7,
  AK_GL_STENCIL_OP_DECR_WRAP = 8
} AkGlStencilOp;

typedef enum AkRenderStateType {
  AK_RENDER_STATE_ALPHA_FUNC                      = 1,
  AK_RENDER_STATE_BLEND_FUNC                      = 2,
  AK_RENDER_STATE_BLEND_FUNC_SEPARATE             = 3,
  AK_RENDER_STATE_BLEND_EQUATION                  = 4,
  AK_RENDER_STATE_BLEND_EQUATION_SEPARATE         = 5,
  AK_RENDER_STATE_COLOR_MATERIAL                  = 6,
  AK_RENDER_STATE_CULL_FACE                       = 7,
  AK_RENDER_STATE_DEPTH_FUNC                      = 8,
  AK_RENDER_STATE_FOG_MODE                        = 9,
  AK_RENDER_STATE_FOG_COORD_SRC                   = 10,
  AK_RENDER_STATE_FRONT_FACE                      = 11,
  AK_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL       = 12,
  AK_RENDER_STATE_LOGIC_OP                        = 13,
  AK_RENDER_STATE_POLYGON_MODE                    = 14,
  AK_RENDER_STATE_SHADE_MODEL                     = 15,
  AK_RENDER_STATE_STENCIL_FUNC                    = 16,
  AK_RENDER_STATE_STENCIL_OP                      = 17,
  AK_RENDER_STATE_STENCIL_FUNC_SEPARATE           = 18,
  AK_RENDER_STATE_STENCIL_OP_SEPARATE             = 19,
  AK_RENDER_STATE_STENCIL_MASK_SEPARATE           = 20,
  AK_RENDER_STATE_LIGHT_ENABLE                    = 21,
  AK_RENDER_STATE_LIGHT_AMBIENT                   = 22,
  AK_RENDER_STATE_LIGHT_DIFFUSE                   = 23,
  AK_RENDER_STATE_LIGHT_SPECULAR                  = 24,
  AK_RENDER_STATE_LIGHT_POSITION                  = 25,
  AK_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION      = 26,
  AK_RENDER_STATE_LIGHT_LINEAR_ATTENUATION        = 27,
  AK_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION       = 28,
  AK_RENDER_STATE_LIGHT_SPOT_CUTOFF               = 29,
  AK_RENDER_STATE_LIGHT_SPOT_DIRECTION            = 30,
  AK_RENDER_STATE_LIGHT_SPOT_EXPONENT             = 31,
  AK_RENDER_STATE_TEXTURE1D                       = 32,
  AK_RENDER_STATE_TEXTURE2D                       = 33,
  AK_RENDER_STATE_TEXTURE3D                       = 34,
  AK_RENDER_STATE_TEXTURECUBE                     = 35,
  AK_RENDER_STATE_TEXTURERECT                     = 36,
  AK_RENDER_STATE_TEXTUREDEPTH                    = 37,
  AK_RENDER_STATE_TEXTURE1D_ENABLE                = 38,
  AK_RENDER_STATE_TEXTURE2D_ENABLE                = 39,
  AK_RENDER_STATE_TEXTURE3D_ENABLE                = 40,
  AK_RENDER_STATE_TEXTURECUBE_ENABLE              = 41,
  AK_RENDER_STATE_TEXTURERECT_ENABLE              = 42,
  AK_RENDER_STATE_TEXTUREDEPTH_ENABLE             = 43,
  AK_RENDER_STATE_TEXTURE_ENV_COLOR               = 44,
  AK_RENDER_STATE_TEXTURE_ENV_MODE                = 45,
  AK_RENDER_STATE_CLIP_PLANE                      = 46,
  AK_RENDER_STATE_CLIP_PLANE_ENABLE               = 47,
  AK_RENDER_STATE_BLEND_COLOR                     = 48,
  AK_RENDER_STATE_COLOR_MASK                      = 49,
  AK_RENDER_STATE_DEPTH_BOUNDS                    = 50,
  AK_RENDER_STATE_DEPTH_MASK                      = 51,
  AK_RENDER_STATE_DEPTH_RANGE                     = 52,
  AK_RENDER_STATE_FOG_DENSITY                     = 53,
  AK_RENDER_STATE_FOG_START                       = 54,
  AK_RENDER_STATE_FOG_END                         = 55,
  AK_RENDER_STATE_FOG_COLOR                       = 56,
  AK_RENDER_STATE_LIGHT_MODEL_AMBIENT             = 57,
  AK_RENDER_STATE_LIGHTING_ENABLE                 = 58,
  AK_RENDER_STATE_LINE_STIPPLE                    = 59,
  AK_RENDER_STATE_LINE_WIDTH                      = 60,
  AK_RENDER_STATE_MATERIAL_AMBIENT                = 61,
  AK_RENDER_STATE_MATERIAL_DIFFUSE                = 62,
  AK_RENDER_STATE_MATERIAL_EMISSION               = 63,
  AK_RENDER_STATE_MATERIAL_SHININESS              = 64,
  AK_RENDER_STATE_MATERIAL_SPECULAR               = 65,
  AK_RENDER_STATE_MODEL_VIEW_MATRIX               = 66,
  AK_RENDER_STATE_POINT_DISTANCE_ATTENUATION      = 67,
  AK_RENDER_STATE_POINT_FADE_THRESOLD_SIZE        = 68,
  AK_RENDER_STATE_POINT_SIZE                      = 69,
  AK_RENDER_STATE_POINT_SIZE_MIN                  = 70,
  AK_RENDER_STATE_POINT_SIZE_MAX                  = 71,
  AK_RENDER_STATE_POLYGON_OFFSET                  = 72,
  AK_RENDER_STATE_PROJECTION_MATRIX               = 73,
  AK_RENDER_STATE_SCISSOR                         = 74,
  AK_RENDER_STATE_STENCIL_MASK                    = 75,
  AK_RENDER_STATE_ALPHA_TEST_ENABLE               = 76,
  AK_RENDER_STATE_BLEND_ENABLE                    = 77,
  AK_RENDER_STATE_COLOR_LOGIC_OP_ENABLE           = 78,
  AK_RENDER_STATE_COLOR_MATERIAL_ENABLE           = 79,
  AK_RENDER_STATE_CULL_FACE_ENABLE                = 80,
  AK_RENDER_STATE_DEPTH_BOUNDS_ENABLE             = 81,
  AK_RENDER_STATE_DEPTH_CLAMP_ENABLE              = 82,
  AK_RENDER_STATE_DEPTH_TEST_ENABLE               = 83,
  AK_RENDER_STATE_DITHER_ENABLE                   = 84,
  AK_RENDER_STATE_FOG_ENABLE                      = 85,
  AK_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE = 86,
  AK_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE     = 87,
  AK_RENDER_STATE_LINE_SMOOTH_ENABLE              = 88,
  AK_RENDER_STATE_LINE_STIPPLE_ENABLE             = 89,
  AK_RENDER_STATE_LOGIC_OP_ENABLE                 = 90,
  AK_RENDER_STATE_MULTISAMPLE_ENABLE              = 91,
  AK_RENDER_STATE_NORMALIZE_ENABLE                = 92,
  AK_RENDER_STATE_POINT_SMOOTH_ENABLE             = 93,
  AK_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE      = 94,
  AK_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE      = 95,
  AK_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE     = 96,
  AK_RENDER_STATE_POLYGON_SMOOTH_ENABLE           = 97,
  AK_RENDER_STATE_POLYGON_STIPPLE_ENABLE          = 98,
  AK_RENDER_STATE_RESCALE_NORMAL_ENABLE           = 99,
  AK_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE = 100,
  AK_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE      = 101,
  AK_RENDER_STATE_SAMPLE_COVERAGE_ENABLE          = 102,
  AK_RENDER_STATE_SCISSOR_TEST_ENABLE             = 103,
  AK_RENDER_STATE_STENCIL_TEST_ENABLE             = 104
} AkRenderStateType;

typedef struct AkRenderState {
  AkRenderStateType      state_type;
  struct AkRenderState * next;
} AkRenderState;

typedef struct ak_alpha_func_s ak_alpha_func;
struct ak_alpha_func_s {
  AkRenderState base;

  struct {
    AkGlFunc   val;
    const char  * param;
  } func;

  struct {
    float        val;
    const char * param;
  } val;
};

typedef struct ak_blend_func_s ak_blend_func;
struct ak_blend_func_s {
  AkRenderState base;

  struct {
    AkGlBlend val;
    const char * param;
  } src;

  struct {
    AkGlBlend val;
    const char * param;
  } dest;
};

typedef struct ak_blend_func_separate_s ak_blend_func_separate;
struct ak_blend_func_separate_s {
  AkRenderState base;

  struct {
    AkGlBlend val;
    const char * param;
  } src_rgb;

  struct {
    AkGlBlend val;
    const char * param;
  } dest_rgb;

  struct {
    AkGlBlend val;
    const char * param;
  } src_alpha;

  struct {
    AkGlBlend val;
    const char * param;
  } dest_alpha;
};

typedef struct ak_blend_equation_separate_s ak_blend_equation_separate;
struct ak_blend_equation_separate_s {
  AkRenderState base;

  struct {
    AkGlBlendEquation val;
    const char * param;
  } rgb;

  struct {
    AkGlBlendEquation val;
    const char * param;
  } alpha;
};

typedef struct ak_color_material_s ak_color_material;
struct ak_color_material_s {
  AkRenderState base;

  struct {
    AkGlFace  val;
    const char * param;
  } face;

  struct {
    AkGlMaterialType val;
    const char * param;
  } mode;
};

typedef struct ak_polygon_mode_s ak_polygon_mode;
struct ak_polygon_mode_s {
  AkRenderState base;

  struct{
    AkGlFace  val;
    const char * param;
  } face;

  struct{
    AkGlPolygonMode val;
    const char * param;
  } mode;
};

typedef struct ak_stencil_func_s ak_stencil_func;
struct ak_stencil_func_s {
  AkRenderState base;

  struct{
    AkGlFunc  val;
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
  AkRenderState base;

  struct{
    AkGlStencilOp val;
    const char * param;
  } fail;

  struct{
    AkGlStencilOp val;
    const char * param;
  } zfail;

  struct{
    AkGlStencilOp val;
    const char * param;
  } zpass;
};

typedef struct ak_stencil_func_separate_s ak_stencil_func_separate;
struct ak_stencil_func_separate_s {
  AkRenderState base;

  struct{
    AkGlFunc  val;
    const char * param;
  } front;

  struct{
    AkGlFunc  val;
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
  AkRenderState base;

  struct{
    AkGlFace  val;
    const char * param;
  } face;

  struct{
    AkGlStencilOp val;
    const char * param;
  } fail;

  struct{
    AkGlStencilOp val;
    const char * param;
  } zfail;

  struct{
    AkGlStencilOp val;
    const char * param;
  } zpass;
};

typedef struct ak_stencil_mask_separate_s ak_stencil_mask_separate;
struct ak_stencil_mask_separate_s {
  AkRenderState base;

  struct{
    AkGlFace  val;
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
    AkRenderState base;                                                    \
    T val;                                                                    \
    const char * param;                                                       \
  };

#define _ak_DEF_STATE_T2(P, T)                                               \
  typedef struct ak_ ## P ## _s ak_ ## P;                                   \
  struct ak_ ## P ## _s {                                                    \
    AkRenderState base;                                                    \
    T val;                                                                    \
    const char * param;                                                       \
    unsigned long index;                                                      \
  };

_ak_DEF_STATE_T1(state_t_bool4,              AkBool4)
_ak_DEF_STATE_T1(state_t_int2,               AkInt2)
_ak_DEF_STATE_T1(state_t_int4,               AkInt4)
_ak_DEF_STATE_T1(state_t_ul,                 unsigned long )
_ak_DEF_STATE_T2(state_t_ul_i,               unsigned long )
_ak_DEF_STATE_T1(state_t_float,              AkFloat)
_ak_DEF_STATE_T2(state_t_float_i,            AkFloat)
_ak_DEF_STATE_T1(state_t_float2,             AkFloat2)
_ak_DEF_STATE_T1(state_t_float3,             AkFloat3)
_ak_DEF_STATE_T2(state_t_float3_i,           AkFloat3)
_ak_DEF_STATE_T1(state_t_float4,             AkFloat4)
_ak_DEF_STATE_T2(state_t_float4_i,           AkFloat4)
_ak_DEF_STATE_T1(state_t_float4x4,           AkFloat4x4)
_ak_DEF_STATE_T2(state_t_sampler,            AkFxSamplerCommon *)
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
