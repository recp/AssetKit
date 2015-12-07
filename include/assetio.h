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
//extern "C" {
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
#define AIO_FORMAT_PRECISION_LOW                                        0x01
#define AIO_FORMAT_PRECISION_MID                                        0x01
#define AIO_FORMAT_PRECISION_HIGHT                                      0x01
#define AIO_FORMAT_PRECISION_MAX                                        0x01

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
};

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
  aio_asset_base;

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
  aio_asset_base;

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
  aio_asset_base;
  
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

#ifdef _AIO_PROFILE_BASE_
#  undef _AIO_PROFILE_BASE_
#endif

/*
 * Common properties of all profiles
 */
#define _AIO_PROFILE_BASE_                                                    \
  aio_asset_base;                                                             \
  aio_profile_type   profile_type;                                            \
  const char       * id;                                                      \
  aio_newparam     * newparam;                                                \
  aio_technique    * technique;                                               \
  aio_tree         * extra;                                                   \
  aio_profile      * prev;                                                    \
  aio_profile      * next;

typedef struct aio_profile_s aio_profile;
struct aio_profile_s {
  _AIO_PROFILE_BASE_;
};

typedef struct aio_profile_common_s aio_profile_common;
struct aio_profile_common_s {
  _AIO_PROFILE_BASE_;
};

typedef struct aio_profile_cg_s aio_profile_CG;
struct aio_profile_cg_s {
  _AIO_PROFILE_BASE_;

  const char    * platform;
  aio_code      * code;
  aio_include   * include;
};

typedef struct aio_profile_gles_s aio_profile_GLES;
struct aio_profile_gles_s {
  _AIO_PROFILE_BASE_;

  const char * platform;
};

typedef struct aio_profile_gles2_s aio_profile_GLES2;
struct aio_profile_gles2_s {
  _AIO_PROFILE_BASE_;

  const char  * language;
  aio_code    * code;
  aio_include * include;
  const char  * platforms;
};

typedef struct aio_profile_glsl_s aio_profile_GLSL;
struct aio_profile_glsl_s {
  _AIO_PROFILE_BASE_;

  const char  * platform;
  aio_code    * code;
  aio_include * include;
};

typedef struct aio_profile_bridge_s aio_profile_BRIDGE;
struct aio_profile_bridge_s {
  _AIO_PROFILE_BASE_;
  
  const char * platform;
  const char * url;
};

#undef _AIO_PROFILE_BASE_

typedef struct aio_effect_s aio_effect;
struct aio_effect_s {
  aio_asset_base;

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
  };

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
  aio_asset_base;
  void * aio_data;
};

extern void aio_cleanup();

int aio_load(aio_doc ** __restrict dest,
             const char * __restrict file, ...);

#ifdef __cplusplus
//}
#endif
#endif /* __libassetio__assetio__h_ */
