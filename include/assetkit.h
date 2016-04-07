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
#include <stdint.h>

#ifdef __cplusplus
//extern "C" {
#endif

#if defined(_WIN32)
#  ifdef _assetkit_dll
#    define AK_EXPORT __declspec(dllexport)
#  else
#    define AK_EXPORT __declspec(dllimport)
#  endif
#  define _assetkit_hide
#else
#  define AK_EXPORT      __attribute__((visibility("default")))
#  define _assetkit_hide __attribute__((visibility("hidden")))
#endif

#define AK_ARRAY_LEN(ARR) sizeof(ARR) / sizeof(ARR[0]);

typedef int32_t      AkEnum;
typedef const char * AkString;
typedef char       * AkMutString;
typedef bool         AkBool;
typedef int32_t      AkInt;
typedef uint32_t     AkUInt;
typedef int64_t      AkInt64;
typedef uint64_t     AkUInt64;
typedef float        AkFloat;
typedef double       AkDouble;

typedef AkBool       AkBool4[4];
typedef AkInt        AkInt2[2];
typedef AkInt        AkInt4[4];
typedef AkFloat      AkFloat2[2];
typedef AkFloat      AkFloat3[3];
typedef AkFloat      AkFloat4[4];
typedef AkDouble     AkDouble2[2];
typedef AkDouble     AkDouble3[3];
typedef AkDouble     AkDouble4[4];

typedef AkDouble     AkDouble4x4[4];
typedef AkFloat4     AkFloat4x4[4];

#undef AK__DEF_ARRAY

#define AK__DEF_ARRAY(TYPE)                                                   \
  typedef struct TYPE##Array {                                                \
    size_t count;                                                             \
    TYPE   items[];                                                           \
  } TYPE##Array;                                                              \
                                                                              \
  typedef struct TYPE##ArrayL {                                               \
    struct TYPE##ArrayL * next;                                               \
    size_t count;                                                             \
    TYPE   items[];                                                           \
  } TYPE##ArrayL

AK__DEF_ARRAY(AkBool);
AK__DEF_ARRAY(AkInt);
AK__DEF_ARRAY(AkUInt);
AK__DEF_ARRAY(AkFloat);
AK__DEF_ARRAY(AkDouble);
AK__DEF_ARRAY(AkString);

#undef AK__DEF_ARRAY

/* End Core Value Types */

#include "ak-memory.h"

/**
 * @brief library time type
 */
typedef time_t ak_time_t;

typedef enum AkResult {
  AK_ERR = -1,
  AK_OK  = 0
} AkResult;

typedef enum AkValueType {
  AK_VALUE_TYPE_UNKNOWN  = 0,
  AK_VALUE_TYPE_BOOL     = 1,
  AK_VALUE_TYPE_BOOL2    = 2,
  AK_VALUE_TYPE_BOOL3    = 3,
  AK_VALUE_TYPE_BOOL4    = 4,
  AK_VALUE_TYPE_INT      = 5,
  AK_VALUE_TYPE_INT2     = 6,
  AK_VALUE_TYPE_INT3     = 7,
  AK_VALUE_TYPE_INT4     = 8,
  AK_VALUE_TYPE_FLOAT    = 9,
  AK_VALUE_TYPE_FLOAT2   = 10,
  AK_VALUE_TYPE_FLOAT3   = 11,
  AK_VALUE_TYPE_FLOAT4   = 12,
  AK_VALUE_TYPE_FLOAT2x2 = 13,
  AK_VALUE_TYPE_FLOAT3x3 = 14,
  AK_VALUE_TYPE_FLOAT4x4 = 15,
  AK_VALUE_TYPE_STRING   = 16
} AkValueType;

typedef enum AkModifier {
  AK_MODIFIER_CONST    = 1,
  AK_MODIFIER_UNIFORM  = 2,
  AK_MODIFIER_VARYING  = 3,
  AK_MODIFIER_STATIC   = 4,
  AK_MODIFIER_VOLATILE = 5,
  AK_MODIFIER_EXTERN   = 6,
  AK_MODIFIER_SHARED   = 7
} AkModifier;

typedef enum AkProfileType {
  AK_PROFILE_TYPE_UNKOWN = 0,
  AK_PROFILE_TYPE_COMMON = 1,
  AK_PROFILE_TYPE_CG     = 2,
  AK_PROFILE_TYPE_GLES   = 3,
  AK_PROFILE_TYPE_GLES2  = 4,
  AK_PROFILE_TYPE_GLSL   = 5,
  AK_PROFILE_TYPE_BRIDGE = 6
} AkProfileType;

typedef enum AkTechniqueCommonType {
  AK_TECHNIQUE_COMMON_CAMERA_PERSPECTIVE  = 1,
  AK_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC = 2,
  AK_TECHNIQUE_COMMON_LIGHT_AMBIENT       = 3,
  AK_TECHNIQUE_COMMON_LIGHT_DIRECTIONAL   = 4,
  AK_TECHNIQUE_COMMON_LIGHT_POINT         = 5,
  AK_TECHNIQUE_COMMON_LIGHT_SPOT          = 6
} AkTechniqueCommonType;

typedef enum AkFileType {
  AK_FILE_TYPE_AUTO      = 0,
  AK_FILE_TYPE_COLLADA   = 1,
  AK_FILE_TYPE_WAVEFRONT = 2,
  AK_FILE_TYPE_FBX       = 3
} AkFileType;

typedef enum AkAssetType {
  AK_ASSET_TYPE_UNKNOWN
} AkAssetType;

typedef enum AkUpAxis {
  AK_UP_AXIS_Y = 0,
  AK_UP_AXIS_X = 1,
  AK_UP_AXIS_Z = 2
} AkUpAxis;

typedef enum AkAltitudeMode {
  AK_ALTITUDE_RELATIVETOGROUND = 0,
  AK_ALTITUDE_ABSOLUTE         = 1
} AkAltitudeMode;

typedef enum AkOpaque {
  AK_OPAQUE_A_ONE    = 0,
  AK_OPAQUE_RGB_ZERO = 1,
  AK_OPAQUE_A_ZERO   = 2,
  AK_OPAQUE_RGB_ONE  = 3
} AkOpaque;

typedef enum AkParamType {
  AK_PARAM_TYPE_BASIC    = 0,
  AK_PARAM_TYPE_EXTENDED = 1
} AkParamType;

typedef enum AkWrapMode {
  AK_WRAP_MODE_WRAP        = 0,
  AK_WRAP_MODE_MIRROR      = 1,
  AK_WRAP_MODE_CLAMP       = 2,
  AK_WRAP_MODE_BORDER      = 3,
  AK_WRAP_MODE_MIRROR_ONCE = 4
} AkWrapMode;

typedef enum AkMinFilter {
  AK_MINFILTER_LINEAR      = 0,
  AK_MINFILTER_NEAREST     = 1,
  AK_MINFILTER_ANISOTROPIC = 2
} AkMinFilter;

typedef enum AkMagFilter {
  AK_MAGFILTER_LINEAR  = 0,
  AK_MAGFILTER_NEAREST = 1
} AkMagFilter;

typedef enum AkMipFilter {
  AK_MIPFILTER_LINEAR  = 0,
  AK_MIPFILTER_NONE    = 1,
  AK_MIPFILTER_NEAREST = 2
} AkMipFilter;

typedef enum AkFace {
  AK_FACE_POSITIVE_X = 1,
  AK_FACE_NEGATIVE_X = 2,
  AK_FACE_POSITIVE_Y = 3,
  AK_FACE_NEGATIVE_Y = 4,
  AK_FACE_POSITIVE_Z = 5,
  AK_FACE_NEGATIVE_Z = 6
} AkFace;

typedef enum AkChannelFormat {
  AK_CHANNEL_FORMAT_RGB  = 1,
  AK_CHANNEL_FORMAT_RGBA = 2,
  AK_CHANNEL_FORMAT_RGBE = 3,
  AK_CHANNEL_FORMAT_L    = 4,
  AK_CHANNEL_FORMAT_LA   = 5,
  AK_CHANNEL_FORMAT_D    = 6
} AkChannelFormat;

typedef enum AkRangeFormat {
  AK_RANGE_FORMAT_SNORM = 1,
  AK_RANGE_FORMAT_UNORM = 2,
  AK_RANGE_FORMAT_SINT  = 3,
  AK_RANGE_FORMAT_UINT  = 4,
  AK_RANGE_FORMAT_FLOAT = 5
} AkRangeFormat;

typedef enum AkPrecisionFormat {
  AK_PRECISION_FORMAT_DEFAULT = 1,
  AK_PRECISION_FORMAT_LOW     = 2,
  AK_PRECISION_FORMAT_MID     = 3,
  AK_PRECISION_FORMAT_HIGHT   = 4,
  AK_PRECISION_FORMAT_MAX     = 5
} AkPrecisionFormat;

typedef enum AkDrawType {
  AK_DRAW_READ_STR_VAL                     = 0,
  AK_DRAW_GEOMETRY                         = 1,
  AK_DRAW_SCENE_GEOMETRY                   = 2,
  AK_DRAW_SCENE_IMAGE                      = 3,
  AK_DRAW_FULL_SCREEN_QUAD                 = 4,
  AK_DRAW_FULL_SCREEN_QUAD_PLUS_HALF_PIXEL = 5
} AkDrawType;

typedef enum AkPipelineStage {
  AK_PIPELINE_STAGE_VERTEX      = 1,
  AK_PIPELINE_STAGE_FRAGMENT    = 2,
  AK_PIPELINE_STAGE_TESSELATION = 3,
  AK_PIPELINE_STAGE_GEOMETRY    = 4
} AkPipelineStage;

typedef enum AkInputSemantic {
  /* read semanticRaw */
  AK_INPUT_SEMANTIC_OTHER           = 0,
  AK_INPUT_SEMANTIC_BINORMAL        = 1,
  AK_INPUT_SEMANTIC_COLOR           = 2,
  AK_INPUT_SEMANTIC_CONTINUITY      = 3,
  AK_INPUT_SEMANTIC_IMAGE           = 4,
  AK_INPUT_SEMANTIC_INPUT           = 5,
  AK_INPUT_SEMANTIC_IN_TANGENT      = 6,
  AK_INPUT_SEMANTIC_INTERPOLATION   = 7,
  AK_INPUT_SEMANTIC_INV_BIND_MATRIX = 8,
  AK_INPUT_SEMANTIC_JOINT           = 9,
  AK_INPUT_SEMANTIC_LINEAR_STEPS    = 10,
  AK_INPUT_SEMANTIC_MORPH_TARGET    = 11,
  AK_INPUT_SEMANTIC_MORPH_WEIGHT    = 12,
  AK_INPUT_SEMANTIC_NORMAL          = 13,
  AK_INPUT_SEMANTIC_OUTPUT          = 14,
  AK_INPUT_SEMANTIC_OUT_TANGENT     = 15,
  AK_INPUT_SEMANTIC_POSITION        = 16,
  AK_INPUT_SEMANTIC_TANGENT         = 17,
  AK_INPUT_SEMANTIC_TEXBINORMAL     = 18,
  AK_INPUT_SEMANTIC_TEXCOORD        = 19,
  AK_INPUT_SEMANTIC_TEXTANGENT      = 20,
  AK_INPUT_SEMANTIC_UV              = 21,
  AK_INPUT_SEMANTIC_VERTEX          = 22,
  AK_INPUT_SEMANTIC_WEIGHT          = 23
} AkInputSemantic;

typedef enum AkTriangleMode {
  AK_TRIANGLE_MODE_TRIANGLES      = 0,
  AK_TRIANGLE_MODE_TRIANGLE_STRIP = 1,
  AK_TRIANGLE_MODE_TRIANGLE_FAN   = 2
} AkTriangleMode;

typedef enum AkLineMode {
  AK_LINE_MODE_LINES      = 0,
  AK_LINE_MODE_LINE_LOOP  = 1,
  AK_LINE_MODE_LINE_STRIP = 2
} AkLineMode;

typedef enum AkPolygonMode {
  AK_POLYGON_MODE_POLYLIST = 0,
  AK_POLYGON_MODE_POLYGONS = 1
} AkPolygonMode;

typedef enum AkMeshPrimitiveType {
  AK_MESH_PRIMITIVE_TYPE_LINES      = 1,
  AK_MESH_PRIMITIVE_TYPE_POLYGONS   = 2,
  AK_MESH_PRIMITIVE_TYPE_TRIANGLES  = 3
} AkMeshPrimitiveType;

typedef enum AkCurveElementType {
  AK_CURVE_ELEMENT_TYPE_LINE      = 1,
  AK_CURVE_ELEMENT_TYPE_CIRCLE    = 2,
  AK_CURVE_ELEMENT_TYPE_ELLIPSE   = 3,
  AK_CURVE_ELEMENT_TYPE_PARABOLA  = 4,
  AK_CURVE_ELEMENT_TYPE_HYPERBOLA = 5,
  AK_CURVE_ELEMENT_TYPE_NURBS     = 6,
} AkCurveElementType;

typedef enum AkSurfaceElementType {
  AK_SURFACE_ELEMENT_TYPE_CONE          = 1,
  AK_SURFACE_ELEMENT_TYPE_PLANE         = 2,
  AK_SURFACE_ELEMENT_TYPE_CYLINDER      = 3,
  AK_SURFACE_ELEMENT_TYPE_NURBS_SURFACE = 4,
  AK_SURFACE_ELEMENT_TYPE_SPHERE        = 5,
  AK_SURFACE_ELEMENT_TYPE_TORUS         = 6,
  AK_SURFACE_ELEMENT_TYPE_SWEPT_SURFACE = 7
} AkSurfaceElementType;

typedef enum AkSourceArrayType {
  AK_SOURCE_ARRAY_TYPE_BOOL   = 1,
  AK_SOURCE_ARRAY_TYPE_FLOAT  = 2,
  AK_SOURCE_ARRAY_TYPE_INT    = 3,
  AK_SOURCE_ARRAY_TYPE_IDREF  = 4,
  AK_SOURCE_ARRAY_TYPE_NAME   = 5,
  AK_SOURCE_ARRAY_TYPE_SIDREF = 6,
  AK_SOURCE_ARRAY_TYPE_TOKEN  = 7
} AkSourceArrayType;

typedef enum AkMorphMethod {
  AK_MORPH_METHOD_NORMALIZED = 1,
  AK_MORPH_METHOD_RELATIVE   = 2
} AkMorphMethod;

typedef enum AkNodeType {
  AK_NODE_TYPE_NODE  = 1,
  AK_NODE_TYPE_JOINT = 2
} AkNodeType;

typedef enum AkNodeTransformType {
  AK_NODE_TRANSFORM_TYPE_LOOK_AT   = 1,
  AK_NODE_TRANSFORM_TYPE_MATRIX    = 2,
  AK_NODE_TRANSFORM_TYPE_ROTATE    = 3,
  AK_NODE_TRANSFORM_TYPE_SCALE     = 4,
  AK_NODE_TRANSFORM_TYPE_SKEW      = 5,
  AK_NODE_TRANSFORM_TYPE_TRANSLATE = 6
} AkNodeTransformType;

/**
 * Almost all assets includes this fields.
 * This macro defines base fields of assets
 */
#define ak_asset_base                                                        \
  ak_assetinf  * inf;                                                        \
  AkAssetType   type;

typedef struct AkTreeNodeAttr {
  const char * name;
  char       * val;

  struct AkTreeNodeAttr * next;
  struct AkTreeNodeAttr * prev;
} AkTreeNodeAttr;

typedef struct AkTreeNode {
  const char    * name;
  char          * val;
  unsigned long   attrc;
  unsigned long   chldc;

  AkTreeNodeAttr * attr;

  struct AkTreeNode * chld;
  struct AkTreeNode * parent;
  struct AkTreeNode * next;
  struct AkTreeNode * prev;
} AkTreeNode;

typedef struct AkTreeNode AkTree;

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
    AkFloat4 vec;

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
  AkAltitudeMode mode;
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
  AkTree        * extra;
  ak_time_t        created;
  ak_time_t        modified;
  unsigned long     revision;
  AkUpAxis        upaxis;
};

typedef struct ak_docinf_s ak_docinf;
struct ak_docinf_s {
  ak_assetinf   base;
  const char   * fname;
  AkFileType   ftype;
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
  AkTree   * chld;

  ak_technique * prev;
  ak_technique * next;
};

typedef struct ak_technique_common_s ak_technique_common;
struct ak_technique_common_s {
  ak_technique_common      * next;
  void                      * technique;
  AkTechniqueCommonType   technique_type;
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
  AkTree      * extra;
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
  AkTree   * extra;

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
  AkTree             * extra;

  ak_light * next;
};

/* FX */
/* Effects */
/*
 * base type of param
 */
typedef struct ak_param_s ak_param;
struct ak_param_s {
  AkParamType  param_type;
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
  AkValueType   type;
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

  AkFace     face;
  AkUInt     mip_index;
  AkUInt     depth;
  AkInt      array_index;
  AkBool     mips_generate;

  ak_init_from * prev;
  ak_init_from * next;
};

typedef struct ak_size_exact_s ak_size_exact;
struct ak_size_exact_s {
  AkFloat width;
  AkFloat height;
};

typedef struct ak_size_ratio_s ak_size_ratio;
struct ak_size_ratio_s {
  AkFloat width;
  AkFloat height;
};

typedef struct ak_mips_s ak_mips;
struct ak_mips_s {
  AkUInt levels;
  AkBool auto_generate;
};

typedef struct ak_image_format_s ak_image_format;
struct ak_image_format_s {
  struct {
    AkChannelFormat    channel;
    AkRangeFormat      range;
    AkPrecisionFormat  precision;
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
    AkUInt width;
    AkUInt height;
    AkUInt depth;
  } size;

  ak_mips           mips;
  long               array_len;
  ak_image_format * format;
  ak_init_from    * init_from;
};

typedef struct ak_image_cube_s ak_image_cube;
struct ak_image_cube_s {
  struct {
    AkUInt width;
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
  AkTree       * extra;

  ak_image * prev;
  ak_image * next;

  struct {
    AkBool share;
  } renderable;

};

typedef struct ak_image_instance_s ak_image_instance;
struct ak_image_instance_s {
  const char * url;
  const char * sid;
  const char * name;

  AkTree * extra;
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

  AkWrapMode wrap_s;
  AkWrapMode wrap_t;
  AkWrapMode wrap_p;

  AkMinFilter minfilter;
  AkMagFilter magfilter;
  AkMipFilter mipfilter;

  unsigned long mip_max_level;
  unsigned long mip_min_level;
  float         mip_bias;
  unsigned long max_anisotropy;

  ak_color * border_color;
  AkTree  * extra;
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
  AkTree   * extra;
};

typedef struct ak_fx_color_or_tex_s ak_fx_color_or_tex;
struct ak_fx_color_or_tex_s {
  AkOpaque       opaque;
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
  AkValueType   val_type;

  ak_annotate * next;
};

typedef struct ak_newparam_s ak_newparam;
struct ak_newparam_s {
  const char     * sid;
  ak_annotate   * annotate;
  const char     * semantic;
  AkModifier     modifier;
  void           * val;
  AkValueType   val_type;

  ak_newparam * next;
};

typedef struct ak_setparam_s ak_setparam;
struct ak_setparam_s {
  const char     * ref;
  void           * val;
  AkValueType   val_type;

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

struct AkRenderState;
typedef struct ak_states_s ak_states;
struct ak_states_s {
  struct AkRenderState * next;
  long               count;
};

typedef struct ak_evaluate_target_s ak_evaluate_target;
struct ak_evaluate_target_s {
  unsigned long index;
  unsigned long slice;
  unsigned long mip;
  AkFace      face;

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
  AkFloat     val;
};

typedef struct ak_stencil_clear_s ak_stencil_clear;
struct ak_stencil_clear_s {
  unsigned long index;
  unsigned long val;
};

typedef struct AK_DRAW_s ak_draw;
struct AK_DRAW_s {
  AkDrawType enum_draw;
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
  AkValueType    val_type;

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
  AkPipelineStage stage;

  ak_sources      * sources;
  ak_compiler     * compiler;
  ak_bind_uniform * bind_uniform;
  AkTree         * extra;

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
  AkTree         * extra;

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

  AkTree        * extra;

  ak_technique_fx * next;
};

typedef struct ak_profile_s ak_profile;
struct ak_profile_s {
  ak_asset_base
  AkProfileType   profile_type;
  const char       * id;
  ak_newparam     * newparam;
  ak_technique_fx * technique;
  AkTree         * extra;
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

  AkTree   * extra;

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

  AkTree * extra;
};

typedef struct ak_material_s ak_material;
struct ak_material_s {
  ak_asset_base
  const char * id;
  const char * name;
  ak_effect_instance *effect_inst;
  AkTree * extra;

  ak_material * next;
};

typedef struct AkBoolArrayN {
  const char * id;
  const char * name;
  size_t       count;
  AkBool       items[];
} AkBoolArrayN;

typedef struct AkFloatArrayN {
  const char * id;
  const char * name;
  size_t       count;
  AkUInt       digits;
  AkUInt       magnitude;
  AkFloat      items[];
} AkFloatArrayN;

typedef struct AkIntArrayN {
  const char * id;
  const char * name;
  size_t       count;
  AkInt        minInclusive;
  AkInt        maxInclusive;
  AkInt        items[];
} AkIntArrayN;

typedef struct AkStringArrayN {
  const char  * id;
  const char  * name;
  size_t count;
  AkString items[];
} AkStringArrayN;

typedef struct AkSource {
  ak_asset_base

  const char * id;
  const char * name;
  AkObject   * data;

  ak_technique_common * techniqueCommon;
  ak_technique        * technique;

  struct AkSource * next;
} AkSource;

typedef struct AkInputBasic {
  ak_asset_base

  const char * source;
  const char * semanticRaw;
  AkInputSemantic semantic;

  struct AkInputBasic * next;
} AkInputBasic;

typedef struct AkInput {
  AkInputBasic base;

  uint32_t offset;
  uint32_t set;
} AkInput;

typedef struct AkVertices {
  ak_asset_base

  const char   * id;
  const char   * name;
  AkInputBasic * input;
  AkTree       * extra;
} AkVertices;

typedef struct AkLines {
  ak_asset_base

  const char * name;
  const char * material;
  uint64_t     count;

  AkInput    * input;
  AkTree     * extra;
  AkLineMode   mode;

  AkDoubleArrayL * primitives;
} AkLines;

typedef struct AkPolygon {
  struct AkPolygon * next;

  AkDoubleArray  * primitives;
  AkDoubleArrayL * holes;
  AkIntArray     * vcount;
  AkPolygonMode    mode;
  AkBool           haveHoles;
} AkPolygon;

typedef struct AkPolygons {
  ak_asset_base

  const char  * name;
  const char  * material;
  uint64_t      count;

  AkInput     * input;
  AkPolygon   * polygon;
  AkTree      * extra;
} AkPolygons;

typedef struct AkTriangles {
  ak_asset_base

  const char     * name;
  const char     * material;
  AkInput        * input;
  AkDoubleArrayL * primitives;
  AkTree         * extra;
  uint64_t         count;
  AkTriangleMode   mode;
} AkTriangles;

typedef struct AkMesh {
  ak_asset_base

  const char * convexHullOf;
  AkSource   * source;
  AkVertices * vertices;
  AkObject   * gprimitive;
  AkTree     * extra;
} AkMesh;

typedef struct AkControlVerts {
  ak_asset_base

  AkInputBasic * input;
  AkTree       * extra;
} AkControlVerts;

typedef struct AkSpline {
  ak_asset_base

  AkSource       * source;
  AkControlVerts * cverts;
  AkTree         * extra;
  AkBool           closed;
} AkSpline;

typedef struct AkLine {
  AkDouble3 origin;
  AkDouble3 direction;
  AkTree * extra;
} AkLine;

typedef struct AkCircle {
  AkFloat  radius;
  AkTree * extra;
} AkCircle;

typedef struct AkEllipse {
  AkFloat2 radius;
  AkTree * extra;
} AkEllipse;

typedef struct AkParabola {
  AkFloat  focal;
  AkTree * extra;
} AkParabola;

typedef struct AkHyperbola {
  AkFloat2 radius;
  AkTree * extra;
} AkHyperbola;

typedef struct AkNurbs {
  AkSource       * source;
  AkControlVerts * cverts;
  AkTree         * extra;
  AkUInt           degree;
  AkBool           closed;
} AkNurbs;

typedef struct AkCurve {
  AkDoubleArrayL * orient;
  AkDouble3        origin;
  AkObject       * curve;
  struct AkCurve * next;
} AkCurve;

typedef struct AkCurves {
  AkCurve * curve;
  AkTree  * extra;
} AkCurves;

typedef struct AkCone {
  AkFloat  radius;
  AkFloat  angle;
  AkTree * extra;
} AkCone;

typedef struct AkPlane {
  AkDouble4 equation;
  AkTree * extra;
} AkPlane;

typedef struct AkCylinder {
  AkFloat2 radius;
  AkTree * extra;
} AkCylinder;

typedef struct AkNurbsSurface {
  AkUInt degree_u;
  AkBool closed_u;
  AkUInt degree_v;
  AkBool closed_v;

  AkSource       * source;
  AkControlVerts * cverts;
  AkTree         * extra;
} AkNurbsSurface;

typedef struct AkSphere {
  AkFloat  radius;
  AkTree * extra;
} AkSphere;

typedef struct AkTorus {
  AkFloat2 radius;
  AkTree * extra;
} AkTorus;

typedef struct AkSweptSurface {
  AkCurve * curve;
  AkFloat3  direction;
  AkFloat3  origin;
  AkFloat3  axis;
  AkTree  * extra;
} AkSweptSurface;

typedef struct AkSurface {
  const char * sid;
  const char * name;

  AkObject      * surface;
  AkFloatArrayL * orient;
  AkFloat3        origin;
} AkSurface;

typedef struct AkSurfaces {
  AkSurface * surface;
  AkTree    * extra;
} AkSurfaces;

typedef struct AkEdges {
  const char    * id;
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkEdges;

typedef struct AkWires {
  const char    * id;
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkWires;

typedef struct AkFaces {
  const char    * id;
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkFaces;

typedef struct AkPCurves {
  const char    * id;
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkPCurves;

typedef struct AkShells {
  const char    * id;
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkShells;

typedef struct AkSolids {
  const char    * id;
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkSolids;

typedef struct AkBoundryRep {
  ak_asset_base
  AkCurves   * curves;
  AkCurves   * surfaceCurves;
  AkSurfaces * surfaces;
  AkSource   * source;
  AkVertices * vertices;
  AkEdges    * edges;
  AkWires    * wires;
  AkFaces    * faces;
  AkPCurves  * pcurves;
  AkShells   * shells;
  AkSolids   * solids;
  AkTree     * extra;
} AkBoundryRep;

typedef struct AkGeometry {
  ak_asset_base

  const char * id;
  const char * name;
  AkObject   * gdata;
  AkTree     * extra;

  struct AkGeometry * next;
} AkGeometry;

typedef struct AkJoints {
  ak_asset_base

  AkInputBasic * input;
  AkTree       * extra;
} AkJoints;

typedef struct AkVertexWeights {
  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * v;
  AkTree        * extra;
  AkUInt          count;
} AkVertexWeights;

typedef struct AkSkin {
  ak_asset_base

  const char      * baseMesh;
  AkDoubleArray   * bindShapeMatrix;
  AkSource        * source;
  AkJoints        * joints;
  AkVertexWeights * vertexWeights;
  AkTree          * extra;
} AkSkin;

typedef struct AkTargets {
  ak_asset_base

  AkInputBasic * input;
  AkTree       * extra;
} AkTargets;

typedef struct AkMorph {
  ak_asset_base

  const char *  baseMesh;
  AkMorphMethod method;

  AkSource    * source;
  AkTargets   * targets;
  AkTree      * extra;
} AkMorph;

typedef struct AkController {
  ak_asset_base

  const char * id;
  const char * name;
  AkObject   * data;
  AkTree     * extra;

  struct AkController * next;
} AkController;

typedef struct AkLookAt {
  ak_asset_base

  const char * sid;
  AkDouble     val[9];
} AkLookAt;

typedef struct AkMatrix {
  ak_asset_base

  const char * sid;
  AkDouble     val[4][4];
} AkMatrix;

typedef struct AkRotate {
  ak_asset_base

  const char * sid;
  AkDouble     val[4];
} AkRotate;

typedef struct AkScale {
  ak_asset_base

  const char * sid;
  AkDouble     val[3];
} AkScale;

typedef struct AkSkew {
  ak_asset_base

  const char * sid;
  AkDouble     val[7];
} AkSkew;

typedef struct AkTranslate {
  ak_asset_base

  const char * sid;
  AkDouble     val[3];
} AkTranslate;

typedef struct AkInstanceCamera {
  const char * id;
  const char * name;
  const char * url;
  AkTree     * extra;
} AkInstanceCamera;

typedef struct AkSkeleton {
  const char * val;

  struct AkSkeleton * next;
} AkSkeleton;

typedef struct AkBindMaterial {
  ak_param     * param;
  ak_technique_common * techniqueCommon;
  ak_technique * technique;
  AkTree       * extra;
} AkBindMaterial;

typedef struct AkInstanceController {
  const char     * id;
  const char     * name;
  const char     * url;
  AkSkeleton     * skeleton;
  AkBindMaterial * bindMaterial;
  AkTree         * extra;
} AkInstanceController;

typedef struct AkInstanceGeometry {
  const char     * id;
  const char     * name;
  const char     * url;
  AkBindMaterial * bindMaterial;
  AkTree         * extra;
} AkInstanceGeometry;

typedef struct AkInstanceLight {
  const char     * id;
  const char     * name;
  const char     * url;
  const char     * proxy;
  AkTree         * extra;
} AkInstanceLight;

typedef struct AkInstanceNode {
  const char     * id;
  const char     * name;
  const char     * url;
  AkTree         * extra;
} AkInstanceNode;

typedef struct AkNode {
  ak_asset_base

  const char * id;
  const char * name;
  const char * sid;
  AkNodeType   nodeType;
  AkStringArray        * layer;
  AkObject             * transform;
  AkInstanceCamera     * camera;
  AkInstanceController * controller;
  AkInstanceGeometry   * geometry;
  AkInstanceLight      * light;
  AkTree        * extra;
  struct AkNode * chld;
  struct AkNode * next;
} AkNode;

typedef struct AkTechniqueOverride {
  const char * ref;
  const char * pass;
} AkTechniqueOverride;

typedef struct AkBind {
  const char    * semantic;
  const char    * target;
  struct AkBind * next;
} AkBind;

typedef struct AkBindVertexInput {
  const char * semantic;
  const char * inputSemantic;
  AkUInt       inputSet;
  struct AkBindVertexInput * next;
} AkBindVertexInput;

typedef struct AkInstanceMaterial {
  ak_asset_base

  const char          * url;
  AkTechniqueOverride * techniqueOverride;
  AkBind              * bind;
  AkBindVertexInput   * bindVertexInput;
  AkTree              * extra;

  struct AkRender * next;
} AkInstanceMaterial;

typedef struct AkRender {
  ak_asset_base

  const char    * name;
  const char    * sid;
  const char    * cameraMode;
  AkStringArray * layer;
  AkInstanceMaterial * instanceMaterial;
  AkTree        * extra;

  struct AkRender * next;
} AkRender;

typedef struct AkEvaluateScene {
  ak_asset_base

  AkRender * render;
  AkTree   * extra;

  struct AkEvaluateScene * next;
} AkEvaluateScene;

typedef struct AkVisualScene {
  ak_asset_base

  const char * id;
  const char * name;
  AkNode     * node;
  AkEvaluateScene * evaluateScene;
  AkTree     * extra;

  struct AkVisualScene * next;
} AkVisualScene;

#undef _ak_DEF_LIB
#undef AK__DEF_LIB

#define _ak_DEF_LIB(T)                                                       \
  typedef struct ak_lib_ ## T ## _s  ak_lib_ ## T;                          \
  struct ak_lib_ ## T ## _s {                                                \
    ak_assetinf  * inf;                                                      \
    const char    * id;                                                       \
    const char    * name;                                                     \
    ak_##T       * chld;                                                     \
    AkTree      * extra;                                                    \
    unsigned long   count;                                                    \
    ak_lib_##T   * next;                                                     \
  }

#define AK__DEF_LIB(T)                                                        \
  typedef struct AkLib ## T {                                                 \
    ak_assetinf * inf;                                                        \
    const char  * id;                                                         \
    const char  * name;                                                       \
    Ak ## T     * chld;                                                       \
    AkTree      * extra;                                                      \
    uint64_t      count;                                                      \
    struct AkLib ## T * next;                                                 \
  } AkLib ## T

_ak_DEF_LIB(camera);
_ak_DEF_LIB(light);
_ak_DEF_LIB(effect);
_ak_DEF_LIB(image);
_ak_DEF_LIB(material);
AK__DEF_LIB(Geometry);
AK__DEF_LIB(Controller);
AK__DEF_LIB(VisualScene);

#undef _ak_DEF_LIB
#undef AK__DEF_LIB

typedef struct ak_lib_s ak_lib;
struct ak_lib_s {
  ak_lib_camera    * cameras;
  ak_lib_light     * lights;
  ak_lib_effect    * effects;
  ak_lib_image     * images;
  ak_lib_material  * materials;
  AkLibGeometry    * geometries;
  AkLibController  * controllers;
  AkLibVisualScene * visualScenes;
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

AkResult
AK_EXPORT
ak_load(ak_doc ** __restrict dest,
         const char * __restrict file, ...);

#ifdef __cplusplus
//}
#endif
#endif /* __libassetkit__assetkit__h_ */
