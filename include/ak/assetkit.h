/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef assetkit_h
#define assetkit_h

#include <stdlib.h>
#include <time.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FList;
struct FListItem;
struct AkBuffer;
struct AkLibrary;

/* End Core Value Types */

#include "core-types.h"
#include "memory.h"
#include "coord.h"
#include "url.h"
#include "type.h"

typedef enum AkFileType {
  AK_FILE_TYPE_AUTO      = 0,
  AK_FILE_TYPE_COLLADA   = 1,
  AK_FILE_TYPE_GLTF      = 2,
  AK_FILE_TYPE_WAVEFRONT = 3,
  AK_FILE_TYPE_STL       = 4,
  AK_FILE_TYPE_PLY       = 5,
  AK_FILE_TYPE_3MF       = 6
} AkFileType;

typedef enum AkAltitudeMode {
  AK_ALTITUDE_RELATIVETOGROUND = 0,
  AK_ALTITUDE_ABSOLUTE         = 1
} AkAltitudeMode;

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
  AK_CHANNEL_FORMAT_D    = 6,
  AK_CHANNEL_FORMAT_XYZ  = 7,
  AK_CHANNEL_FORMAT_XYZW = 8
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

typedef enum AkInputSemantic {
  /* read semanticRaw */
  AK_INPUT_OTHER           = 0,
  AK_INPUT_BINORMAL        = 1,
  AK_INPUT_COLOR           = 2,
  AK_INPUT_CONTINUITY      = 3,
  AK_INPUT_IMAGE           = 4,
  AK_INPUT_INPUT           = 5,
  AK_INPUT_IN_TANGENT      = 6,
  AK_INPUT_INTERPOLATION   = 7,
  AK_INPUT_INV_BIND_MATRIX = 8,
  AK_INPUT_JOINT           = 9,
  AK_INPUT_LINEAR_STEPS    = 10,
  AK_INPUT_MORPH_TARGET    = 11,
  AK_INPUT_MORPH_WEIGHT    = 12,
  AK_INPUT_NORMAL          = 13,
  AK_INPUT_OUTPUT          = 14,
  AK_INPUT_OUT_TANGENT     = 15,
  AK_INPUT_POSITION        = 16,
  AK_INPUT_TANGENT         = 17,
  AK_INPUT_TEXBINORMAL     = 18,
  AK_INPUT_TEXCOORD        = 19,
  AK_INPUT_TEXTANGENT      = 20,
  AK_INPUT_UV              = 21,
  AK_INPUT_WEIGHT          = 22
} AkInputSemantic;

typedef enum AkCurveElementType {
  AK_CURVE_LINE      = 1,
  AK_CURVE_CIRCLE    = 2,
  AK_CURVE_ELLIPSE   = 3,
  AK_CURVE_PARABOLA  = 4,
  AK_CURVE_HYPERBOLA = 5,
  AK_CURVE_NURBS     = 6,
} AkCurveElementType;

typedef enum AkSurfaceElementType {
  AK_SURFACE_CONE          = 1,
  AK_SURFACE_PLANE         = 2,
  AK_SURFACE_CYLINDER      = 3,
  AK_SURFACE_NURBS_SURFACE = 4,
  AK_SURFACE_SPHERE        = 5,
  AK_SURFACE_TORUS         = 6,
  AK_SURFACE_SWEPT_SURFACE = 7
} AkSurfaceElementType;

typedef enum AkInstanceType {
  AK_INSTANCE_NODE       = 1,
  AK_INSTANCE_CAMERA     = 2,
  AK_INSTANCE_LIGHT      = 3,
  AK_INSTANCE_GEOMETRY   = 4,
  AK_INSTANCE_IMAGE      = 5,
  AK_INSTANCE_CONTROLLER = 6,
  AK_INSTANCE_EFFECT     = 7
} AkInstanceType;

typedef struct AkValue {
  void      *value;
  AkTypeDesc type;
} AkValue;

typedef struct AkTreeNodeAttr {
  const char            *name;
  char                  *val;
  struct AkTreeNodeAttr *next;
  struct AkTreeNodeAttr *prev;
} AkTreeNodeAttr;

typedef struct AkTreeNode {
  AkTreeNodeAttr    *attribs;
  
  const char        *name;
  char              *val;

  struct AkTreeNode *chld;
  struct AkTreeNode *parent;
  struct AkTreeNode *next;
  struct AkTreeNode *prev;
  
  unsigned long      attrc;
  unsigned long      chldc;
} AkTreeNode;

typedef struct AkTreeNode AkTree;

#include "source.h"

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
  const char    *revision;
  AkTree        *extra;
  time_t         created;
  time_t         modified;
} AkAssetInf;

typedef struct AkDocInf {
  AkAssetInf   base;
  const char  *dir;
  const char  *name;
  size_t       dirlen;
  AkFileType   ftype;
  bool         flipImage;
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
  struct AkParam *prev;
  struct AkParam *next;
} AkParam;

typedef struct AkHexData {
  const char *format;
  const char *hexval; /* hex value    */
  void       *data;   /* binary value */
} AkHexData;

typedef struct AkInitFrom {
  struct AkInitFrom *next;
  const char        *ref;
  AkHexData         *hex;
  struct AkBuffer   *buff;
  const char        *buffMime;
  AkFace             face;
  AkUInt             mipIndex;
  AkUInt             depth;
  AkInt              arrayIndex;
  AkBool             mipsGenerate;
} AkInitFrom;

struct AkNode;
typedef struct AkInstanceBase {
  /* const char * sid; */
  AkURL                  url;
  AkInstanceType         type;
  void                  *object;
  const char            *name;
  AkTree                *extra;
  struct AkNode         *node;
  struct AkInstanceBase *prev;
  struct AkInstanceBase *next;
} AkInstanceBase;

typedef struct AkEvaluateTarget {
  AkParam        *param;
  AkInstanceBase *instanceImage;
  unsigned long   index;
  unsigned long   slice;
  unsigned long   mip;
  AkFace          face;
} AkEvaluateTarget;

#include "profile.h"

struct AkNewParam;
typedef struct AkEffect {
  /* const char * id; */
  const char      *name;
  struct AkNewParam  *newparam;
  AkProfile       *profile;
  AkTree          *extra;
  struct AkEffect *next;

  /* effect specific options, override global options */
  AkProfileType    bestProfile;
} AkEffect;

typedef struct AkInstanceEffect {
  AkInstanceBase   base;
  AkTechniqueHint *techniqueHint;
} AkInstanceEffect;

typedef struct AkMaterial {
  /* const char * id; */
  AkOneWayIterBase   base;
  const char        *name;
  AkInstanceEffect  *effect;
  AkTree            *extra;
} AkMaterial;

struct AkAccessor;

typedef struct AkInput {
  const char        *semanticRaw;
  struct AkInput    *next;
  struct AkAccessor *accessor;
  uint32_t           index; /* TEXCOORD0, TEXCOORD1... */
  bool               isIndexed;
  AkInputSemantic    semantic;
  uint32_t           offset;
  uint32_t           set;
  
  /* TODO: WILL BE DELETED */
//  AkURL              source;
} AkInput;

struct AkInstanceMaterial;
typedef struct AkBindMaterial {
  AkParam                   *param;
  struct AkInstanceMaterial *tcommon;
  AkTechnique               *technique;
  AkTree                    *extra;
} AkBindMaterial;

typedef struct AkInstanceGeometry {
  AkInstanceBase          base;

  AkBindMaterial         *bindMaterial;
  struct AkInstanceMorph *morpher;
  struct AkInstanceSkin  *skinner;
} AkInstanceGeometry;

typedef struct AkInstanceNode {
  AkInstanceBase base;
  const char    *proxy;
} AkInstanceNode;

/*
 * TODO: separate all instances to individual nodes?
 */
struct AkMatrix;
struct AkBoundingBox;
struct AkTransform;

typedef struct AkBind {
  const char    * semantic;
  const char    * target;
  struct AkBind * next;
} AkBind;

typedef struct AkBindVertexInput {
  struct AkBindVertexInput *next;
  const char               *semantic;
  const char               *inputSemantic;
  AkUInt                    inputSet;
} AkBindVertexInput;

typedef struct AkInstanceMaterial {
  AkInstanceBase       base;
  const char          *symbol;
  AkTechniqueOverride *techniqueOverride;
  AkBind              *bind;
  AkBindVertexInput   *bindVertexInput;
} AkInstanceMaterial;

typedef struct AkRender {
  /* const char * sid; */

  const char     * name;
  const char     * cameraNode;
  AkStringArrayL * layer;
  AkInstanceMaterial * instanceMaterial;
  AkTree         * extra;

  struct AkRender * next;
} AkRender;

typedef struct AkEvaluateScene {
  /* const char * id; */
  /* const char * sid; */

  const char * name;
  AkRender   * render;
  AkTree     * extra;
  AkBool       enable;

  struct AkEvaluateScene * next;
} AkEvaluateScene;

struct AkInstanceList;

typedef struct AkVisualScene {
  /* const char * id; */
  AkOneWayIterBase       base;
  const char            *name;
  struct AkNode         *node;
  struct AkNode         *firstCamNode; /* first found camera       */
  struct AkInstanceList *cameras;      /* all cameras inside scene */
  struct AkInstanceList *lights;       /* all lights inside scene  */
  AkEvaluateScene       *evaluateScene;
  struct AkBoundingBox  *bbox;
  AkTree                *extra;
} AkVisualScene;

typedef struct AkScene {
  /*
   TODO:
      instance_physics_scene
      instance_kinematics_scene
   */
  AkInstanceBase *visualScene;
  AkTree * extra;
} AkScene;

struct AkMorph;
struct AkSkin;

typedef struct AkLibraries {
  struct AkLibrary *cameras;
  struct AkLibrary *lights;
  struct AkLibrary *effects;
  struct AkLibrary *libimages;
  struct AkLibrary *materials;
  struct AkLibrary *geometries;
  struct AkLibrary *controllers;
  struct AkLibrary *visualScenes;
  struct AkLibrary *nodes;
  struct AkLibrary *animations;
  
  struct FListItem *buffers;
  struct FListItem *accessors;
  struct FListItem *textures;
  struct FListItem *samplers;
  struct FListItem *images;
  struct AkMorph   *morphs;
  struct AkSkin    *skins;
} AkLibraries;

typedef const char* (*AkFetchFromURLHandler)(const char * __restrict url);

typedef struct AkDoc {
  AkDocInf   *inf;
  AkCoordSys *coordSys;
  AkUnit     *unit;
  AkTree     *extra;
  void       *reserved;
  AkLibraries lib;
  AkScene     scene;
} AkDoc;

#include "context.h"
#include "geom.h"
#include "image.h"
#include "string.h"
#include "coord-util.h"
#include "library.h"
#include "instance.h"
#include "cam.h"
#include "transform.h"
#include "sid.h"
#include "light.h"
#include "node.h"
#include "texture.h"
#include "animation.h"
#include "controller.h"

AK_EXPORT
AkResult
ak_load(AkDoc     ** __restrict dest,
        const char * __restrict url,
        .../* options */);

AK_EXPORT
void *
ak_getId(void * __restrict objptr);

AK_EXPORT
AkResult
ak_setId(void       * __restrict objptr,
         const char * __restrict objectId);

AK_EXPORT
AkResult
ak_moveId(void * __restrict objptrOld,
          void * __restrict objptrNew);

AK_EXPORT
void *
ak_getObjectById(AkDoc      * __restrict doc,
                 const char * __restrict objectId);

AK_EXPORT
void *
ak_getObjectByUrl(AkURL * __restrict url);

const char*
ak_getFile(const char *url);

char*
ak_getFileFrom(AkDoc *doc, const char *url);

AK_EXPORT
const char *
ak_generatId(AkDoc      * __restrict doc,
             void       * __restrict parentmem,
             const char * __restrict prefix);

AK_EXPORT
void*
ak_getAssetInfo(void * __restrict obj,
                size_t itemOffset);

/* same as: ak_getAssetInfo(obj, offsetof(AkAssetInf, coordSys)) */
AK_EXPORT
AkCoordSys*
ak_getCoordSys(void * __restrict obj);

AK_EXPORT
bool
ak_hasCoordSys(void * __restrict obj);

AK_EXPORT
void
ak_retainURL(void * __restrict obj, AkURL * __restrict url);

AK_EXPORT
void
ak_releaseURL(void * __restrict obj, AkURL * __restrict url);

AK_EXPORT
void
ak_setFetchFromURLHandler(AkFetchFromURLHandler handler);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_h */
