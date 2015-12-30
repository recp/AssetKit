/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__assetio__h_
#define __libassetio__assetio__h_

#include <stdlib.h>
#include <time.h>

/* since C99 or compiler ext */
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#  ifdef _assetio_dll
#    define _assetio_export __declspec(dllexport)
#  else
#    define _assetio_export __declspec(dllimport)
#  endif
# define _assetio_hide
#else
#  define _assetio_export __attribute__((visibility("default")))
#  define _assetio_hide   __attribute__((visibility("hidden")))
#endif

#if DEBUG
#  define __aio_assert(x) assert(x)
#else
#  define __aio_assert(x) /* do nothing */
#endif

#define __aio_restrict __restrict

#define AIO_ARRAY_LEN(ARR) sizeof(ARR) / sizeof(ARR[0]);

/* Core Value types based on COLLADA specs 1.5 */

typedef const char *  aio_string;
typedef char       *  aio_mut_string;
typedef bool          aio_bool;
typedef aio_bool      aio_bool2[2];
typedef aio_bool      aio_bool3[3];
typedef aio_bool      aio_bool4[4];
typedef double        aio_float;
typedef aio_float     aio_float2[2];
typedef aio_float     aio_float3[3];
typedef aio_float2    aio_float2x2[2];
typedef aio_float2    aio_float2x3[3];
typedef aio_float2    aio_float2x4[4];
typedef aio_float     aio_float3[3];
typedef aio_float3    aio_float3x2[2];
typedef aio_float3    aio_float3x3[3];
typedef aio_float3    aio_float3x4[4];
typedef aio_float     aio_float4[4];
typedef aio_float4    aio_float4x2[2];
typedef aio_float4    aio_float4x3[3];
typedef aio_float4    aio_float4x4[4];
typedef aio_float     aio_float7[7];
typedef int           aio_int;
typedef unsigned int  aio_uint;
typedef aio_int       aio_hexBinary;
typedef aio_int       aio_int2[2];
typedef aio_int2      aio_int2x2[2];
typedef aio_int       aio_int3[3];
typedef aio_int3      aio_int3x3[3];
typedef aio_int       aio_int4[4];
typedef aio_int4      aio_int4x4[4];
typedef aio_bool      aio_list_of_bools[];
typedef aio_float     aio_list_of_floats[];
typedef aio_int       aio_list_of_ints[];
typedef aio_uint      aio_list_of_uints[];
typedef aio_hexBinary aio_list_of_hexBinary[];
typedef aio_string  * aio_list_of_string;

/* End Core Value Types */

/** 
 * @brief library time type
 */
typedef time_t aio_time_t;

/*
 * Value Types
 */
#define aio_value_type int
#define AIO_VALUE_TYPE_BOOL                                             0x01
#define AIO_VALUE_TYPE_BOOL2                                            0x02
#define AIO_VALUE_TYPE_BOOL3                                            0x03
#define AIO_VALUE_TYPE_BOOL4                                            0x03
#define AIO_VALUE_TYPE_INT                                              0x04
#define AIO_VALUE_TYPE_INT2                                             0x05
#define AIO_VALUE_TYPE_INT3                                             0x06
#define AIO_VALUE_TYPE_INT4                                             0x07
#define AIO_VALUE_TYPE_FLOAT                                            0x08
#define AIO_VALUE_TYPE_FLOAT2                                           0x09
#define AIO_VALUE_TYPE_FLOAT3                                           0x10
#define AIO_VALUE_TYPE_FLOAT4                                           0x11
#define AIO_VALUE_TYPE_FLOAT2x2                                         0x12
#define AIO_VALUE_TYPE_FLOAT3x3                                         0x13
#define AIO_VALUE_TYPE_FLOAT4x4                                         0x14
#define AIO_VALUE_TYPE_STRING                                           0x15

/*
 * Modifiers
 */
#define aio_modifier long
#define AIO_MODIFIER_CONST                                              0x01
#define AIO_MODIFIER_UNIFORM                                            0x02
#define AIO_MODIFIER_VARYING                                            0x03
#define AIO_MODIFIER_STATIC                                             0x04
#define AIO_MODIFIER_VOLATILE                                           0x05
#define AIO_MODIFIER_EXTERN                                             0x06
#define AIO_MODIFIER_SHARED                                             0x07

/*
 * Profiles
 */
#define aio_profile_type long
#define AIO_PROFILE_TYPE_COMMON                                         0x01
#define AIO_PROFILE_TYPE_CG                                             0x02
#define AIO_PROFILE_TYPE_GLES                                           0x03
#define AIO_PROFILE_TYPE_GLES2                                          0x04
#define AIO_PROFILE_TYPE_GLSL                                           0x05
#define AIO_PROFILE_TYPE_BRIDGE                                         0x06

/*
 * Technique Common
 */
#define aio_technique_common_type int
#define AIO_TECHNIQUE_COMMON_CAMERA_PERSPECTIVE                         0x01
#define AIO_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC                        0x02

#define AIO_TECHNIQUE_COMMON_LIGHT_AMBIENT                              0x03
#define AIO_TECHNIQUE_COMMON_LIGHT_DIRECTIONAL                          0x04
#define AIO_TECHNIQUE_COMMON_LIGHT_POINT                                0x05
#define AIO_TECHNIQUE_COMMON_LIGHT_SPOT                                 0x06

/**
 * @brief It keeps file type of asset doc
 *
 * @discussion To auto dedect file type by file extension set file type auto
 */
#define aio_filetype int
#define AIO_FILE_TYPE_AUTO                                              0x00
#define AIO_FILE_TYPE_COLLADA                                           0x01
#define AIO_FILE_TYPE_WAVEFRONT                                         0x02
#define AIO_FILE_TYPE_FBX                                               0x03

/**
 * @brief Represents asset type such as mesh, light, camera etc...
 *
 * @discussion Because assetio library developed to include any asset
 */

#define aio_assettype long
#define AIO_ASSET_TYPE_UNKNOWN                                          0x00
#define AIO_ASSET_TYPE_CAMERA                                           0x01
#define AIO_ASSET_TYPE_LIGHT                                            0x02

#define aio_upaxis long
#define AIO_UP_AXIS_Y                                                   0x00
#define AIO_UP_AXIS_X                                                   0x01
#define AIO_UP_AXIS_Z                                                   0x02

#define aio_altitude_mode long
#define AIO_ALTITUDE_RELATIVETOGROUND                                   0x00
#define AIO_ALTITUDE_ABSOLUTE                                           0x01

/* FX */
#define aio_opaque long
#define AIO_OPAQUE_A_ONE                                                0x00
#define AIO_OPAQUE_RGB_ZERO                                             0x01
#define AIO_OPAQUE_A_ZERO                                               0x02
#define AIO_OPAQUE_RGB_ONE                                              0x03

#define aio_param_type long
#define AIO_PARAM_TYPE_BASIC                                            0x00
#define AIO_PARAM_TYPE_EXTENDED                                         0x01

#define aio_wrap_mode long
#define AIO_WRAP_MODE_WRAP                                              0x00
#define AIO_WRAP_MODE_MIRROR                                            0x01
#define AIO_WRAP_MODE_CLAMP                                             0x02
#define AIO_WRAP_MODE_BORDER                                            0x03
#define AIO_WRAP_MODE_MIRROR_ONCE                                       0x04

#define aio_minfilter long
#define AIO_MINFILTER_LINEAR                                            0x00
#define AIO_MINFILTER_NEAREST                                           0x01
#define AIO_MINFILTER_ANISOTROPIC                                       0x02

#define aio_magfilter long
#define AIO_MAGFILTER_LINEAR                                            0x00
#define AIO_MAGFILTER_NEAREST                                           0x01

#define aio_mipfilter long
#define AIO_MIPFILTER_LINEAR                                            0x00
#define AIO_MIPFILTER_NONE                                              0x01
#define AIO_MIPFILTER_NEAREST                                           0x02

#define aio_face long
#define AIO_FACE_POSITIVE_X                                             0x01
#define AIO_FACE_NEGATIVE_X                                             0x02
#define AIO_FACE_POSITIVE_Y                                             0x03
#define AIO_FACE_NEGATIVE_Y                                             0x04
#define AIO_FACE_POSITIVE_Z                                             0x05
#define AIO_FACE_NEGATIVE_Z                                             0x06

#define aio_format_channel long
#define AIO_FORMAT_CHANNEL_RGB                                          0x01
#define AIO_FORMAT_CHANNEL_RGBA                                         0x02
#define AIO_FORMAT_CHANNEL_RGBE                                         0x03
#define AIO_FORMAT_CHANNEL_L                                            0x04
#define AIO_FORMAT_CHANNEL_LA                                           0x05
#define AIO_FORMAT_CHANNEL_D                                            0x06

#define aio_format_range long
#define AIO_FORMAT_RANGE_SNORM                                          0x01
#define AIO_FORMAT_RANGE_UNORM                                          0x02
#define AIO_FORMAT_RANGE_SINT                                           0x03
#define AIO_FORMAT_RANGE_UINT                                           0x04
#define AIO_FORMAT_RANGE_FLOAT                                          0x05

#define aio_format_precision long
#define AIO_FORMAT_PRECISION_DEFAULT                                    0x01
#define AIO_FORMAT_PRECISION_LOW                                        0x02
#define AIO_FORMAT_PRECISION_MID                                        0x03
#define AIO_FORMAT_PRECISION_HIGHT                                      0x04
#define AIO_FORMAT_PRECISION_MAX                                        0x05

/*
 * Render states
 */
#define aio_render_state_type long
#define AIO_RENDER_STATE_ALPHA_FUNC                                   0x0001
#define AIO_RENDER_STATE_BLEND_FUNC                                   0x0002
#define AIO_RENDER_STATE_BLEND_FUNC_SEPARATE                          0x0003
#define AIO_RENDER_STATE_BLEND_EQUATION                               0x0004
#define AIO_RENDER_STATE_BLEND_EQUATION_SEPARATE                      0x0005
#define AIO_RENDER_STATE_COLOR_MATERIAL                               0x0006
#define AIO_RENDER_STATE_CULL_FACE                                    0x0007
#define AIO_RENDER_STATE_DEPTH_FUNC                                   0x0008
#define AIO_RENDER_STATE_FOG_MODE                                     0x0009
#define AIO_RENDER_STATE_FOG_COORD_SRC                                0x0010
#define AIO_RENDER_STATE_FRONT_FACE                                   0x0011
#define AIO_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL                    0x0012
#define AIO_RENDER_STATE_LOGIC_OP                                     0x0013
#define AIO_RENDER_STATE_POLYGON_MODE                                 0x0014
#define AIO_RENDER_STATE_SHADE_MODEL                                  0x0015
#define AIO_RENDER_STATE_STENCIL_FUNC                                 0x0016
#define AIO_RENDER_STATE_STENCIL_OP                                   0x0017
#define AIO_RENDER_STATE_STENCIL_FUNC_SEPARATE                        0x0018
#define AIO_RENDER_STATE_STENCIL_OP_SEPARATE                          0x0019
#define AIO_RENDER_STATE_STENCIL_MASK_SEPARATE                        0x0020
#define AIO_RENDER_STATE_LIGHT_ENABLE                                 0x0021
#define AIO_RENDER_STATE_LIGHT_AMBIENT                                0x0022
#define AIO_RENDER_STATE_LIGHT_DIFFUSE                                0x0023
#define AIO_RENDER_STATE_LIGHT_SPECULAR                               0x0024
#define AIO_RENDER_STATE_LIGHT_POSITION                               0x0025
#define AIO_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION                   0x0026
#define AIO_RENDER_STATE_LIGHT_LINEAR_ATTENUATION                     0x0027
#define AIO_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION                    0x0028
#define AIO_RENDER_STATE_LIGHT_SPOT_CUTOFF                            0x0029
#define AIO_RENDER_STATE_LIGHT_SPOT_DIRECTION                         0x0030
#define AIO_RENDER_STATE_LIGHT_SPOT_EXPONENT                          0x0031
#define AIO_RENDER_STATE_TEXTURE1D                                    0x0032
#define AIO_RENDER_STATE_TEXTURE2D                                    0x0033
#define AIO_RENDER_STATE_TEXTURE3D                                    0x0034
#define AIO_RENDER_STATE_TEXTURECUBE                                  0x0035
#define AIO_RENDER_STATE_TEXTURERECT                                  0x0036
#define AIO_RENDER_STATE_TEXTUREDEPTH                                 0x0037
#define AIO_RENDER_STATE_TEXTURE1D_ENABLE                             0x0038
#define AIO_RENDER_STATE_TEXTURE2D_ENABLE                             0x0039
#define AIO_RENDER_STATE_TEXTURE3D_ENABLE                             0x0040
#define AIO_RENDER_STATE_TEXTURECUBE_ENABLE                           0x0041
#define AIO_RENDER_STATE_TEXTURERECT_ENABLE                           0x0042
#define AIO_RENDER_STATE_TEXTUREDEPTH_ENABLE                          0x0043
#define AIO_RENDER_STATE_TEXTURE_ENV_COLOR                            0x0044
#define AIO_RENDER_STATE_TEXTURE_ENV_MODE                             0x0045
#define AIO_RENDER_STATE_CLIP_PLANE                                   0x0046
#define AIO_RENDER_STATE_CLIP_PLANE_ENABLE                            0x0047
#define AIO_RENDER_STATE_BLEND_COLOR                                  0x0048
#define AIO_RENDER_STATE_COLOR_MASK                                   0x0049
#define AIO_RENDER_STATE_DEPTH_BOUNDS                                 0x0050
#define AIO_RENDER_STATE_DEPTH_MASK                                   0x0051
#define AIO_RENDER_STATE_DEPTH_RANGE                                  0x0052
#define AIO_RENDER_STATE_FOG_DENSITY                                  0x0053
#define AIO_RENDER_STATE_FOG_START                                    0x0054
#define AIO_RENDER_STATE_FOG_END                                      0x0055
#define AIO_RENDER_STATE_FOG_COLOR                                    0x0056
#define AIO_RENDER_STATE_LIGHT_MODEL_AMBIENT                          0x0057
#define AIO_RENDER_STATE_LIGHTING_ENABLE                              0x0058
#define AIO_RENDER_STATE_LINE_STIPPLE                                 0x0059
#define AIO_RENDER_STATE_LINE_WIDTH                                   0x0060
#define AIO_RENDER_STATE_MATERIAL_AMBIENT                             0x0061
#define AIO_RENDER_STATE_MATERIAL_DIFFUSE                             0x0062
#define AIO_RENDER_STATE_MATERIAL_EMISSION                            0x0063
#define AIO_RENDER_STATE_MATERIAL_SHININESS                           0x0064
#define AIO_RENDER_STATE_MATERIAL_SPECULAR                            0x0065
#define AIO_RENDER_STATE_MODEL_VIEW_MATRIX                            0x0066
#define AIO_RENDER_STATE_POINT_DISTANCE_ATTENUATION                   0x0067
#define AIO_RENDER_STATE_POINT_FADE_THRESOLD_SIZE                     0x0068
#define AIO_RENDER_STATE_POINT_SIZE                                   0x0069
#define AIO_RENDER_STATE_POINT_SIZE_MIN                               0x0070
#define AIO_RENDER_STATE_POINT_SIZE_MAX                               0x0071
#define AIO_RENDER_STATE_POLYGON_OFFSET                               0x0072
#define AIO_RENDER_STATE_PROJECTION_MATRIX                            0x0073
#define AIO_RENDER_STATE_SCISSOR                                      0x0074
#define AIO_RENDER_STATE_STENCIL_MASK                                 0x0075
#define AIO_RENDER_STATE_ALPHA_TEST_ENABLE                            0x0076
#define AIO_RENDER_STATE_BLEND_ENABLE                                 0x0077
#define AIO_RENDER_STATE_COLOR_LOGIC_OP_ENABLE                        0x0078
#define AIO_RENDER_STATE_COLOR_MATERIAL_ENABLE                        0x0079
#define AIO_RENDER_STATE_CULL_FACE_ENABLE                             0x0080
#define AIO_RENDER_STATE_DEPTH_BOUNDS_ENABLE                          0x0081
#define AIO_RENDER_STATE_DEPTH_CLAMP_ENABLE                           0x0082
#define AIO_RENDER_STATE_DEPTH_TEST_ENABLE                            0x0083
#define AIO_RENDER_STATE_DITHER_ENABLE                                0x0084
#define AIO_RENDER_STATE_FOG_ENABLE                                   0x0085
#define AIO_RENDER_STATE_LIGTH_MODEL_LOCAL_VIEWER_ENABLE              0x0086
#define AIO_RENDER_STATE_LIGTH_MODEL_TWO_SIDE_ENABLE                  0x0087
#define AIO_RENDER_STATE_LINE_SMOOTH_ENABLE                           0x0088
#define AIO_RENDER_STATE_LINE_STIPPLE_ENABLE                          0x0089
#define AIO_RENDER_STATE_LOGIC_OP_ENABLE                              0x0090
#define AIO_RENDER_STATE_MULTISAMPLE_ENABLE                           0x0091
#define AIO_RENDER_STATE_NORMALIZE_ENABLE                             0x0092
#define AIO_RENDER_STATE_POINT_SMOOTH_ENABLE                          0x0093
#define AIO_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE                   0x0094
#define AIO_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE                   0x0095
#define AIO_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE                  0x0096
#define AIO_RENDER_STATE_POLYGON_SMOOTH_ENABLE                        0x0097
#define AIO_RENDER_STATE_POLYGON_STIPPLE_ENABLE                       0x0098
#define AIO_RENDER_STATE_RESCALE_NORMAL_ENABLE                        0x0099
#define AIO_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE              0x0100
#define AIO_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE                   0x0101
#define AIO_RENDER_STATE_SAMPLE_COVERAGE_ENABLE                       0x0102
#define AIO_RENDER_STATE_SCISSOR_TEST_ENABLE                          0x0103
#define AIO_RENDER_STATE_STENCIL_TEST_ENABLE                          0x0104

#define aio_gl_func long
#define AIO_GL_FUNC_NEVER                                             0x0200
#define AIO_GL_FUNC_LESS                                              0x0201
#define AIO_GL_FUNC_EQUAL                                             0x0202
#define AIO_GL_FUNC_LEQUAL                                            0x0203
#define AIO_GL_FUNC_GREATER                                           0x0204
#define AIO_GL_FUNC_NOTEQUAL                                          0x0205
#define AIO_GL_FUNC_GEQUAL                                            0x0206
#define AIO_GL_FUNC_ALWAYS                                            0x0207

#define aio_gl_blend long
#define AIO_GL_BLEND_ZERO                                             0x00
#define AIO_GL_BLEND_ONE                                              0x01
#define AIO_GL_BLEND_SRC_COLOR                                        0x0300
#define AIO_GL_BLEND_ONE_MINUS_SRC_COLOR                              0x0301
#define AIO_GL_BLEND_DEST_COLOR                                       0x0306
#define AIO_GL_BLEND_ONE_MINUS_DEST_COLOR                             0x0307
#define AIO_GL_BLEND_SRC_ALPHA                                        0x0302
#define AIO_GL_BLEND_ONE_MINUS_SRC_ALPHA                              0x0303
#define AIO_GL_BLEND_DST_ALPHA                                        0x0304
#define AIO_GL_BLEND_ONE_MINUS_DST_ALPHA                              0x0305
#define AIO_GL_BLEND_CONSTANT_COLOR                                   0x8001
#define AIO_GL_BLEND_ONE_MINUS_CONSTANT_COLOR                         0x8002
#define AIO_GL_BLEND_CONSTANT_ALPHA                                   0x8003
#define AIO_GL_BLEND_ONE_MINUS_CONSTANT_ALPHA                         0x8004
#define AIO_GL_BLEND_SRC_ALPHA_SATURATE                               0x0308

#define aio_gl_blend_equation long
#define AIO_GL_BLEND_EQUATION_FUNC_ADD                                0x8006
#define AIO_GL_BLEND_EQUATION_FUNC_SUBTRACT                           0x800A
#define AIO_GL_BLEND_EQUATION_FUNC_REVERSE_SUBTRACT                   0x800B
#define AIO_GL_BLEND_EQUATION_MIN                                     0x8007
#define AIO_GL_BLEND_EQUATION_MAX                                     0x8008

#define aio_gl_face long
#define AIO_GL_FACE_FRONT                                             0x0404
#define AIO_GL_FACE_BACK                                              0x0405
#define AIO_GL_FACE_FRONT_AND_BACK                                    0x0408

#define aio_gl_front_face long
#define AIO_GL_FRONT_FACE_CW                                          0x0900
#define AIO_GL_FRONT_FACE_CCW                                         0x0901

#define aio_gl_material long
#define AIO_GL_MATERIAL_EMISSION                                      0x1600
#define AIO_GL_MATERIAL_AMBIENT                                       0x1200
#define AIO_GL_MATERIAL_DIFFUSE                                       0x1201
#define AIO_GL_MATERIAL_SPECULAR                                      0x1202
#define AIO_GL_MATERIAL_AMBIENT_AND_DIFFUSE                           0x1602

#define aio_gl_fog long
#define AIO_GL_FOG_LINEAR                                             0x2601
#define AIO_GL_FOG_EXP                                                0x0800
#define AIO_GL_FOG_EXP2                                               0x0801

#define aio_gl_fog_coord_src long
#define AIO_GL_FOG_COORD_SRC_FOG_COORDINATE                           0x8451
#define AIO_GL_FOG_COORD_SRC_FRAGMENT_DEPTH                           0x8452

#define aio_gl_light_model_color_control long
#define AIO_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR                 0x81F9
#define AIO_GL_LIGHT_MODEL_COLOR_CONTROL_SEPARATE_SPECULAR_COLOR      0x81FA

#define aio_gl_logic_op long
#define AIO_GL_LOGIC_OP_CLEAR                                         0x1500
#define AIO_GL_LOGIC_OP_AND                                           0x1501
#define AIO_GL_LOGIC_OP_AND_REVERSE                                   0x1502
#define AIO_GL_LOGIC_OP_COPY                                          0x1503
#define AIO_GL_LOGIC_OP_AND_INVERTED                                  0x1504
#define AIO_GL_LOGIC_OP_NOOP                                          0x1505
#define AIO_GL_LOGIC_OP_XOR                                           0x1506
#define AIO_GL_LOGIC_OP_OR                                            0x1507
#define AIO_GL_LOGIC_OP_NOR                                           0x1508
#define AIO_GL_LOGIC_OP_EQUIV                                         0x1509
#define AIO_GL_LOGIC_OP_INVERT                                        0x150A
#define AIO_GL_LOGIC_OP_OR_REVERSE                                    0x150B
#define AIO_GL_LOGIC_OP_COPY_INVERTED                                 0x150C
#define AIO_GL_LOGIC_OP_NAND                                          0x150E
#define AIO_GL_LOGIC_OP_SET                                           0x150F

#define aio_gl_polygon_mode long
#define AIO_GL_POLYGON_MODE_POINT                                     0x1B00
#define AIO_GL_POLYGON_MODE_LINE                                      0x1B01
#define AIO_GL_POLYGON_MODE_FILL                                      0x1B02

#define aio_gl_shade_model long
#define AIO_GL_SHADE_MODEL_FLAT                                       0x1D00
#define AIO_GL_SHADE_MODEL_SMOOTH                                     0x1D01

#define aio_gl_stencil_op long
#define AIO_GL_STENCIL_OP_KEEP                                        0x1E00
#define AIO_GL_STENCIL_OP_ZERO                                        0x0
#define AIO_GL_STENCIL_OP_REPLACE                                     0x1E01
#define AIO_GL_STENCIL_OP_INCR                                        0x1E02
#define AIO_GL_STENCIL_OP_DECR                                        0x1E03
#define AIO_GL_STENCIL_OP_INVERT                                      0x150A
#define AIO_GL_STENCIL_OP_INCR_WRAP                                   0x8507
#define AIO_GL_STENCIL_OP_DECR_WRAP                                   0x8508

/* DRAW */
#define aio_draw_enum long
#define AIO_DRAW_READ_STR_VAL                                         0x0000
#define AIO_DRAW_GEOMETRY                                             0x0001
#define AIO_DRAW_SCENE_GEOMETRY                                       0x0002
#define AIO_DRAW_SCENE_IMAGE                                          0x0003
#define AIO_DRAW_FULL_SCREEN_QUAD                                     0x0004
#define AIO_DRAW_FULL_SCREEN_QUAD_PLUS_HALF_PIXEL                     0x0005
/* users can extend the draw enum elsewhere by custom values */

#define aio_pipeline_stage long
#define AIO_PIPELINE_STAGE_TESSELATION                                  0x0000
#define AIO_PIPELINE_STAGE_VERTEX                                       0x0000
#define AIO_PIPELINE_STAGE_GEOMETRY                                         0x0000
#define AIO_PIPELINE_STAGE_FRAGMENT                                         0x0000

/**
 * Almost all assets includes this fields. 
 * This macro defines base fields of assets
 */
#define aio_asset_base                                                        \
  aio_assetinf  * inf;                                                        \
  aio_assettype   type;

/**
 * @brief attribute for a node. To get attribute count fetch attrc from node
 * instead of iterating each attribute
 */
typedef struct aio_tree_node_attr_s aio_tree_node_attr;

struct aio_tree_node_attr_s {
  const char * name;
  char       * val;

  aio_tree_node_attr * next;
  aio_tree_node_attr * prev;
};

/**
 * @brief list node for general purpose.
 */
typedef struct aio_tree_node_s aio_tree_node;

/**
 * @brief identically to list_node
 */
typedef struct aio_tree_node_s aio_tree;

struct aio_tree_node_s {
  const char    * name;
  char          * val;
  unsigned long   attrc;
  unsigned long   chldc;

  aio_tree_node_attr * attr;
  aio_tree_node      * chld;
  aio_tree_node      * parent;
  aio_tree_node      * next;
  aio_tree_node      * prev;
};


#ifdef _AIO_DEF_BASIC_ATTR
#  undef _AIO_DEF_BASIC_ATTR
#endif

#define _AIO_DEF_BASIC_ATTR(T, P)                                             \
typedef struct aio_basic_attr ## P ## _s aio_basic_attr ## P;                 \
struct aio_basic_attr ## P ## _s {                                            \
  const char * sid;                                                           \
  T            val;                                                           \
}

_AIO_DEF_BASIC_ATTR(float, f);
_AIO_DEF_BASIC_ATTR(double, d);
_AIO_DEF_BASIC_ATTR(long, l);
_AIO_DEF_BASIC_ATTR(const char *, s);

#undef _AIO_DEF_BASIC_ATTR

typedef struct aio_unit_s aio_unit;
struct aio_unit_s {
  const char * name;
  double       dist;
};

typedef struct aio_color_s aio_color;
struct aio_color_s {
  const char * sid;
  union {
    float vec[4];

    struct {
      float R;
      float G;
      float B;
      float A;
    };
  };
};

typedef struct aio_contributor_s aio_contributor;
struct aio_contributor_s {
  const char * author;
  const char * author_email;
  const char * author_website;
  const char * authoring_tool;
  const char * comments;
  const char * copyright;
  const char * source_data;

  aio_contributor * next;
  aio_contributor * prev;
};

typedef struct aio_docinf_s aio_docinf;
struct aio_docinf_s {
  aio_contributor * contributor;
  aio_unit        * unit;
  const char      * subject;
  const char      * title;
  const char      * keywords;
  const char      * copyright;
  const char      * comments;
  const char      * tooldesc;
  const char      * fname;
  aio_time_t        created;
  aio_time_t        modified;
  unsigned long     revision;
  aio_upaxis        upaxis;
  aio_filetype      ftype;
};

typedef struct aio_altitude_s aio_altitude;
struct aio_altitude_s {
  aio_altitude_mode mode;
  double            val;
};

typedef struct aio_geo_loc_s aio_geo_loc;
struct aio_geo_loc_s {
  double       lng;
  double       lat;
  aio_altitude alt;
};

typedef struct aio_coverage_s aio_coverage;
struct aio_coverage_s {
  aio_geo_loc geo_loc;
};

typedef struct aio_assetinf_s aio_assetinf;
struct aio_assetinf_s {
  aio_contributor * contributor;
  aio_coverage    * coverage;
  const char      * subject;
  const char      * title;
  const char      * keywords;

  aio_unit        * unit;
  aio_tree        * extra;
  aio_time_t        created;
  aio_time_t        modified;
  unsigned long     revision;
  aio_upaxis        upaxis;
};

/*
 * TODO:
 * There should be an option (aio_load) to prevent optional 
 * load unsed / non-portable techniques. 
 * This may increase parse performance and reduce memory usage
 */
typedef struct aio_technique_s aio_technique;
struct aio_technique_s {
  const char * profile;

  /**
   * @brief
   * COLLADA Specs 1.5:
   * This XML Schema namespace attribute identifies an additional schema 
   * to use for validating the content of this instance document. Optional.
   */
  const char * xmlns;
  aio_tree   * chld;

  aio_technique * prev;
  aio_technique * next;
};

typedef struct aio_technique_common_s aio_technique_common;
struct aio_technique_common_s {
  aio_technique_common      * next;
  void                      * technique;
  aio_technique_common_type   technique_type;
};

typedef struct aio_perspective_s aio_perspective;
struct aio_perspective_s {
  aio_basic_attrd * xfov;
  aio_basic_attrd * yfov;
  aio_basic_attrd * aspect_ratio;
  aio_basic_attrd * znear;
  aio_basic_attrd * zfar;
};

typedef struct aio_orthographic_s aio_orthographic;
struct aio_orthographic_s {
  aio_basic_attrd * xmag;
  aio_basic_attrd * ymag;
  aio_basic_attrd * aspect_ratio;
  aio_basic_attrd * znear;
  aio_basic_attrd * zfar;
};

typedef struct aio_optics_s aio_optics;
struct aio_optics_s {
  aio_technique_common * technique_common;
  aio_technique        * technique;
};

typedef struct aio_imager_s aio_imager;
struct aio_imager_s {
  aio_technique * technique;
  aio_tree      * extra;
};

/**
 * Declares a view of the visual scene hierarchy or scene graph.
 * The camera contains elements that describe the camera’s optics and imager.
 */
typedef struct aio_camera_s aio_camera;
struct aio_camera_s {
  aio_asset_base

  const char * id;
  const char * name;

  aio_optics * optics;
  aio_imager * imager;
  aio_tree   * extra;

  aio_camera * next;
  aio_camera * prev;
};

/**
 * ambient light
 */
typedef struct aio_ambient_s aio_ambient;
struct aio_ambient_s {
  aio_color color;
};

/**
 * directional light
 */
typedef struct aio_directional_s aio_directional;
struct aio_directional_s {
  aio_color color;
};

/**
 * point light
 */
typedef struct aio_point_s aio_point;
struct aio_point_s {
  aio_color         color;
  aio_basic_attrd * constant_attenuation;
  aio_basic_attrd * linear_attenuation;
  aio_basic_attrd * quadratic_attenuation;
};

/**
 * spot light
 */
typedef struct aio_spot_s aio_spot;
struct aio_spot_s {
  aio_color         color;
  aio_basic_attrd * constant_attenuation;
  aio_basic_attrd * linear_attenuation;
  aio_basic_attrd * quadratic_attenuation;
  aio_basic_attrd * falloff_angle;
  aio_basic_attrd * falloff_exponent;
};

/**
 * Declares a light source that illuminates a scene.
 */
typedef struct aio_light_s aio_light;
struct aio_light_s {
  aio_asset_base

  const char * id;
  const char * name;

  aio_technique_common * technique_common;
  aio_technique        * technique;
  aio_tree             * extra;

  aio_light * next;
  aio_light * prev;
};

#pragma mark -
#pragma mark FX

/* FX */
/* Effects */
/*
 * base type of param
 */
#define _AIO_PARAM_BASE_                                                      \
  aio_param_type  param_type;                                                 \
  aio_param     * prev;                                                       \
  aio_param     * next

typedef struct aio_param_s aio_param;
struct aio_param_s {
  _AIO_PARAM_BASE_;
};

typedef struct aio_param_basic_s aio_param_basic;
struct aio_param_basic_s {
  _AIO_PARAM_BASE_;
  union {
    const char * val;
    const char * ref;
  };
};

typedef struct aio_param_extended_s aio_param_extended;
struct aio_param_extended_s {
  _AIO_PARAM_BASE_;
  const char     * val;
  const char     * name;
  const char     * sid;
  const char     * semantic;
  const char     * type_name;
  aio_value_type   type;
};

typedef struct aio_hex_data_s aio_hex_data;
struct aio_hex_data_s {
  const char * format;
  const char * val;
};

typedef struct aio_init_from_s aio_init_from;
struct aio_init_from_s {
  const char   * ref;
  aio_hex_data * hex;
  
  aio_face     face;
  aio_uint     mip_index;
  aio_uint     depth;
  aio_int      array_index;
  aio_bool     mips_generate;

  aio_init_from * prev;
  aio_init_from * next;
};

typedef struct aio_size_exact_s aio_size_exact;
struct aio_size_exact_s {
  aio_float width;
  aio_float height;
};

typedef struct aio_size_ratio_s aio_size_ratio;
struct aio_size_ratio_s {
  aio_float width;
  aio_float height;
};

typedef struct aio_mips_s aio_mips;
struct aio_mips_s {
  aio_uint levels;
  aio_bool auto_generate;
};

typedef struct aio_image_format_s aio_image_format;
struct aio_image_format_s {
  struct {
    aio_format_channel    channel;
    aio_format_range      range;
    aio_format_precision  precision;
    const char          * space;
  } hint;

  const char * exact;
};

typedef struct aio_image2d_s aio_image2d;
struct aio_image2d_s {
  aio_size_exact   * size_exact;
  aio_size_ratio   * size_ratio;
  aio_mips         * mips;
  const char       * unnormalized;
  long               array_len;
  aio_image_format * format;
  aio_init_from    * init_from;
};

typedef struct aio_image3d_s aio_image3d;
struct aio_image3d_s {
  struct {
    aio_uint width;
    aio_uint height;
    aio_uint depth;
  } size;

  aio_mips           mips;
  long               array_len;
  aio_image_format * format;
  aio_init_from    * init_from;
};

typedef struct aio_image_cube_s aio_image_cube;
struct aio_image_cube_s {
  struct {
    aio_uint width;
  } size;

  aio_mips           mips;
  long               array_len;
  aio_image_format * format;
  aio_init_from    * init_from;
};

typedef struct aio_image_s aio_image;
struct aio_image_s {
  aio_asset_base
  
  const char * id;
  const char * sid;
  const char * name;

  aio_init_from  * init_from;
  aio_image2d    * image2d;
  aio_image3d    * image3d;
  aio_image_cube * cube;
  aio_tree       * extra;

  aio_image * prev;
  aio_image * next;
  
  struct {
    aio_bool share;
  } renderable;

};

typedef struct aio_image_instance_s aio_image_instance;
struct aio_image_instance_s {
  const char * url;
  const char * sid;
  const char * name;

  aio_tree * extra;
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
#define _AIO_FX_SAMPLER_COMMON                                                \
  aio_asset_base                                                             \
  aio_image_instance * image;                                                 \
                                                                              \
  struct {                                                                    \
    const char * semantic;                                                    \
  } texcoord;                                                                 \
                                                                              \
  aio_wrap_mode wrap_s;                                                       \
  aio_wrap_mode wrap_t;                                                       \
  aio_wrap_mode wrap_p;                                                       \
                                                                              \
  aio_minfilter minfilter;                                                    \
  aio_magfilter magfilter;                                                    \
  aio_mipfilter mipfilter;                                                    \
                                                                              \
  unsigned long mip_max_level;                                                \
  unsigned long mip_min_level;                                                \
  float         mip_bias;                                                     \
  unsigned long max_anisotropy;                                               \
                                                                              \
  aio_color * border_color;                                                   \
  aio_tree  * extra;

typedef struct aio_fx_sampler_common_s aio_fx_sampler_common;
struct aio_fx_sampler_common_s {
  _AIO_FX_SAMPLER_COMMON
};

typedef struct aio_sampler1D_s aio_sampler1D;
struct aio_sampler1D_s {
  _AIO_FX_SAMPLER_COMMON
};

typedef struct aio_sampler2D_s aio_sampler2D;
struct aio_sampler2D_s {
  _AIO_FX_SAMPLER_COMMON
};

typedef struct aio_sampler3D_s aio_sampler3D;
struct aio_sampler3D_s {
  _AIO_FX_SAMPLER_COMMON
};

typedef struct aio_samplerCUBE_s aio_samplerCUBE;
struct aio_samplerCUBE_s {
  _AIO_FX_SAMPLER_COMMON
};

typedef struct aio_samplerDEPTH_s aio_samplerDEPTH;
struct aio_samplerDEPTH_s {
  _AIO_FX_SAMPLER_COMMON
};

typedef struct aio_samplerRECT_s aio_samplerRECT;
struct aio_samplerRECT_s {
  _AIO_FX_SAMPLER_COMMON
};

typedef struct aio_samplerStates_s aio_samplerStates;
struct aio_samplerStates_s {
  _AIO_FX_SAMPLER_COMMON
};

#undef _AIO_FX_SAMPLER_COMMON

typedef struct aio_fx_texture_s aio_fx_texture;
struct aio_fx_texture_s {
  const char * texture;
  const char * texcoord;
  aio_tree   * extra;
};

typedef struct aio_fx_color_or_tex_s aio_fx_color_or_tex;
struct aio_fx_color_or_tex_s {
  aio_opaque        opaque;
  aio_color       * color;
  aio_param_basic * param;
  aio_fx_texture  * texture;
};

typedef aio_fx_color_or_tex aio_ambient_fx;
typedef aio_fx_color_or_tex aio_diffuse;
typedef aio_fx_color_or_tex aio_emission;
typedef aio_fx_color_or_tex aio_reflective;
typedef aio_fx_color_or_tex aio_specular;
typedef aio_fx_color_or_tex aio_transparent;

typedef struct aio_fx_float_or_param_s aio_fx_float_or_param;
struct aio_fx_float_or_param_s {
  aio_basic_attrf * val;
  aio_param_basic * param;
};

typedef aio_fx_float_or_param aio_index_of_refraction;
typedef aio_fx_float_or_param aio_reflectivity;
typedef aio_fx_float_or_param aio_shininess;
typedef aio_fx_float_or_param aio_transparency;

typedef struct aio_annotate_s aio_annotate;
struct aio_annotate_s {
  const char     * name;
  void           * val;
  aio_value_type   val_type;

  aio_annotate * next;
  aio_annotate * prev;
};

typedef struct aio_newparam_s aio_newparam;
struct aio_newparam_s {
  const char     * sid;
  aio_annotate   * annotate;
  const char     * semantic;
  aio_modifier     modifier;
  void           * val;
  aio_value_type   val_type;

  aio_newparam * prev;
  aio_newparam * next;
};

typedef struct aio_code_s aio_code;
struct aio_code_s {
  const char * sid;
  const char * val;

  aio_code * prev;
  aio_code * next;
};

typedef struct aio_include_s aio_include;
struct aio_include_s {
  const char * sid;
  const char * url;

  aio_include * prev;
  aio_include * next;
};

typedef struct aio_blinn_s aio_blinn;
struct aio_blinn_s {
  aio_emission            * emission;
  aio_ambient_fx          * ambient;
  aio_diffuse             * diffuse;
  aio_specular            * specular;
  aio_shininess           * shininess;
  aio_reflective          * reflective;
  aio_reflectivity        * reflectivity;
  aio_transparent         * transparent;
  aio_transparency        * transparency;
  aio_index_of_refraction * index_of_refraction;
};

typedef struct aio_constant_fx_s aio_constant_fx;
struct aio_constant_fx_s {
  aio_emission            * emission;
  aio_reflective          * reflective;
  aio_reflectivity        * reflectivity;
  aio_transparent         * transparent;
  aio_transparency        * transparency;
  aio_index_of_refraction * index_of_refraction;
};

typedef struct aio_lambert_s aio_lambert;
struct aio_lambert_s {
  aio_emission            * emission;
  aio_ambient_fx          * ambient;
  aio_diffuse             * diffuse;
  aio_reflective          * reflective;
  aio_reflectivity        * reflectivity;
  aio_transparent         * transparent;
  aio_transparency        * transparency;
  aio_index_of_refraction * index_of_refraction;
};

typedef struct aio_phong_s aio_phong;
struct aio_phong_s {
  aio_emission            * emission;
  aio_ambient_fx          * ambient;
  aio_diffuse             * diffuse;
  aio_specular            * specular;
  aio_shininess           * shininess;
  aio_reflective          * reflective;
  aio_reflectivity        * reflectivity;
  aio_transparent         * transparent;
  aio_transparency        * transparency;
  aio_index_of_refraction * index_of_refraction;
};

typedef struct aio_render_state_s aio_render_state;
typedef struct aio_states_s aio_states;
struct aio_states_s {
  aio_render_state * next;
  long               count;
};

typedef struct aio_color_target_s aio_color_target;
struct aio_color_target {
  unsigned long index;
  unsigned long slice;
  unsigned long mip;
  aio_face      face;

  aio_param          * param;
  aio_image_instance * image_instance;
};

typedef struct aio_depth_target_s aio_depth_target;
struct aio_depth_target_s {
  unsigned long index;
  unsigned long slice;
  unsigned long mip;
  aio_face      face;

  aio_param          * param;
  aio_image_instance * image_instance;
};

typedef struct aio_stencil_target_s aio_stencil_target;
struct aio_stencil_target_s {
  unsigned long index;
  unsigned long slice;
  unsigned long mip;
  aio_face      face;

  aio_param          * param;
  aio_image_instance * image_instance;
};

typedef struct aio_color_clear_s aio_color_clear;
struct aio_color_clear_s {
  unsigned long index;
  aio_color     val;
};

typedef struct aio_depth_clear_s aio_depth_clear;
struct aio_depth_clear_s {
  unsigned long index;
  aio_float     val;
};

typedef struct aio_stencil_clear_s aio_stencil_clear;
struct aio_stencil_clear_s {
  unsigned long index;
  unsigned long val;
};

typedef struct aio_draw_s aio_draw;
struct aio_draw_s {
  aio_draw_enum enum_draw;
  const char    * str_val;
};

typedef struct aio_evaluate_s aio_evaluate;
struct aio_evaluate_s {
  aio_color_target   * color_target;
  aio_depth_target   * depth_target;
  aio_stencil_target * stencil_target;
  aio_color_clear    * color_clear;
  aio_depth_clear    * depth_clear;
  aio_stencil_clear  * stencil_clear;
  aio_draw             draw;
};

typedef struct aio_inline_s aio_inline;
struct aio_inline_s {
  const char * val;

  aio_inline * prev;
  aio_inline * next;
};

typedef struct aio_import_s aio_import;
struct aio_import_s {
  const char * ref;

  aio_import * prev;
  aio_import * next;
};

typedef struct aio_sources_s aio_sources;
struct aio_sources_s {
  const char * entry;

  aio_inline * inlines;
  aio_import * imports;
};

typedef struct aio_binary_s aio_binary;
struct aio_binary_s {
  const char * ref;
  aio_hex_data * hex;

  aio_binary * prev;
  aio_binary * next;
};

typedef struct aio_compiler_s aio_compiler;
struct aio_compiler_s {
  const char * platform;
  const char * target;
  const char * options;

  aio_binary * binary;
  const char * text;

  aio_compiler * prev;
  aio_compiler * next;
};

typedef struct aio_bind_uniform_s aio_bind_uniform;
struct aio_bind_uniform_s {
  const char * symbol;

  aio_param      * param;
  void           * val;
  aio_value_type   val_type;

  aio_bind_uniform * prev;
  aio_bind_uniform * next;
};

typedef struct aio_bind_attrib_s aio_bind_attrib;
struct aio_bind_attrib_s {
  const char * symbol;
  const char * semantic;

  aio_bind_uniform * prev;
  aio_bind_uniform * next;
};

typedef struct aio_shader_s aio_shader;
struct aio_shader_s {
  aio_pipeline_stage stage;

  aio_sources      * sources;
  aio_compiler     * compiler;
  aio_bind_uniform * bind_uniform;
  aio_tree         * extra;

  aio_shader * prev;
  aio_shader * next;
};

typedef struct aio_linker_s aio_linker;
struct aio_linker_s {
  const char * platform;
  const char * target;
  const char * options;

  aio_binary * binary;

  aio_linker * prev;
  aio_linker * next;
};

typedef struct aio_program_s aio_program;
struct aio_program_s {
  aio_shader       * shader;
  aio_bind_attrib  * bind_attrib;
  aio_bind_uniform * bind_uniform;
  aio_linker       * linker;
};

typedef struct aio_pass_s aio_pass;
struct aio_pass_s {
  aio_asset_base

  const char * sid;

  aio_annotate     * annotate;
  aio_states       * states;
  aio_evaluate     * evaluate;
  aio_program      * program;
  aio_tree         * extra;

  aio_pass * prev;
  aio_pass * next;
};

typedef struct aio_technique_fx_s aio_technique_fx;
struct aio_technique_fx_s {
  aio_asset_base

  const char * id;
  const char * sid;

  aio_annotate    * annotate;
  aio_blinn       * blinn;
  aio_constant_fx * constant;
  aio_lambert     * lambert;
  aio_phong       * phong;
  aio_pass        * pass;

  aio_tree        * extra;

  aio_technique_fx * prev;
  aio_technique_fx * next;
};

#ifdef _AIO_PROFILE_BASE_
#  undef _AIO_PROFILE_BASE_
#endif

/*
 * Common properties of all profiles
 */
#define _AIO_PROFILE_BASE_                                                    \
  aio_asset_base                                                             \
  aio_profile_type   profile_type;                                            \
  const char       * id;                                                      \
  aio_newparam     * newparam;                                                \
  aio_technique_fx * technique;                                               \
  aio_tree         * extra;                                                   \
  aio_profile      * prev;                                                    \
  aio_profile      * next;

typedef struct aio_profile_s aio_profile;
struct aio_profile_s {
  _AIO_PROFILE_BASE_
};

typedef struct aio_profile_common_s aio_profile_common;
struct aio_profile_common_s {
  _AIO_PROFILE_BASE_
};

typedef struct aio_profile_cg_s aio_profile_CG;
struct aio_profile_cg_s {
  _AIO_PROFILE_BASE_

  const char    * platform;
  aio_code      * code;
  aio_include   * include;
};

typedef struct aio_profile_gles_s aio_profile_GLES;
struct aio_profile_gles_s {
  _AIO_PROFILE_BASE_

  const char * platform;
};

typedef struct aio_profile_gles2_s aio_profile_GLES2;
struct aio_profile_gles2_s {
  _AIO_PROFILE_BASE_

  const char  * language;
  aio_code    * code;
  aio_include * include;
  const char  * platforms;
};

typedef struct aio_profile_glsl_s aio_profile_GLSL;
struct aio_profile_glsl_s {
  _AIO_PROFILE_BASE_

  const char  * platform;
  aio_code    * code;
  aio_include * include;
};

typedef struct aio_profile_bridge_s aio_profile_BRIDGE;
struct aio_profile_bridge_s {
  _AIO_PROFILE_BASE_
  
  const char * platform;
  const char * url;
};

#undef _AIO_PROFILE_BASE_

typedef struct aio_effect_s aio_effect;
struct aio_effect_s {
  aio_asset_base

  const char * id;
  const char * name;

  aio_annotate * annotate;
  aio_newparam * newparam;
  aio_profile  * profile;

  aio_tree   * extra;

  aio_effect * prev;
  aio_effect * next;
};

#pragma mark -
#pragma mark define library

#undef _AIO_DEF_LIB

#define _AIO_DEF_LIB(T)                                                       \
  typedef struct aio_lib_ ## T ## _s  aio_lib_ ## T;                          \
  struct aio_lib_ ## T ## _s {                                                \
    aio_assetinf  * inf;                                                      \
    const char    * id;                                                       \
    const char    * name;                                                     \
    aio_##T       * next;                                                     \
    aio_tree      * extra;                                                    \
    unsigned long   count;                                                    \
  }

_AIO_DEF_LIB(camera);
_AIO_DEF_LIB(light);
_AIO_DEF_LIB(effect);
_AIO_DEF_LIB(image);

#undef _AIO_DEF_LIB

typedef struct aio_lib_s aio_lib;
struct aio_lib_s {
  aio_lib_camera cameras;
  aio_lib_light  lights;
  aio_lib_effect effects;
  aio_lib_image  images;
};

#pragma mark -

typedef struct aio_doc_s aio_doc;
struct aio_doc_s {
  aio_docinf docinf;
  aio_lib    lib;
};

typedef struct aio_asset_s aio_asset;
struct aio_asset_s {
  aio_asset_base
  void * aio_data;
};

#include "assetio-states.h"
#include "assetio-string.h"

extern void aio_cleanup();

int aio_load(aio_doc ** __restrict dest,
             const char * __restrict file, ...);

#ifdef __cplusplus
}
#endif
#endif /* __libassetio__assetio__h_ */
