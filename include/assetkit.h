/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__assetkit__h_
#define __libassetkit__assetkit__h_

#include <stdlib.h>
#include <time.h>

/* since C99 or compiler ext */
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#  ifdef _assetkit_dll
#    define _assetkit_export __declspec(dllexport)
#  else
#    define _assetkit_export __declspec(dllimport)
#  endif
# define _assetkit_hide
#else
#  define _assetkit_export __attribute__((visibility("default")))
#  define _assetkit_hide   __attribute__((visibility("hidden")))
#endif

#if DEBUG
#  define __ak_assert(x) assert(x)
#else
#  define __ak_assert(x) /* do nothing */
#endif

#define __ak_restrict __restrict

#define ak_ARRAY_LEN(ARR) sizeof(ARR) / sizeof(ARR[0]);

/* Core Value types based on COLLADA specs 1.5 */

typedef const char *  ak_string;
typedef char       *  ak_mut_string;
typedef const char *  ak_str;
typedef char       *  ak_mutstr;
typedef unsigned const char * ak_ustr;
typedef bool          ak_bool;
typedef ak_bool      ak_bool2[2];
typedef ak_bool      ak_bool3[3];
typedef ak_bool      ak_bool4[4];
typedef double        ak_float;
typedef ak_float     ak_float2[2];
typedef ak_float     ak_float3[3];
typedef ak_float2    ak_float2x2[2];
typedef ak_float2    ak_float2x3[3];
typedef ak_float2    ak_float2x4[4];
typedef ak_float     ak_float3[3];
typedef ak_float3    ak_float3x2[2];
typedef ak_float3    ak_float3x3[3];
typedef ak_float3    ak_float3x4[4];
typedef ak_float     ak_float4[4];
typedef ak_float4    ak_float4x2[2];
typedef ak_float4    ak_float4x3[3];
typedef ak_float4    ak_float4x4[4];
typedef ak_float     ak_float7[7];
typedef int           ak_int;
typedef unsigned int  ak_uint;
typedef ak_int       ak_hexBinary;
typedef ak_int       ak_int2[2];
typedef ak_int2      ak_int2x2[2];
typedef ak_int       ak_int3[3];
typedef ak_int3      ak_int3x3[3];
typedef ak_int       ak_int4[4];
typedef ak_int4      ak_int4x4[4];
typedef ak_bool      ak_list_of_bools[];
typedef ak_float     ak_list_of_floats[];
typedef ak_int       ak_list_of_ints[];
typedef ak_uint      ak_list_of_uints[];
typedef ak_hexBinary ak_list_of_hexBinary[];
typedef ak_string  * ak_list_of_string;

/* End Core Value Types */

/** 
 * @brief library time type
 */
typedef time_t ak_time_t;

/*
 * Value Types
 */
#define ak_value_type long
#define ak_VALUE_TYPE_UNKNOWN                                          0x00
#define ak_VALUE_TYPE_BOOL                                             0x01
#define ak_VALUE_TYPE_BOOL2                                            0x02
#define ak_VALUE_TYPE_BOOL3                                            0x03
#define ak_VALUE_TYPE_BOOL4                                            0x04
#define ak_VALUE_TYPE_INT                                              0x05
#define ak_VALUE_TYPE_INT2                                             0x06
#define ak_VALUE_TYPE_INT3                                             0x07
#define ak_VALUE_TYPE_INT4                                             0x08
#define ak_VALUE_TYPE_FLOAT                                            0x09
#define ak_VALUE_TYPE_FLOAT2                                           0x10
#define ak_VALUE_TYPE_FLOAT3                                           0x11
#define ak_VALUE_TYPE_FLOAT4                                           0x12
#define ak_VALUE_TYPE_FLOAT2x2                                         0x13
#define ak_VALUE_TYPE_FLOAT3x3                                         0x14
#define ak_VALUE_TYPE_FLOAT4x4                                         0x15
#define ak_VALUE_TYPE_STRING                                           0x16

/*
 * Modifiers
 */
#define ak_modifier long
#define ak_MODIFIER_CONST                                              0x01
#define ak_MODIFIER_UNIFORM                                            0x02
#define ak_MODIFIER_VARYING                                            0x03
#define ak_MODIFIER_STATIC                                             0x04
#define ak_MODIFIER_VOLATILE                                           0x05
#define ak_MODIFIER_EXTERN                                             0x06
#define ak_MODIFIER_SHARED                                             0x07

/*
 * Profiles
 */
#define ak_profile_type long
#define ak_PROFILE_TYPE_UNKOWN                                         0x00
#define ak_PROFILE_TYPE_COMMON                                         0x01
#define ak_PROFILE_TYPE_CG                                             0x02
#define ak_PROFILE_TYPE_GLES                                           0x03
#define ak_PROFILE_TYPE_GLES2                                          0x04
#define ak_PROFILE_TYPE_GLSL                                           0x05
#define ak_PROFILE_TYPE_BRIDGE                                         0x06

/*
 * Technique Common
 */
#define ak_technique_common_type int
#define ak_TECHNIQUE_COMMON_CAMERA_PERSPECTIVE                         0x01
#define ak_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC                        0x02

#define ak_TECHNIQUE_COMMON_LIGHT_AMBIENT                              0x03
#define ak_TECHNIQUE_COMMON_LIGHT_DIRECTIONAL                          0x04
#define ak_TECHNIQUE_COMMON_LIGHT_POINT                                0x05
#define ak_TECHNIQUE_COMMON_LIGHT_SPOT                                 0x06

/**
 * @brief It keeps file type of asset doc
 *
 * @discussion To auto dedect file type by file extension set file type auto
 */
#define ak_filetype int
#define ak_FILE_TYPE_AUTO                                              0x00
#define ak_FILE_TYPE_COLLADA                                           0x01
#define ak_FILE_TYPE_WAVEFRONT                                         0x02
#define ak_FILE_TYPE_FBX                                               0x03

/**
 * @brief Represents asset type such as mesh, light, camera etc...
 *
 * @discussion Because assetkit library developed to include any asset
 */

#define ak_assettype long
#define ak_ASSET_TYPE_UNKNOWN                                          0x00
#define ak_ASSET_TYPE_CAMERA                                           0x01
#define ak_ASSET_TYPE_LIGHT                                            0x02

#define ak_upaxis long
#define ak_UP_AXIS_Y                                                   0x00
#define ak_UP_AXIS_X                                                   0x01
#define ak_UP_AXIS_Z                                                   0x02

#define ak_altitude_mode long
#define ak_ALTITUDE_RELATIVETOGROUND                                   0x00
#define ak_ALTITUDE_ABSOLUTE                                           0x01

/* FX */
#define ak_opaque long
#define ak_OPAQUE_A_ONE                                                0x00
#define ak_OPAQUE_RGB_ZERO                                             0x01
#define ak_OPAQUE_A_ZERO                                               0x02
#define ak_OPAQUE_RGB_ONE                                              0x03

#define ak_param_type long
#define ak_PARAM_TYPE_BASIC                                            0x00
#define ak_PARAM_TYPE_EXTENDED                                         0x01

#define ak_wrap_mode long
#define ak_WRAP_MODE_WRAP                                              0x00
#define ak_WRAP_MODE_MIRROR                                            0x01
#define ak_WRAP_MODE_CLAMP                                             0x02
#define ak_WRAP_MODE_BORDER                                            0x03
#define ak_WRAP_MODE_MIRROR_ONCE                                       0x04

#define ak_minfilter long
#define ak_MINFILTER_LINEAR                                            0x00
#define ak_MINFILTER_NEAREST                                           0x01
#define ak_MINFILTER_ANISOTROPIC                                       0x02

#define ak_magfilter long
#define ak_MAGFILTER_LINEAR                                            0x00
#define ak_MAGFILTER_NEAREST                                           0x01

#define ak_mipfilter long
#define ak_MIPFILTER_LINEAR                                            0x00
#define ak_MIPFILTER_NONE                                              0x01
#define ak_MIPFILTER_NEAREST                                           0x02

#define ak_face long
#define ak_FACE_POSITIVE_X                                             0x01
#define ak_FACE_NEGATIVE_X                                             0x02
#define ak_FACE_POSITIVE_Y                                             0x03
#define ak_FACE_NEGATIVE_Y                                             0x04
#define ak_FACE_POSITIVE_Z                                             0x05
#define ak_FACE_NEGATIVE_Z                                             0x06

#define ak_format_channel long
#define ak_FORMAT_CHANNEL_RGB                                          0x01
#define ak_FORMAT_CHANNEL_RGBA                                         0x02
#define ak_FORMAT_CHANNEL_RGBE                                         0x03
#define ak_FORMAT_CHANNEL_L                                            0x04
#define ak_FORMAT_CHANNEL_LA                                           0x05
#define ak_FORMAT_CHANNEL_D                                            0x06

#define ak_format_range long
#define ak_FORMAT_RANGE_SNORM                                          0x01
#define ak_FORMAT_RANGE_UNORM                                          0x02
#define ak_FORMAT_RANGE_SINT                                           0x03
#define ak_FORMAT_RANGE_UINT                                           0x04
#define ak_FORMAT_RANGE_FLOAT                                          0x05

#define ak_format_precision long
#define ak_FORMAT_PRECISION_DEFAULT                                    0x01
#define ak_FORMAT_PRECISION_LOW                                        0x02
#define ak_FORMAT_PRECISION_MID                                        0x03
#define ak_FORMAT_PRECISION_HIGHT                                      0x04
#define ak_FORMAT_PRECISION_MAX                                        0x05

/*
 * Render states
 */
#define ak_render_state_type long
#define ak_RENDER_STATE_ALPHA_FUNC                                   0x0001
#define ak_RENDER_STATE_BLEND_FUNC                                   0x0002
#define ak_RENDER_STATE_BLEND_FUNC_SEPARATE                          0x0003
#define ak_RENDER_STATE_BLEND_EQUATION                               0x0004
#define ak_RENDER_STATE_BLEND_EQUATION_SEPARATE                      0x0005
#define ak_RENDER_STATE_COLOR_MATERIAL                               0x0006
#define ak_RENDER_STATE_CULL_FACE                                    0x0007
#define ak_RENDER_STATE_DEPTH_FUNC                                   0x0008
#define ak_RENDER_STATE_FOG_MODE                                     0x0009
#define ak_RENDER_STATE_FOG_COORD_SRC                                0x0010
#define ak_RENDER_STATE_FRONT_FACE                                   0x0011
#define ak_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL                    0x0012
#define ak_RENDER_STATE_LOGIC_OP                                     0x0013
#define ak_RENDER_STATE_POLYGON_MODE                                 0x0014
#define ak_RENDER_STATE_SHADE_MODEL                                  0x0015
#define ak_RENDER_STATE_STENCIL_FUNC                                 0x0016
#define ak_RENDER_STATE_STENCIL_OP                                   0x0017
#define ak_RENDER_STATE_STENCIL_FUNC_SEPARATE                        0x0018
#define ak_RENDER_STATE_STENCIL_OP_SEPARATE                          0x0019
#define ak_RENDER_STATE_STENCIL_MASK_SEPARATE                        0x0020
#define ak_RENDER_STATE_LIGHT_ENABLE                                 0x0021
#define ak_RENDER_STATE_LIGHT_AMBIENT                                0x0022
#define ak_RENDER_STATE_LIGHT_DIFFUSE                                0x0023
#define ak_RENDER_STATE_LIGHT_SPECULAR                               0x0024
#define ak_RENDER_STATE_LIGHT_POSITION                               0x0025
#define ak_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION                   0x0026
#define ak_RENDER_STATE_LIGHT_LINEAR_ATTENUATION                     0x0027
#define ak_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION                    0x0028
#define ak_RENDER_STATE_LIGHT_SPOT_CUTOFF                            0x0029
#define ak_RENDER_STATE_LIGHT_SPOT_DIRECTION                         0x0030
#define ak_RENDER_STATE_LIGHT_SPOT_EXPONENT                          0x0031
#define ak_RENDER_STATE_TEXTURE1D                                    0x0032
#define ak_RENDER_STATE_TEXTURE2D                                    0x0033
#define ak_RENDER_STATE_TEXTURE3D                                    0x0034
#define ak_RENDER_STATE_TEXTURECUBE                                  0x0035
#define ak_RENDER_STATE_TEXTURERECT                                  0x0036
#define ak_RENDER_STATE_TEXTUREDEPTH                                 0x0037
#define ak_RENDER_STATE_TEXTURE1D_ENABLE                             0x0038
#define ak_RENDER_STATE_TEXTURE2D_ENABLE                             0x0039
#define ak_RENDER_STATE_TEXTURE3D_ENABLE                             0x0040
#define ak_RENDER_STATE_TEXTURECUBE_ENABLE                           0x0041
#define ak_RENDER_STATE_TEXTURERECT_ENABLE                           0x0042
#define ak_RENDER_STATE_TEXTUREDEPTH_ENABLE                          0x0043
#define ak_RENDER_STATE_TEXTURE_ENV_COLOR                            0x0044
#define ak_RENDER_STATE_TEXTURE_ENV_MODE                             0x0045
#define ak_RENDER_STATE_CLIP_PLANE                                   0x0046
#define ak_RENDER_STATE_CLIP_PLANE_ENABLE                            0x0047
#define ak_RENDER_STATE_BLEND_COLOR                                  0x0048
#define ak_RENDER_STATE_COLOR_MASK                                   0x0049
#define ak_RENDER_STATE_DEPTH_BOUNDS                                 0x0050
#define ak_RENDER_STATE_DEPTH_MASK                                   0x0051
#define ak_RENDER_STATE_DEPTH_RANGE                                  0x0052
#define ak_RENDER_STATE_FOG_DENSITY                                  0x0053
#define ak_RENDER_STATE_FOG_START                                    0x0054
#define ak_RENDER_STATE_FOG_END                                      0x0055
#define ak_RENDER_STATE_FOG_COLOR                                    0x0056
#define ak_RENDER_STATE_LIGHT_MODEL_AMBIENT                          0x0057
#define ak_RENDER_STATE_LIGHTING_ENABLE                              0x0058
#define ak_RENDER_STATE_LINE_STIPPLE                                 0x0059
#define ak_RENDER_STATE_LINE_WIDTH                                   0x0060
#define ak_RENDER_STATE_MATERIAL_AMBIENT                             0x0061
#define ak_RENDER_STATE_MATERIAL_DIFFUSE                             0x0062
#define ak_RENDER_STATE_MATERIAL_EMISSION                            0x0063
#define ak_RENDER_STATE_MATERIAL_SHININESS                           0x0064
#define ak_RENDER_STATE_MATERIAL_SPECULAR                            0x0065
#define ak_RENDER_STATE_MODEL_VIEW_MATRIX                            0x0066
#define ak_RENDER_STATE_POINT_DISTANCE_ATTENUATION                   0x0067
#define ak_RENDER_STATE_POINT_FADE_THRESOLD_SIZE                     0x0068
#define ak_RENDER_STATE_POINT_SIZE                                   0x0069
#define ak_RENDER_STATE_POINT_SIZE_MIN                               0x0070
#define ak_RENDER_STATE_POINT_SIZE_MAX                               0x0071
#define ak_RENDER_STATE_POLYGON_OFFSET                               0x0072
#define ak_RENDER_STATE_PROJECTION_MATRIX                            0x0073
#define ak_RENDER_STATE_SCISSOR                                      0x0074
#define ak_RENDER_STATE_STENCIL_MASK                                 0x0075
#define ak_RENDER_STATE_ALPHA_TEST_ENABLE                            0x0076
#define ak_RENDER_STATE_BLEND_ENABLE                                 0x0077
#define ak_RENDER_STATE_COLOR_LOGIC_OP_ENABLE                        0x0078
#define ak_RENDER_STATE_COLOR_MATERIAL_ENABLE                        0x0079
#define ak_RENDER_STATE_CULL_FACE_ENABLE                             0x0080
#define ak_RENDER_STATE_DEPTH_BOUNDS_ENABLE                          0x0081
#define ak_RENDER_STATE_DEPTH_CLAMP_ENABLE                           0x0082
#define ak_RENDER_STATE_DEPTH_TEST_ENABLE                            0x0083
#define ak_RENDER_STATE_DITHER_ENABLE                                0x0084
#define ak_RENDER_STATE_FOG_ENABLE                                   0x0085
#define ak_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE              0x0086
#define ak_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE                  0x0087
#define ak_RENDER_STATE_LINE_SMOOTH_ENABLE                           0x0088
#define ak_RENDER_STATE_LINE_STIPPLE_ENABLE                          0x0089
#define ak_RENDER_STATE_LOGIC_OP_ENABLE                              0x0090
#define ak_RENDER_STATE_MULTISAMPLE_ENABLE                           0x0091
#define ak_RENDER_STATE_NORMALIZE_ENABLE                             0x0092
#define ak_RENDER_STATE_POINT_SMOOTH_ENABLE                          0x0093
#define ak_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE                   0x0094
#define ak_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE                   0x0095
#define ak_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE                  0x0096
#define ak_RENDER_STATE_POLYGON_SMOOTH_ENABLE                        0x0097
#define ak_RENDER_STATE_POLYGON_STIPPLE_ENABLE                       0x0098
#define ak_RENDER_STATE_RESCALE_NORMAL_ENABLE                        0x0099
#define ak_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE              0x0100
#define ak_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE                   0x0101
#define ak_RENDER_STATE_SAMPLE_COVERAGE_ENABLE                       0x0102
#define ak_RENDER_STATE_SCISSOR_TEST_ENABLE                          0x0103
#define ak_RENDER_STATE_STENCIL_TEST_ENABLE                          0x0104

#define ak_gl_func long
#define ak_GL_FUNC_NEVER                                             0x0200
#define ak_GL_FUNC_LESS                                              0x0201
#define ak_GL_FUNC_EQUAL                                             0x0202
#define ak_GL_FUNC_LEQUAL                                            0x0203
#define ak_GL_FUNC_GREATER                                           0x0204
#define ak_GL_FUNC_NOTEQUAL                                          0x0205
#define ak_GL_FUNC_GEQUAL                                            0x0206
#define ak_GL_FUNC_ALWAYS                                            0x0207

#define ak_gl_blend long
#define ak_GL_BLEND_ZERO                                             0x00
#define ak_GL_BLEND_ONE                                              0x01
#define ak_GL_BLEND_SRC_COLOR                                        0x0300
#define ak_GL_BLEND_ONE_MINUS_SRC_COLOR                              0x0301
#define ak_GL_BLEND_DEST_COLOR                                       0x0306
#define ak_GL_BLEND_ONE_MINUS_DEST_COLOR                             0x0307
#define ak_GL_BLEND_SRC_ALPHA                                        0x0302
#define ak_GL_BLEND_ONE_MINUS_SRC_ALPHA                              0x0303
#define ak_GL_BLEND_DST_ALPHA                                        0x0304
#define ak_GL_BLEND_ONE_MINUS_DST_ALPHA                              0x0305
#define ak_GL_BLEND_CONSTANT_COLOR                                   0x8001
#define ak_GL_BLEND_ONE_MINUS_CONSTANT_COLOR                         0x8002
#define ak_GL_BLEND_CONSTANT_ALPHA                                   0x8003
#define ak_GL_BLEND_ONE_MINUS_CONSTANT_ALPHA                         0x8004
#define ak_GL_BLEND_SRC_ALPHA_SATURATE                               0x0308

#define ak_gl_blend_equation long
#define ak_GL_BLEND_EQUATION_FUNC_ADD                                0x8006
#define ak_GL_BLEND_EQUATION_FUNC_SUBTRACT                           0x800A
#define ak_GL_BLEND_EQUATION_FUNC_REVERSE_SUBTRACT                   0x800B
#define ak_GL_BLEND_EQUATION_MIN                                     0x8007
#define ak_GL_BLEND_EQUATION_MAX                                     0x8008

#define ak_gl_face long
#define ak_GL_FACE_FRONT                                             0x0404
#define ak_GL_FACE_BACK                                              0x0405
#define ak_GL_FACE_FRONT_AND_BACK                                    0x0408

#define ak_gl_front_face long
#define ak_GL_FRONT_FACE_CW                                          0x0900
#define ak_GL_FRONT_FACE_CCW                                         0x0901

#define ak_gl_material long
#define ak_GL_MATERIAL_EMISSION                                      0x1600
#define ak_GL_MATERIAL_AMBIENT                                       0x1200
#define ak_GL_MATERIAL_DIFFUSE                                       0x1201
#define ak_GL_MATERIAL_SPECULAR                                      0x1202
#define ak_GL_MATERIAL_AMBIENT_AND_DIFFUSE                           0x1602

#define ak_gl_fog long
#define ak_GL_FOG_LINEAR                                             0x2601
#define ak_GL_FOG_EXP                                                0x0800
#define ak_GL_FOG_EXP2                                               0x0801

#define ak_gl_fog_coord_src long
#define ak_GL_FOG_COORD_SRC_FOG_COORDINATE                           0x8451
#define ak_GL_FOG_COORD_SRC_FRAGMENT_DEPTH                           0x8452

#define ak_gl_light_model_color_control long
#define ak_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR                 0x81F9
#define ak_GL_LIGHT_MODEL_COLOR_CONTROL_SEPARATE_SPECULAR_COLOR      0x81FA

#define ak_gl_logic_op long
#define ak_GL_LOGIC_OP_CLEAR                                         0x1500
#define ak_GL_LOGIC_OP_AND                                           0x1501
#define ak_GL_LOGIC_OP_AND_REVERSE                                   0x1502
#define ak_GL_LOGIC_OP_COPY                                          0x1503
#define ak_GL_LOGIC_OP_AND_INVERTED                                  0x1504
#define ak_GL_LOGIC_OP_NOOP                                          0x1505
#define ak_GL_LOGIC_OP_XOR                                           0x1506
#define ak_GL_LOGIC_OP_OR                                            0x1507
#define ak_GL_LOGIC_OP_NOR                                           0x1508
#define ak_GL_LOGIC_OP_EQUIV                                         0x1509
#define ak_GL_LOGIC_OP_INVERT                                        0x150A
#define ak_GL_LOGIC_OP_OR_REVERSE                                    0x150B
#define ak_GL_LOGIC_OP_COPY_INVERTED                                 0x150C
#define ak_GL_LOGIC_OP_NAND                                          0x150E
#define ak_GL_LOGIC_OP_SET                                           0x150F

#define ak_gl_polygon_mode long
#define ak_GL_POLYGON_MODE_POINT                                     0x1B00
#define ak_GL_POLYGON_MODE_LINE                                      0x1B01
#define ak_GL_POLYGON_MODE_FILL                                      0x1B02

#define ak_gl_shade_model long
#define ak_GL_SHADE_MODEL_FLAT                                       0x1D00
#define ak_GL_SHADE_MODEL_SMOOTH                                     0x1D01

#define ak_gl_stencil_op long
#define ak_GL_STENCIL_OP_KEEP                                        0x1E00
#define ak_GL_STENCIL_OP_ZERO                                        0x0
#define ak_GL_STENCIL_OP_REPLACE                                     0x1E01
#define ak_GL_STENCIL_OP_INCR                                        0x1E02
#define ak_GL_STENCIL_OP_DECR                                        0x1E03
#define ak_GL_STENCIL_OP_INVERT                                      0x150A
#define ak_GL_STENCIL_OP_INCR_WRAP                                   0x8507
#define ak_GL_STENCIL_OP_DECR_WRAP                                   0x8508

/* DRAW */
#define ak_draw_enum long
#define ak_DRAW_READ_STR_VAL                                         0x0000
#define ak_DRAW_GEOMETRY                                             0x0001
#define ak_DRAW_SCENE_GEOMETRY                                       0x0002
#define ak_DRAW_SCENE_IMAGE                                          0x0003
#define ak_DRAW_FULL_SCREEN_QUAD                                     0x0004
#define ak_DRAW_FULL_SCREEN_QUAD_PLUS_HALF_PIXEL                     0x0005
/* users can extend the draw enum elsewhere by custom values */

#define ak_pipeline_stage long
#define ak_PIPELINE_STAGE_VERTEX                                     0x0001
#define ak_PIPELINE_STAGE_FRAGMENT                                   0x0002
#define ak_PIPELINE_STAGE_TESSELATION                                0x0003
#define ak_PIPELINE_STAGE_GEOMETRY                                   0x0004

/**
 * Almost all assets includes this fields. 
 * This macro defines base fields of assets
 */
#define ak_asset_base                                                        \
  ak_assetinf  * inf;                                                        \
  ak_assettype   type;

/**
 * @brief attribute for a node. To get attribute count fetch attrc from node
 * instead of iterating each attribute
 */
typedef struct ak_tree_node_attr_s ak_tree_node_attr;

struct ak_tree_node_attr_s {
  const char * name;
  char       * val;

  ak_tree_node_attr * next;
  ak_tree_node_attr * prev;
};

/**
 * @brief list node for general purpose.
 */
typedef struct ak_tree_node_s ak_tree_node;

/**
 * @brief identically to list_node
 */
typedef struct ak_tree_node_s ak_tree;

struct ak_tree_node_s {
  const char    * name;
  char          * val;
  unsigned long   attrc;
  unsigned long   chldc;

  ak_tree_node_attr * attr;
  ak_tree_node      * chld;
  ak_tree_node      * parent;
  ak_tree_node      * next;
  ak_tree_node      * prev;
};


#ifdef _ak_DEF_BASIC_ATTR
#  undef _ak_DEF_BASIC_ATTR
#endif

#define _ak_DEF_BASIC_ATTR(T, P)                                             \
typedef struct ak_basic_attr ## P ## _s ak_basic_attr ## P;                 \
struct ak_basic_attr ## P ## _s {                                            \
  const char * sid;                                                           \
  T            val;                                                           \
}

_ak_DEF_BASIC_ATTR(float, f);
_ak_DEF_BASIC_ATTR(double, d);
_ak_DEF_BASIC_ATTR(long, l);
_ak_DEF_BASIC_ATTR(const char *, s);

#undef _ak_DEF_BASIC_ATTR

typedef struct ak_unit_s ak_unit;
struct ak_unit_s {
  const char * name;
  double       dist;
};

typedef struct ak_color_s ak_color;
struct ak_color_s {
  const char * sid;
  union {
    ak_float4 vec;

    struct {
      float R;
      float G;
      float B;
      float A;
    };
  };
};

typedef struct ak_contributor_s ak_contributor;
struct ak_contributor_s {
  const char * author;
  const char * author_email;
  const char * author_website;
  const char * authoring_tool;
  const char * comments;
  const char * copyright;
  const char * source_data;

  ak_contributor * next;
  ak_contributor * prev;
};

typedef struct ak_altitude_s ak_altitude;
struct ak_altitude_s {
  ak_altitude_mode mode;
  double            val;
};

typedef struct ak_geo_loc_s ak_geo_loc;
struct ak_geo_loc_s {
  double       lng;
  double       lat;
  ak_altitude alt;
};

typedef struct ak_coverage_s ak_coverage;
struct ak_coverage_s {
  ak_geo_loc geo_loc;
};

typedef struct ak_assetinf_s ak_assetinf;
struct ak_assetinf_s {
  ak_contributor * contributor;
  ak_coverage    * coverage;
  const char      * subject;
  const char      * title;
  const char      * keywords;

  ak_unit        * unit;
  ak_tree        * extra;
  ak_time_t        created;
  ak_time_t        modified;
  unsigned long     revision;
  ak_upaxis        upaxis;
};

typedef struct ak_docinf_s ak_docinf;
struct ak_docinf_s {
  ak_assetinf   base;
  const char   * fname;
  ak_filetype   ftype;
};

/*
 * TODO:
 * There should be an option (ak_load) to prevent optional 
 * load unsed / non-portable techniques. 
 * This may increase parse performance and reduce memory usage
 */
typedef struct ak_technique_s ak_technique;
struct ak_technique_s {
  const char * profile;

  /**
   * @brief
   * COLLADA Specs 1.5:
   * This XML Schema namespace attribute identifies an additional schema 
   * to use for validating the content of this instance document. Optional.
   */
  const char * xmlns;
  ak_tree   * chld;

  ak_technique * prev;
  ak_technique * next;
};

typedef struct ak_technique_common_s ak_technique_common;
struct ak_technique_common_s {
  ak_technique_common      * next;
  void                      * technique;
  ak_technique_common_type   technique_type;
};

typedef struct ak_perspective_s ak_perspective;
struct ak_perspective_s {
  ak_basic_attrd * xfov;
  ak_basic_attrd * yfov;
  ak_basic_attrd * aspect_ratio;
  ak_basic_attrd * znear;
  ak_basic_attrd * zfar;
};

typedef struct ak_orthographic_s ak_orthographic;
struct ak_orthographic_s {
  ak_basic_attrd * xmag;
  ak_basic_attrd * ymag;
  ak_basic_attrd * aspect_ratio;
  ak_basic_attrd * znear;
  ak_basic_attrd * zfar;
};

typedef struct ak_optics_s ak_optics;
struct ak_optics_s {
  ak_technique_common * technique_common;
  ak_technique        * technique;
};

typedef struct ak_imager_s ak_imager;
struct ak_imager_s {
  ak_technique * technique;
  ak_tree      * extra;
};

/**
 * Declares a view of the visual scene hierarchy or scene graph.
 * The camera contains elements that describe the camera’s optics and imager.
 */
typedef struct ak_camera_s ak_camera;
struct ak_camera_s {
  ak_asset_base

  const char * id;
  const char * name;

  ak_optics * optics;
  ak_imager * imager;
  ak_tree   * extra;

  ak_camera * next;
};

/**
 * ambient light
 */
typedef struct ak_ambient_s ak_ambient;
struct ak_ambient_s {
  ak_color color;
};

/**
 * directional light
 */
typedef struct ak_directional_s ak_directional;
struct ak_directional_s {
  ak_color color;
};

/**
 * point light
 */
typedef struct ak_point_s ak_point;
struct ak_point_s {
  ak_color         color;
  ak_basic_attrd * constant_attenuation;
  ak_basic_attrd * linear_attenuation;
  ak_basic_attrd * quadratic_attenuation;
};

/**
 * spot light
 */
typedef struct ak_spot_s ak_spot;
struct ak_spot_s {
  ak_color         color;
  ak_basic_attrd * constant_attenuation;
  ak_basic_attrd * linear_attenuation;
  ak_basic_attrd * quadratic_attenuation;
  ak_basic_attrd * falloff_angle;
  ak_basic_attrd * falloff_exponent;
};

/**
 * Declares a light source that illuminates a scene.
 */
typedef struct ak_light_s ak_light;
struct ak_light_s {
  ak_asset_base

  const char * id;
  const char * name;

  ak_technique_common * technique_common;
  ak_technique        * technique;
  ak_tree             * extra;

  ak_light * next;
};

/* FX */
/* Effects */
/*
 * base type of param
 */
typedef struct ak_param_s ak_param;
struct ak_param_s {
  ak_param_type  param_type;
  const char * ref;

  ak_param * next;
};

typedef struct ak_param_ex_s ak_param_ex;
struct ak_param_ex_s {
  ak_param        base;
  const char     * name;
  const char     * sid;
  const char     * semantic;
  const char     * type_name;
  ak_value_type   type;
};

typedef struct ak_hex_data_s ak_hex_data;
struct ak_hex_data_s {
  const char * format;
  const char * val;
};

typedef struct ak_init_from_s ak_init_from;
struct ak_init_from_s {
  const char   * ref;
  ak_hex_data * hex;
  
  ak_face     face;
  ak_uint     mip_index;
  ak_uint     depth;
  ak_int      array_index;
  ak_bool     mips_generate;

  ak_init_from * prev;
  ak_init_from * next;
};

typedef struct ak_size_exact_s ak_size_exact;
struct ak_size_exact_s {
  ak_float width;
  ak_float height;
};

typedef struct ak_size_ratio_s ak_size_ratio;
struct ak_size_ratio_s {
  ak_float width;
  ak_float height;
};

typedef struct ak_mips_s ak_mips;
struct ak_mips_s {
  ak_uint levels;
  ak_bool auto_generate;
};

typedef struct ak_image_format_s ak_image_format;
struct ak_image_format_s {
  struct {
    ak_format_channel    channel;
    ak_format_range      range;
    ak_format_precision  precision;
    const char          * space;
  } hint;

  const char * exact;
};

typedef struct ak_image2d_s ak_image2d;
struct ak_image2d_s {
  ak_size_exact   * size_exact;
  ak_size_ratio   * size_ratio;
  ak_mips         * mips;
  const char       * unnormalized;
  long               array_len;
  ak_image_format * format;
  ak_init_from    * init_from;
};

typedef struct ak_image3d_s ak_image3d;
struct ak_image3d_s {
  struct {
    ak_uint width;
    ak_uint height;
    ak_uint depth;
  } size;

  ak_mips           mips;
  long               array_len;
  ak_image_format * format;
  ak_init_from    * init_from;
};

typedef struct ak_image_cube_s ak_image_cube;
struct ak_image_cube_s {
  struct {
    ak_uint width;
  } size;

  ak_mips           mips;
  long               array_len;
  ak_image_format * format;
  ak_init_from    * init_from;
};

typedef struct ak_image_s ak_image;
struct ak_image_s {
  ak_asset_base
  
  const char * id;
  const char * sid;
  const char * name;

  ak_init_from  * init_from;
  ak_image2d    * image2d;
  ak_image3d    * image3d;
  ak_image_cube * cube;
  ak_tree       * extra;

  ak_image * prev;
  ak_image * next;
  
  struct {
    ak_bool share;
  } renderable;

};

typedef struct ak_image_instance_s ak_image_instance;
struct ak_image_instance_s {
  const char * url;
  const char * sid;
  const char * name;

  ak_tree * extra;
};

/*!
 * base type for these types:
 * sampler1D
 * sampler2D
 * sampler3D
 * samplerCUBE
 * samplerDEPTH
 * samplerRECT
 * samplerStates
 */
typedef struct ak_fx_sampler_common_s ak_fx_sampler_common;
struct ak_fx_sampler_common_s {
  ak_asset_base
  ak_image_instance * image_inst;

  struct {
    const char * semantic;
  } texcoord;

  ak_wrap_mode wrap_s;
  ak_wrap_mode wrap_t;
  ak_wrap_mode wrap_p;

  ak_minfilter minfilter;
  ak_magfilter magfilter;
  ak_mipfilter mipfilter;

  unsigned long mip_max_level;
  unsigned long mip_min_level;
  float         mip_bias;
  unsigned long max_anisotropy;

  ak_color * border_color;
  ak_tree  * extra;
};

typedef ak_fx_sampler_common ak_sampler1D;
typedef ak_fx_sampler_common ak_sampler2D;
typedef ak_fx_sampler_common ak_sampler3D;
typedef ak_fx_sampler_common ak_samplerCUBE;
typedef ak_fx_sampler_common ak_samplerDEPTH;
typedef ak_fx_sampler_common ak_samplerRECT;
typedef ak_fx_sampler_common ak_samplerStates;

typedef struct ak_fx_texture_s ak_fx_texture;
struct ak_fx_texture_s {
  const char * texture;
  const char * texcoord;
  ak_tree   * extra;
};

typedef struct ak_fx_color_or_tex_s ak_fx_color_or_tex;
struct ak_fx_color_or_tex_s {
  ak_opaque       opaque;
  ak_color      * color;
  ak_param      * param;
  ak_fx_texture * texture;
};

typedef ak_fx_color_or_tex ak_ambient_fx;
typedef ak_fx_color_or_tex ak_diffuse;
typedef ak_fx_color_or_tex ak_emission;
typedef ak_fx_color_or_tex ak_reflective;
typedef ak_fx_color_or_tex ak_specular;
typedef ak_fx_color_or_tex ak_transparent;

typedef struct ak_fx_float_or_param_s ak_fx_float_or_param;
struct ak_fx_float_or_param_s {
  ak_basic_attrf * val;
  ak_param       * param;
};

typedef ak_fx_float_or_param ak_index_of_refraction;
typedef ak_fx_float_or_param ak_reflectivity;
typedef ak_fx_float_or_param ak_shininess;
typedef ak_fx_float_or_param ak_transparency;

typedef struct ak_annotate_s ak_annotate;
struct ak_annotate_s {
  const char     * name;
  void           * val;
  ak_value_type   val_type;

  ak_annotate * next;
};

typedef struct ak_newparam_s ak_newparam;
struct ak_newparam_s {
  const char     * sid;
  ak_annotate   * annotate;
  const char     * semantic;
  ak_modifier     modifier;
  void           * val;
  ak_value_type   val_type;

  ak_newparam * next;
};

typedef struct ak_setparam_s ak_setparam;
struct ak_setparam_s {
  const char     * ref;
  void           * val;
  ak_value_type   val_type;

  ak_setparam * next;
};

typedef struct ak_code_s ak_code;
struct ak_code_s {
  const char * sid;
  const char * val;

  ak_code * next;
};

typedef struct ak_include_s ak_include;
struct ak_include_s {
  const char * sid;
  const char * url;

  ak_include * next;
};

typedef struct ak_blinn_s ak_blinn;
struct ak_blinn_s {
  ak_emission            * emission;
  ak_ambient_fx          * ambient;
  ak_diffuse             * diffuse;
  ak_specular            * specular;
  ak_shininess           * shininess;
  ak_reflective          * reflective;
  ak_reflectivity        * reflectivity;
  ak_transparent         * transparent;
  ak_transparency        * transparency;
  ak_index_of_refraction * index_of_refraction;
};

typedef struct ak_constant_fx_s ak_constant_fx;
struct ak_constant_fx_s {
  ak_emission            * emission;
  ak_reflective          * reflective;
  ak_reflectivity        * reflectivity;
  ak_transparent         * transparent;
  ak_transparency        * transparency;
  ak_index_of_refraction * index_of_refraction;
};

typedef struct ak_lambert_s ak_lambert;
struct ak_lambert_s {
  ak_emission            * emission;
  ak_ambient_fx          * ambient;
  ak_diffuse             * diffuse;
  ak_reflective          * reflective;
  ak_reflectivity        * reflectivity;
  ak_transparent         * transparent;
  ak_transparency        * transparency;
  ak_index_of_refraction * index_of_refraction;
};

typedef struct ak_phong_s ak_phong;
struct ak_phong_s {
  ak_emission            * emission;
  ak_ambient_fx          * ambient;
  ak_diffuse             * diffuse;
  ak_specular            * specular;
  ak_shininess           * shininess;
  ak_reflective          * reflective;
  ak_reflectivity        * reflectivity;
  ak_transparent         * transparent;
  ak_transparency        * transparency;
  ak_index_of_refraction * index_of_refraction;
};

typedef struct ak_render_state_s ak_render_state;
typedef struct ak_states_s ak_states;
struct ak_states_s {
  ak_render_state * next;
  long               count;
};

typedef struct ak_evaluate_target_s ak_evaluate_target;
struct ak_evaluate_target_s {
  unsigned long index;
  unsigned long slice;
  unsigned long mip;
  ak_face      face;

  ak_param          * param;
  ak_image_instance * image_inst;
};

typedef struct ak_color_clear_s ak_color_clear;
struct ak_color_clear_s {
  unsigned long index;
  ak_color     val;
};

typedef struct ak_depth_clear_s ak_depth_clear;
struct ak_depth_clear_s {
  unsigned long index;
  ak_float     val;
};

typedef struct ak_stencil_clear_s ak_stencil_clear;
struct ak_stencil_clear_s {
  unsigned long index;
  unsigned long val;
};

typedef struct ak_draw_s ak_draw;
struct ak_draw_s {
  ak_draw_enum enum_draw;
  const char    * str_val;
};

typedef struct ak_evaluate_s ak_evaluate;
struct ak_evaluate_s {
  ak_evaluate_target * color_target;
  ak_evaluate_target * depth_target;
  ak_evaluate_target * stencil_target;
  ak_color_clear     * color_clear;
  ak_depth_clear     * depth_clear;
  ak_stencil_clear   * stencil_clear;
  ak_draw              draw;
};

typedef struct ak_inline_s ak_inline;
struct ak_inline_s {
  const char * val;
  ak_inline * next;
};

typedef struct ak_import_s ak_import;
struct ak_import_s {
  const char * ref;
  ak_import * next;
};

typedef struct ak_sources_s ak_sources;
struct ak_sources_s {
  const char * entry;

  ak_inline * inlines;
  ak_import * imports;
};

typedef struct ak_binary_s ak_binary;
struct ak_binary_s {
  const char * ref;
  ak_hex_data * hex;

  ak_binary * next;
};

typedef struct ak_compiler_s ak_compiler;
struct ak_compiler_s {
  const char * platform;
  const char * target;
  const char * options;
  ak_binary * binary;

  ak_compiler * next;
};

typedef struct ak_bind_uniform_s ak_bind_uniform;
struct ak_bind_uniform_s {
  const char * symbol;

  ak_param       * param;
  void            * val;
  ak_value_type    val_type;

  ak_bind_uniform * next;
};

typedef struct ak_bind_attrib_s ak_bind_attrib;
struct ak_bind_attrib_s {
  const char * symbol;
  const char * semantic;

  ak_bind_attrib * next;
};

typedef struct ak_shader_s ak_shader;
struct ak_shader_s {
  ak_pipeline_stage stage;

  ak_sources      * sources;
  ak_compiler     * compiler;
  ak_bind_uniform * bind_uniform;
  ak_tree         * extra;

  ak_shader * prev;
  ak_shader * next;
};

typedef struct ak_linker_s ak_linker;
struct ak_linker_s {
  const char * platform;
  const char * target;
  const char * options;

  ak_binary * binary;

  ak_linker * prev;
  ak_linker * next;
};

typedef struct ak_program_s ak_program;
struct ak_program_s {
  ak_shader       * shader;
  ak_bind_attrib  * bind_attrib;
  ak_bind_uniform * bind_uniform;
  ak_linker       * linker;
};

typedef struct ak_pass_s ak_pass;
struct ak_pass_s {
  ak_asset_base

  const char * sid;

  ak_annotate     * annotate;
  ak_states       * states;
  ak_evaluate     * evaluate;
  ak_program      * program;
  ak_tree         * extra;

  ak_pass * prev;
  ak_pass * next;
};

typedef struct ak_technique_fx_s ak_technique_fx;
struct ak_technique_fx_s {
  ak_asset_base

  const char * id;
  const char * sid;

  ak_annotate    * annotate;
  ak_blinn       * blinn;
  ak_constant_fx * constant;
  ak_lambert     * lambert;
  ak_phong       * phong;
  ak_pass        * pass;

  ak_tree        * extra;

  ak_technique_fx * next;
};

typedef struct ak_profile_s ak_profile;
struct ak_profile_s {
  ak_asset_base                                                           
  ak_profile_type   profile_type;
  const char       * id;
  ak_newparam     * newparam;
  ak_technique_fx * technique;
  ak_tree         * extra;
  ak_profile      * prev;
  ak_profile      * next;
};

typedef ak_profile ak_profile_common;

typedef struct ak_profile_cg_s ak_profile_CG;
struct ak_profile_cg_s {
  ak_profile base;

  ak_code      * code;
  ak_include   * include;
  const char    * platform;
};

typedef struct ak_profile_gles_s ak_profile_GLES;
struct ak_profile_gles_s {
  ak_profile base;

  const char * platform;
};

typedef struct ak_profile_gles2_s ak_profile_GLES2;
struct ak_profile_gles2_s {
  ak_profile base;

  ak_code    * code;
  ak_include * include;
  const char  * language;
  const char  * platforms;
};

typedef struct ak_profile_glsl_s ak_profile_GLSL;
struct ak_profile_glsl_s {
  ak_profile base;

  ak_code    * code;
  ak_include * include;
  const char  * platform;
};

typedef struct ak_profile_bridge_s ak_profile_BRIDGE;
struct ak_profile_bridge_s {
  ak_profile base;
  
  const char * platform;
  const char * url;
};

typedef struct ak_effect_s ak_effect;
struct ak_effect_s {
  ak_asset_base

  const char * id;
  const char * name;

  ak_annotate * annotate;
  ak_newparam * newparam;
  ak_profile  * profile;

  ak_tree   * extra;

  ak_effect * prev;
  ak_effect * next;
};

typedef struct ak_technique_hint_s ak_technique_hint;
struct ak_technique_hint_s {
  const char * platform;
  const char * ref;
  const char * profile;

  ak_technique_hint * next;
};

typedef struct ak_effect_instance_s ak_effect_instance;
struct ak_effect_instance_s {
  const char * url;
  const char * sid;
  const char * name;

  ak_technique_hint *techniqueHint;
  ak_setparam       *setparam;

  ak_tree * extra;
};

typedef struct ak_material_s ak_material;
struct ak_material_s {
  ak_asset_base
  const char * id;
  const char * name;
  ak_effect_instance *effect_inst;
  ak_tree * extra;

  ak_material * next;
};

#undef _ak_DEF_LIB

#define _ak_DEF_LIB(T)                                                       \
  typedef struct ak_lib_ ## T ## _s  ak_lib_ ## T;                          \
  struct ak_lib_ ## T ## _s {                                                \
    ak_assetinf  * inf;                                                      \
    const char    * id;                                                       \
    const char    * name;                                                     \
    ak_##T       * chld;                                                     \
    ak_tree      * extra;                                                    \
    unsigned long   count;                                                    \
    ak_lib_##T   * next;                                                     \
  }

_ak_DEF_LIB(camera);
_ak_DEF_LIB(light);
_ak_DEF_LIB(effect);
_ak_DEF_LIB(image);
_ak_DEF_LIB(material);

#undef _ak_DEF_LIB

typedef struct ak_lib_s ak_lib;
struct ak_lib_s {
  ak_lib_camera   * cameras;
  ak_lib_light    * lights;
  ak_lib_effect   * effects;
  ak_lib_image    * images;
  ak_lib_material * materials;
};

typedef struct ak_doc_s ak_doc;
struct ak_doc_s {
  ak_docinf docinf;
  ak_lib    lib;
};

typedef struct ak_asset_s ak_asset;
struct ak_asset_s {
  ak_asset_base
  void * ak_data;
};

#include "assetkit-states.h"
#include "assetkit-string.h"

extern void ak_cleanup();

int
_assetkit_export
ak_load(ak_doc ** __restrict dest,
         const char * __restrict file, ...);

#ifdef __cplusplus
}
#endif
#endif /* __libassetkit__assetkit__h_ */
