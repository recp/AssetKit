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

#include "ak-common.h"

#ifdef __cplusplus
extern "C" {
#endif

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
#include "ak-coord.h"

/**
 * @brief library time type
 */
typedef time_t ak_time_t;

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
  AK_TECHNIQUE_COMMON_LIGHT_SPOT          = 6,
  AK_TECHNIQUE_COMMON_INSTANCE_MATERIAL   = 7
} AkTechniqueCommonType;

typedef enum AkFileType {
  AK_FILE_TYPE_AUTO      = 0,
  AK_FILE_TYPE_COLLADA   = 1,
  AK_FILE_TYPE_WAVEFRONT = 2,
  AK_FILE_TYPE_FBX       = 3
} AkFileType;

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

typedef enum AkGeometryType {
  AK_GEOMETRY_TYPE_MESH   = 0,
  AK_GEOMETRY_TYPE_SPLINE = 1,
  AK_GEOMETRY_TYPE_BREP   = 2
} AkGeometryType;

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

typedef enum AkMorphMethod {
  AK_MORPH_METHOD_NORMALIZED = 1,
  AK_MORPH_METHOD_RELATIVE   = 2
} AkMorphMethod;

typedef enum AkNodeType {
  AK_NODE_TYPE_NODE        = 1,
  AK_NODE_TYPE_CAMERA_NODE = 2,
  AK_NODE_TYPE_JOINT       = 3
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
 * TODO: remove this!
 */
#define ak_asset_base                                                        \
  AkAssetInf  * inf;

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

#include "ak-source.h"

typedef struct AkUnit {
  const char * name;
  double       dist;
} AkUnit;

typedef struct AkColorRGBA {
  AkFloat R;
  AkFloat G;
  AkFloat B;
  AkFloat A;
} AkColorRGBA;

typedef union AkColorU {
  AkColorRGBA rgba;
  AkFloat4    vec;
} AkColorU;

typedef struct AkColor {
  const char * sid;
  AkColorU color;
} AkColor;

typedef struct AkContributor {
  const char * author;
  const char * authorEmail;
  const char * authorWebsite;
  const char * authoringTool;
  const char * comments;
  const char * copyright;
  const char * sourceData;

  struct AkContributor * next;
} AkContributor;

typedef struct AkAltitude {
  AkAltitudeMode mode;
  double         val;
} AkAltitude;

typedef struct AkGeoLoc {
  double     lng;
  double     lat;
  AkAltitude alt;
} AkGeoLoc;

typedef struct AkCoverage {
  AkGeoLoc geoLoc;
} AkCoverage;

typedef struct AkAssetInf {
  AkCoordSys    *coordSys;
  AkUnit        *unit;
  AkContributor *contributor;
  AkCoverage    *coverage;
  const char    *subject;
  const char    *title;
  const char    *keywords;

  AkTree        *extra;
  ak_time_t      created;
  ak_time_t      modified;
  unsigned long  revision;
} AkAssetInf;

typedef struct AkDocInf {
  AkAssetInf   base;
  const char * fname;
  AkFileType   ftype;
} AkDocInf;

typedef struct AkTechnique {
  const char * profile;

  /**
   * @brief
   * COLLADA Specs 1.5:
   * This XML Schema namespace attribute identifies an additional schema
   * to use for validating the content of this instance document. Optional.
   */
  const char * xmlns;
  AkTree     * chld;

  struct AkTechnique * next;
} AkTechnique;

typedef struct AkTechniqueCommon  {
  struct AkTechniqueCommon * next;
  void                     * technique;
  AkTechniqueCommonType      techniqueType;
} AkTechniqueCommon;

typedef struct AkPerspective {
  ak_basic_attrf * xfov;
  ak_basic_attrf * yfov;
  ak_basic_attrf * aspectRatio;
  ak_basic_attrf * znear;
  ak_basic_attrf * zfar;
} AkPerspective;

typedef struct AkOrthographic {
  ak_basic_attrf * xmag;
  ak_basic_attrf * ymag;
  ak_basic_attrf * aspectRatio;
  ak_basic_attrf * znear;
  ak_basic_attrf * zfar;
} AkOrthographic;

typedef struct AkOptics {
  AkTechniqueCommon * techniqueCommon;
  AkTechnique       * technique;
} AkOptics;

typedef struct AkImager {
  AkTechnique * technique;
  AkTree      * extra;
} AkImager;

typedef struct AkCamera {
  ak_asset_base

  /* const char * id; */
  const char * name;
  AkOptics   * optics;
  AkImager   * imager;
  AkTree     * extra;

  struct AkCamera * next;
} AkCamera;

typedef struct AkAmbient {
  AkColor color;
} AkAmbient;

typedef struct AkDirectional {
  AkColor color;
} AkDirectional;

typedef struct AkPoint {
  AkColor          color;
  ak_basic_attrd * constantAttenuation;
  ak_basic_attrd * linearAttenuation;
  ak_basic_attrd * quadraticAttenuation;
} AkPoint;

typedef struct AkSpot {
  AkColor          color;
  ak_basic_attrd * constantAttenuation;
  ak_basic_attrd * linearAttenuation;
  ak_basic_attrd * quadraticAttenuation;
  ak_basic_attrd * falloffAngle;
  ak_basic_attrd * falloffExponent;
} AkSpot;

typedef struct AkLight {
  ak_asset_base

  /* const char * id; */
  const char * name;

  AkTechniqueCommon * techniqueCommon;
  AkTechnique       * technique;
  AkTree            * extra;
  struct AkLight    * next;
} AkLight;

/* FX */
/* Effects */
/*
 * base type of param
 */
typedef struct AkParam {
  AkParamType  paramType;
  const char * ref;

  struct AkParam * next;
} AkParam;

typedef struct AkParamEx {
  AkParam      base;
  const char * name;
  const char * sid;
  const char * semantic;
  const char * typeName;
  AkValueType  type;
} AkParamEx;

typedef struct AkHexData {
  const char * format;
  const char * val;
} AkHexData;

typedef struct AkInitFrom {
  const char * ref;
  AkHexData  * hex;

  AkFace face;
  AkUInt mipIndex;
  AkUInt depth;
  AkInt  arrayIndex;
  AkBool mipsGenerate;

  struct AkInitFrom * next;
} AkInitFrom;

typedef struct AkSizeExact {
  AkFloat width;
  AkFloat height;
} AkSizeExact;

typedef struct AkSizeRatio {
  AkFloat width;
  AkFloat height;
} AkSizeRatio;

typedef struct AkMips {
  AkUInt levels;
  AkBool autoGenerate;
} AkMips;

typedef struct AkImageFormat {
  struct {
    AkChannelFormat   channel;
    AkRangeFormat     range;
    AkPrecisionFormat precision;
    const char      * space;
  } hint;

  const char * exact;
} AkImageFormat;

typedef struct AkImage2d {
  AkSizeExact   * sizeExact;
  AkSizeRatio   * sizeRatio;
  AkMips        * mips;
  const char    * unnormalized;
  AkImageFormat * format;
  AkInitFrom    * initFrom;
  long            arrayLen;
} AkImage2d;

typedef struct AkImage3d {
  struct {
    AkUInt width;
    AkUInt height;
    AkUInt depth;
  } size;

  AkMips          mips;
  long            arrayLen;
  AkImageFormat * format;
  AkInitFrom    * initFrom;
} AkImage3d;

typedef struct AkImageCube {
  AkUInt          width;
  AkMips          mips;
  long            arrayLen;
  AkImageFormat * format;
  AkInitFrom    * initFrom;
} AkImageCube;

typedef struct AkImage {
  ak_asset_base

  /* const char * id; */
  const char * sid;
  const char * name;

  AkInitFrom  * initFrom;
  AkImage2d   * image2d;
  AkImage3d   * image3d;
  AkImageCube * cube;
  AkTree      * extra;

  struct AkImage * next;

  AkBool renderableShare;
} AkImage;

typedef struct AkInstanceBase {
  void       *object;
  const char *url;
  const char *sid;
  const char *name;
  AkTree     *extra;
} AkInstanceBase;

typedef struct AkInstanceImage {
  AkImage    * image;
  const char * url;
  const char * sid;
  const char * name;

  AkTree * extra;
} AkInstanceImage;

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
typedef struct AkFxSamplerCommon {
  ak_asset_base
  AkInstanceImage * instanceImage;

  const char * texcoordSemantic;

  AkWrapMode wrapS;
  AkWrapMode wrapT;
  AkWrapMode wrapP;

  AkMinFilter minfilter;
  AkMagFilter magfilter;
  AkMipFilter mipfilter;

  unsigned long mipMaxLevel;
  unsigned long mipMinLevel;
  float         mipBias;
  unsigned long maxAnisotropy;

  AkColor * borderColor;
  AkTree  * extra;
} AkFxSamplerCommon;

typedef AkFxSamplerCommon AkSampler1D;
typedef AkFxSamplerCommon AkSampler2D;
typedef AkFxSamplerCommon AkSampler3D;
typedef AkFxSamplerCommon AkSamplerCUBE;
typedef AkFxSamplerCommon AkSamplerDEPTH;
typedef AkFxSamplerCommon AkSamplerRECT;
typedef AkFxSamplerCommon AkSamplerStates;

typedef struct AkFxTexture {
  const char * texture;
  const char * texcoord;
  AkTree     * extra;
} AkFxTexture;

typedef struct AkFxColorOrTex {
  AkColor     * color;
  AkParam     * param;
  AkFxTexture * texture;
  AkOpaque      opaque;
} AkFxColorOrTex;

typedef AkFxColorOrTex AkAmbientFx;
typedef AkFxColorOrTex AkDiffuse;
typedef AkFxColorOrTex AkEmission;
typedef AkFxColorOrTex AkReflective;
typedef AkFxColorOrTex AkSpecular;
typedef AkFxColorOrTex AkTransparent;

typedef struct AkFxFloatOrParam {
  ak_basic_attrf * val;
  AkParam        * param;
} AkFxFloatOrParam;

typedef AkFxFloatOrParam AkIndexOfRefraction;
typedef AkFxFloatOrParam AkReflectivity;
typedef AkFxFloatOrParam AkShininess;
typedef AkFxFloatOrParam AkTransparency;

typedef struct AkAnnotate {
  const char * name;
  void       * val;
  AkValueType  valType;

  struct AkAnnotate * next;
} AkAnnotate;

typedef struct AkNewParam {
  const char * sid;
  AkAnnotate * annotate;
  const char * semantic;
  void       * val;
  AkModifier   modifier;
  AkValueType  valType;

  struct AkNewParam * next;
} AkNewParam;

typedef struct AkSetParam {
  const char * ref;
  void       * val;
  AkValueType  valType;

  struct AkSetParam * next;
} AkSetParam;

typedef struct AkCode {
  const char * sid;
  const char * val;

  struct AkCode * next;
} AkCode;

typedef struct AkInclude {
  const char * sid;
  const char * url;

  struct AkInclude * next;
} AkInclude;

typedef struct AkBlinn {
  AkEmission          * emission;
  AkAmbientFx         * ambient;
  AkDiffuse           * diffuse;
  AkSpecular          * specular;
  AkShininess         * shininess;
  AkReflective        * reflective;
  AkReflectivity      * reflectivity;
  AkTransparent       * transparent;
  AkTransparency      * transparency;
  AkIndexOfRefraction * indexOfRefraction;
} AkBlinn;

typedef struct AkConstantFx {
  AkEmission          * emission;
  AkReflective        * reflective;
  AkReflectivity      * reflectivity;
  AkTransparent       * transparent;
  AkTransparency      * transparency;
  AkIndexOfRefraction * indexOfRefraction;
} AkConstantFx;

typedef struct AkLambert {
  AkEmission          * emission;
  AkAmbientFx         * ambient;
  AkDiffuse           * diffuse;
  AkReflective        * reflective;
  AkReflectivity      * reflectivity;
  AkTransparent       * transparent;
  AkTransparency      * transparency;
  AkIndexOfRefraction * indexOfRefraction;
} AkLambert;

typedef struct AkPhong {
  AkEmission          * emission;
  AkAmbientFx         * ambient;
  AkDiffuse           * diffuse;
  AkSpecular          * specular;
  AkShininess         * shininess;
  AkReflective        * reflective;
  AkReflectivity      * reflectivity;
  AkTransparent       * transparent;
  AkTransparency      * transparency;
  AkIndexOfRefraction * indexOfRefraction;
} AkPhong;

struct AkRenderState;
typedef struct AkStates {
  struct AkRenderState * next;
  long count;
} AkStates;

typedef struct AkEvaluateTarget {
  AkParam     * param;
  unsigned long index;
  unsigned long slice;
  unsigned long mip;
  AkFace        face;

  AkInstanceImage * instanceImage;
} AkEvaluateTarget;

typedef struct AkColorClear {
  unsigned long index;
  AkColor       val;
} AkColorClear;

typedef struct AkDepthClear {
  unsigned long index;
  AkFloat       val;
} AkDepthClear;

typedef struct AkStencilClear {
  unsigned long index;
  unsigned long val;
} AkStencilClear;

typedef struct AkDraw {
  AkDrawType   enumDraw;
  const char * strVal;
} AkDraw;

typedef struct AkEvaluate {
  AkEvaluateTarget * colorTarget;
  AkEvaluateTarget * depthTarget;
  AkEvaluateTarget * stencilTarget;
  AkColorClear     * colorClear;
  AkDepthClear     * depthClear;
  AkStencilClear   * stencilClear;
  AkDraw             draw;
} AkEvaluate;

typedef struct AkInline {
  const char * val;
  struct AkInline * next;
} AkInline;

typedef struct AkImport {
  const char * ref;
  struct AkImport * next;
} AkImport;

typedef struct AkSources {
  const char * entry;
  AkInline   * inlines;
  AkImport   * imports;
} AkSources;

typedef struct AkBinary {
  const char * ref;
  AkHexData  * hex;

  struct AkBinary * next;
} AkBinary;

typedef struct AkCompiler {
  const char * platform;
  const char * target;
  const char * options;
  AkBinary   * binary;

  struct AkCompiler * next;
} AkCompiler;

typedef struct AkBindUniform {
  const char * symbol;
  AkParam    * param;
  void       * val;
  AkValueType  valType;

  struct AkBindUniform * next;
} AkBindUniform;

typedef struct AkBindAttrib {
  const char * symbol;
  const char * semantic;

  struct AkBindAttrib * next;
} AkBindAttrib;

typedef struct AkShader {
  AkPipelineStage stage;

  AkSources     * sources;
  AkCompiler    * compiler;
  AkBindUniform * bindUniform;
  AkTree        * extra;

  struct AkShader * next;
} AkShader;

typedef struct AkLinker {
  const char * platform;
  const char * target;
  const char * options;
  AkBinary   * binary;

  struct AkLinker * next;
} AkLinker;

typedef struct AkProgram {
  AkShader      * shader;
  AkBindAttrib  * bindAttrib;
  AkBindUniform * bindUniform;
  AkLinker      * linker;
} AkProgram;

typedef struct AkPass {
  ak_asset_base

  const char * sid;
  AkAnnotate * annotate;
  AkStates   * states;
  AkEvaluate * evaluate;
  AkProgram  * program;
  AkTree     * extra;

  struct AkPass * next;
} AkPass;

typedef struct AkTechniqueFx {
  ak_asset_base

  /* const char   * id; */
  const char   * sid;
  AkAnnotate   * annotate;
  AkBlinn      * blinn;
  AkConstantFx * constant;
  AkLambert    * lambert;
  AkPhong      * phong;
  AkPass       * pass;
  AkTree       * extra;

  struct AkTechniqueFx * next;
} AkTechniqueFx;

typedef struct AkProfile {
  ak_asset_base

  AkProfileType   profileType;
  /* const char    * id; */
  AkNewParam    * newparam;
  AkTechniqueFx * technique;
  AkTree        * extra;

  struct AkProfile * next;
} AkProfile;

typedef AkProfile AkProfileCommon;

typedef struct AkProfileCG {
  AkProfile base;

  AkCode     * code;
  AkInclude  * include;
  const char * platform;
} AkProfileCG;

typedef struct AkProfileGLES {
  AkProfile base;

  const char * platform;
} AkProfileGLES;

typedef struct AkProfileGLES2 {
  AkProfile base;

  AkCode     * code;
  AkInclude  * include;
  const char * language;
  const char * platforms;
} AkProfileGLES2;

typedef struct AkProfileGLSL {
  AkProfile base;

  AkCode     * code;
  AkInclude  * include;
  const char * platform;
} AkProfileGLSL;

typedef struct AkProfileBridge {
  AkProfile base;

  const char * platform;
  const char * url;
} AkProfileBridge;

typedef struct AkEffect {
  ak_asset_base

  /* const char * id; */
  const char * name;

  AkAnnotate * annotate;
  AkNewParam * newparam;
  AkProfile  * profile;
  AkTree     * extra;

  struct AkEffect * next;
} AkEffect;

typedef struct AkTechniqueHint {
  const char * platform;
  const char * ref;
  const char * profile;

  struct AkTechniqueHint * next;
} AkTechniqueHint;

typedef struct AkInstanceEffect {
  AkEffect   * effect;
  const char * url;
  const char * sid;
  const char * name;

  AkTechniqueHint * techniqueHint;
  AkSetParam      * setparam;
  AkTree          * extra;
} AkInstanceEffect;

typedef struct AkMaterial {
  ak_asset_base
  /* const char * id; */
  const char * name;
  AkInstanceEffect *instanceEffect;
  AkTree     * extra;

  struct AkMaterial * next;
} AkMaterial;

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

  /* const char   * id; */
  const char   * name;
  AkInputBasic * input;
  uint32_t       inputCount;
  AkTree       * extra;
} AkVertices;

typedef struct AkMeshPrimitive {
  AkMeshPrimitiveType    type;
  const char             *name;
  const char             *material;
  AkInput                *input;
  uint32_t                inputCount;
  AkVertices             *vertices;
  AkUIntArray            *indices;
  AkTree                 *extra;
  struct AkMeshPrimitive *next;
} AkMeshPrimitive;

typedef struct AkLines {
  AkMeshPrimitive base;
  uint64_t        count;
  AkLineMode      mode;
} AkLines;

typedef struct AkPolygon {
  AkMeshPrimitive base;
  AkDoubleArrayL *holes;
  AkIntArray     *vcount;
  AkPolygonMode   mode;
  AkBool          haveHoles;
} AkPolygon;

typedef struct AkTriangles {
  AkMeshPrimitive base;
  uint64_t        count;
  AkTriangleMode  mode;
} AkTriangles;

typedef struct AkMesh {
  ak_asset_base
  const char      *convexHullOf;
  AkSource        *source;
  AkVertices      *vertices;
  AkMeshPrimitive *primitive;
  uint32_t         primitiveCount;
  AkTree          *extra;
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
  AkTree  * extra;
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
  AkDouble3 direction;
  AkDouble3 origin;
  AkFloat3  axis;
  AkTree  * extra;
} AkSweptSurface;

typedef struct AkSurface {
  const char * sid;
  const char * name;

  AkObject       * surface;
  AkDoubleArrayL * orient;
  AkDouble3        origin;

  struct AkSurface * next;
} AkSurface;

typedef struct AkSurfaces {
  AkSurface * surface;
  AkTree    * extra;
} AkSurfaces;

typedef struct AkEdges {
  /* const char    * id; */
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkEdges;

typedef struct AkWires {
  /* const char    * id; */
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkWires;

typedef struct AkFaces {
  /* const char    * id; */
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkFaces;

typedef struct AkPCurves {
  /* const char    * id; */
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkPCurves;

typedef struct AkShells {
  /* const char    * id; */
  const char    * name;
  AkUInt          count;

  AkInput       * input;
  AkIntArray    * vcount;
  AkDoubleArray * primitives;
  AkTree        * extra;
} AkShells;

typedef struct AkSolids {
  /* const char    * id; */
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

  /* const char * id; */
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

  const char  * baseMesh;
  AkMorphMethod method;

  AkSource    * source;
  AkTargets   * targets;
  AkTree      * extra;
} AkMorph;

typedef struct AkController {
  ak_asset_base

  /* const char * id; */
  const char * name;
  AkObject   * data;
  AkTree     * extra;

  struct AkController * next;
} AkController;

typedef struct AkLookAt {
  const char * sid;
  AkFloat3     val[3];
} AkLookAt;

typedef struct AkMatrix {
  const char          *sid;
  AK_ALIGN(16) AkFloat val[4][4];
} AkMatrix;

typedef struct AkRotate {
  const char * sid;
  AK_ALIGN(16) AkFloat val[4];
} AkRotate;

typedef struct AkScale {
  const char * sid;
  AkFloat      val[3];
} AkScale;

typedef struct AkSkew {
  const char * sid;
  AkFloat      angle;
  AkFloat3     rotateAxis;
  AkFloat3     aroundAxis;
} AkSkew;

typedef struct AkTranslate {
  const char * sid;
  AkFloat      val[3];
} AkTranslate;

typedef struct AkInstanceCamera {
  AkInstanceBase  base;
  struct AkInstanceCamera * next;
} AkInstanceCamera;

typedef struct AkSkeleton {
  const char * val;

  struct AkSkeleton * next;
} AkSkeleton;

typedef struct AkBindMaterial {
  AkParam      * param;
  AkTechniqueCommon * techniqueCommon;
  AkTechnique  * technique;
  AkTree       * extra;
} AkBindMaterial;

typedef struct AkInstanceController {
  AkController   * controller;
  const char     * name;
  const char     * url;
  AkSkeleton     * skeleton;
  AkBindMaterial * bindMaterial;
  AkTree         * extra;

  struct AkInstanceController * next;
} AkInstanceController;

typedef struct AkInstanceGeometry {
  AkInstanceBase  base;
  AkBindMaterial *bindMaterial;

  struct AkInstanceGeometry * next;
} AkInstanceGeometry;

typedef struct AkInstanceLight {
  AkLight    * light;
  const char * name;
  const char * url;
  AkTree     * extra;

  struct AkInstanceLight * next;
} AkInstanceLight;

typedef struct AkNode AkNode;
typedef struct AkInstanceNode {
  AkNode     * node;
  const char * name;
  const char * url;
  const char * proxy;
  AkTree     * extra;

  struct AkInstanceNode * next;
} AkInstanceNode;

/*
 * TODO: separate all instances to individual nodes?
 */
struct AkNode {
  ak_asset_base

  /* const char * id; */
  const char * name;
  const char * sid;
  AkNodeType   nodeType;
  AkStringArray        * layer;
  AkObject             * transform;
  AkInstanceCamera     * camera;
  AkInstanceController * controller;
  AkInstanceGeometry   * geometry;
  AkInstanceLight      * light;
  AkInstanceNode       * node;
  AkTree        * extra;
  struct AkNode * chld;
  struct AkNode * parent;
  struct AkNode * next;
};

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

  AkMaterial          * material;
  const char          * sid;
  const char          * name;
  const char          * target;
  const char          * symbol;
  const char          * url;
  AkTechniqueOverride * techniqueOverride;
  AkBind              * bind;
  AkBindVertexInput   * bindVertexInput;
  AkTree              * extra;

  struct AkInstanceMaterial * next;
} AkInstanceMaterial;

typedef struct AkRender {
  ak_asset_base

  const char     * name;
  const char     * sid;
  const char     * cameraNode;
  AkStringArrayL * layer;
  AkInstanceMaterial * instanceMaterial;
  AkTree         * extra;

  struct AkRender * next;
} AkRender;

typedef struct AkEvaluateScene {
  ak_asset_base

  /* const char * id; */
  const char * name;
  const char * sid;
  AkRender   * render;
  AkTree     * extra;
  AkBool       enable;

  struct AkEvaluateScene * next;
} AkEvaluateScene;

typedef struct AkVisualScene {
  ak_asset_base

  /* const char * id; */
  const char * name;
  AkNode     * node;
  AkNode     * firstCamNode;
  AkEvaluateScene * evaluateScene;
  AkTree     * extra;

  struct AkVisualScene * next;
} AkVisualScene;

typedef struct AkInstanceVisualScene {
  AkInstanceBase  base;
} AkInstanceVisualScene;

typedef struct AkScene {
  ak_asset_base

  /* 
   TODO: 
      instance_physics_scene
      instance_kinematics_scene
   */

  AkInstanceVisualScene *visualScene;
  AkTree * extra;
} AkScene;

#undef AK__DEF_LIB

#define AK__DEF_LIB(T)                                                        \
  typedef struct AkLib ## T {                                                 \
    AkAssetInf * inf;                                                         \
    /* const char * id; */                                                    \
    const char * name;                                                        \
    Ak ## T    * chld;                                                        \
    AkTree     * extra;                                                       \
    uint64_t     count;                                                       \
    struct AkLib ## T * next;                                                 \
  } AkLib ## T

AK__DEF_LIB(Camera);
AK__DEF_LIB(Light);
AK__DEF_LIB(Effect);
AK__DEF_LIB(Image);
AK__DEF_LIB(Material);
AK__DEF_LIB(Geometry);
AK__DEF_LIB(Controller);
AK__DEF_LIB(VisualScene);
AK__DEF_LIB(Node);

#undef AK__DEF_LIB

typedef struct AkLib {
  AkLibCamera      * cameras;
  AkLibLight       * lights;
  AkLibEffect      * effects;
  AkLibImage       * images;
  AkLibMaterial    * materials;
  AkLibGeometry    * geometries;
  AkLibController  * controllers;
  AkLibVisualScene * visualScenes;
  AkLibNode        * nodes;
} AkLib;

typedef struct AkDoc {
  AkDocInf    docinf;
  AkCoordSys *coordSys;
  AkUnit     *unit;
  AkLib       lib;
  AkScene     scene;
} AkDoc;

#include "ak-states.h"
#include "ak-string.h"
#include "ak-coord-util.h"
#include "ak-lib.h"
#include "ak-instance.h"
#include "ak-geom.h"
#include "ak-cam.h"
#include "ak-transform.h"

AK_EXPORT
AkResult
ak_load(AkDoc ** __restrict dest,
        const char * __restrict file,
        .../* options */);

AK_EXPORT
void *
ak_getId(void * __restrict objptr);

AK_EXPORT
AkResult
ak_setId(void * __restrict objptr,
         const char * __restrict objectId);

AK_EXPORT
void *
ak_getObjectById(AkDoc * __restrict doc,
                 const char * __restrict objectId);

AK_EXPORT
void *
ak_getObjectByUrl(AkDoc * __restrict doc,
                  const char * __restrict objectUrl);

#ifdef __cplusplus
}
#endif
#endif /* __libassetkit__assetkit__h_ */
