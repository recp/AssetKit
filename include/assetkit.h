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
#include "ak-url.h"

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

typedef union AkColor {
  AK_ALIGN(16) AkColorRGBA rgba;
  AK_ALIGN(16) AkFloat4    vec;
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

/* FX */
/* Effects */
/*
 * base type of param
 */
typedef struct AkParam {
  const char     *ref;
  struct AkParam *next;
} AkParam;

typedef struct AkHexData {
  const char *format;
  const char *hexval;
  void       *data;
} AkHexData;

typedef struct AkInitFrom {
  struct AkInitFrom *next;
  const char        *ref;
  AkHexData         *hex;
  AkFace             face;
  AkUInt             mipIndex;
  AkUInt             depth;
  AkInt              arrayIndex;
  AkBool             mipsGenerate;
} AkInitFrom;

typedef struct AkInstanceBase {
  /* const char * sid; */

  AkURL       url;
  void       *object;
  const char *name;
  AkTree     *extra;
  struct AkInstanceBase *next;
} AkInstanceBase;

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
  AkInstanceBase *instanceImage;
  const char     *texcoordSemantic;
  AkColor        *borderColor;
  AkTree         *extra;

  AkWrapMode      wrapS;
  AkWrapMode      wrapT;
  AkWrapMode      wrapP;

  AkMinFilter     minfilter;
  AkMagFilter     magfilter;
  AkMipFilter     mipfilter;

  unsigned long   maxAnisotropy;
  unsigned long   mipMaxLevel;
  unsigned long   mipMinLevel;
  float           mipBias;
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

typedef struct AkAnnotate {
  const char * name;
  void       * val;
  AkValueType  valType;

  struct AkAnnotate * next;
} AkAnnotate;

typedef struct AkNewParam {
  /* const char * sid; */
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
  /* const char * sid; */

  const char * val;

  struct AkCode * next;
} AkCode;

typedef struct AkInclude {
  /* const char * sid; */

  const char * url;

  struct AkInclude * next;
} AkInclude;

struct AkRenderState;
typedef struct AkStates {
  struct AkRenderState * next;
  long count;
} AkStates;

typedef struct AkEvaluateTarget {
  AkParam        *param;
  AkInstanceBase *instanceImage;
  unsigned long   index;
  unsigned long   slice;
  unsigned long   mip;
  AkFace          face;
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

  /* const char * sid; */

  AkAnnotate * annotate;
  AkStates   * states;
  AkEvaluate * evaluate;
  AkProgram  * program;
  AkTree     * extra;

  struct AkPass * next;
} AkPass;

#include "ak-profile.h"

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
  AkInstanceBase   base;
  AkTechniqueHint *techniqueHint;
  AkSetParam      *setparam;
} AkInstanceEffect;

typedef struct AkMaterial {
  ak_asset_base
  /* const char * id; */
  const char        *name;
  AkInstanceEffect  *effect;
  AkTree            *extra;
  struct AkMaterial *next;
} AkMaterial;

typedef struct AkInputBasic {
  AkURL                source;
  AkInputSemantic      semantic;
  const char          *semanticRaw;
  struct AkInputBasic *next;
  uint32_t             reserved;
} AkInputBasic;

typedef struct AkInput {
  AkInputBasic base;

  uint32_t offset;
  uint32_t set;
} AkInput;

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

typedef struct AkSkeleton {
  const char * val;

  struct AkSkeleton * next;
} AkSkeleton;

struct AkInstanceMaterial;
typedef struct AkBindMaterial {
  AkParam                   *param;
  struct AkInstanceMaterial *tcommon;
  AkTechnique               *technique;
  AkTree                    *extra;
} AkBindMaterial;

typedef struct AkInstanceController {
  AkURL            url;
  AkController   * controller;
  const char     * name;
  AkSkeleton     * skeleton;
  AkBindMaterial * bindMaterial;
  AkTree         * extra;

  struct AkInstanceController * next;
} AkInstanceController;

typedef struct AkInstanceGeometry {
  AkInstanceBase  base;
  AkBindMaterial *bindMaterial;
} AkInstanceGeometry;

typedef struct AkNode AkNode;
typedef struct AkInstanceNode {
  AkInstanceBase base;
  const char    *proxy;
} AkInstanceNode;

/*
 * TODO: separate all instances to individual nodes?
 */
struct AkNode {
  ak_asset_base

  /* const char * id;  */
  /* const char * sid; */

  const char           *name;
  AkNodeType            nodeType;
  AkStringArray        *layer;
  AkObject             *transform;

  AkInstanceBase       *camera;
  AkInstanceController *controller;
  AkInstanceGeometry   *geometry;
  AkInstanceBase       *light;
  AkInstanceNode       *node;

  AkTree               *extra;
  struct AkNode        *chld;
  struct AkNode        *parent;
  struct AkNode        *next;
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
  AkInstanceBase       base;
  const char          *symbol;
  AkTechniqueOverride *techniqueOverride;
  AkBind              *bind;
  AkBindVertexInput   *bindVertexInput;
} AkInstanceMaterial;

typedef struct AkRender {
  ak_asset_base

  /* const char * sid; */

  const char     * name;
  const char     * cameraNode;
  AkStringArrayL * layer;
  AkInstanceMaterial * instanceMaterial;
  AkTree         * extra;

  struct AkRender * next;
} AkRender;

typedef struct AkEvaluateScene {
  ak_asset_base

  /* const char * id; */
  /* const char * sid; */

  const char * name;
  AkRender   * render;
  AkTree     * extra;
  AkBool       enable;

  struct AkEvaluateScene * next;
} AkEvaluateScene;

typedef struct AkVisualScene {
  ak_asset_base

  /* const char * id; */
  const char           *name;
  AkNode               *node;
  AkNode               *firstCamNode;
  AkEvaluateScene      *evaluateScene;
  struct AkBoundingBox *bbox;
  AkTree               *extra;
  struct AkVisualScene *next;
} AkVisualScene;

typedef struct AkScene {
  ak_asset_base

  /*
   TODO:
      instance_physics_scene
      instance_kinematics_scene
   */
  AkInstanceBase *visualScene;
  AkTree * extra;
} AkScene;

typedef struct AkLibItem {
  /* const char * id; */

  AkAssetInf *inf;
  const char *name;
  AkTree     *extra;
  void       *chld;
  uint64_t    count;

  struct AkLibItem *next;
} AkLibItem;

typedef struct AkLib {
  AkLibItem *cameras;
  AkLibItem *lights;
  AkLibItem *effects;
  AkLibItem *images;
  AkLibItem *materials;
  AkLibItem *geometries;
  AkLibItem *controllers;
  AkLibItem *visualScenes;
  AkLibItem *nodes;
} AkLib;

typedef struct AkDoc {
  AkDocInf    docinf;
  AkCoordSys *coordSys;
  AkUnit     *unit;
  AkTree     *extra;
  AkLib       lib;
  AkScene     scene;
} AkDoc;

#include "ak-geom.h"
#include "ak-image.h"
#include "ak-states.h"
#include "ak-string.h"
#include "ak-coord-util.h"
#include "ak-lib.h"
#include "ak-instance.h"
#include "ak-cam.h"
#include "ak-transform.h"
#include "ak-sid.h"
#include "ak-light.h"

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
AkResult
ak_moveId(void * __restrict objptrOld,
          void * __restrict objptrNew);

AK_EXPORT
void *
ak_getObjectById(AkDoc * __restrict doc,
                 const char * __restrict objectId);

AK_EXPORT
void *
ak_getObjectByUrl(AkURL * __restrict url);

AK_EXPORT
const char *
ak_generatId(AkDoc      * __restrict doc,
             void       * __restrict parentmem,
             const char * __restrict prefix);

#ifdef __cplusplus
}
#endif
#endif /* __libassetkit__assetkit__h_ */
