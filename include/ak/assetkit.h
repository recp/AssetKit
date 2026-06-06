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
  AK_FILE_TYPE_3MF       = 6,
  AK_FILE_TYPE_X3D       = 7,
  AK_FILE_TYPE_USD       = 8,
  AK_FILE_TYPE_ALEMBIC   = 9,

  AK_FILE_TYPE_DAE       = AK_FILE_TYPE_COLLADA,
  AK_FILE_TYPE_OBJ       = AK_FILE_TYPE_WAVEFRONT
} AkFileType;

typedef enum AkAltitudeMode {
  AK_ALTITUDE_RELATIVETOGROUND = 0,
  AK_ALTITUDE_ABSOLUTE         = 1
} AkAltitudeMode;

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
  AK_INSTANCE_CAMERA     = 2,
  AK_INSTANCE_LIGHT      = 3,
  AK_INSTANCE_GEOMETRY   = 4,
  AK_INSTANCE_IMAGE      = 5,
  AK_INSTANCE_CONTROLLER = 6
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

/*!
 * @brief Return optional metadata attached to an AssetKit object.
 *
 * COLLADA <extra>, glTF extensions and preserved glTF extras are exposed
 * as AkTree. The returned tree is owned by the document heap.
 */
AK_EXPORT
AkTree*
ak_extra(void * __restrict obj);

/*!
 * @brief Attach optional metadata to an AssetKit object.
 */
AK_EXPORT
void
ak_extra_set(void   * __restrict obj, AkTree * __restrict extra);

#include "source.h"

typedef struct AkUnit {
  const char * name;
  double       dist;
} AkUnit;

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

struct AkNode;
typedef struct AkCamera AkCamera;
typedef struct AkLight  AkLight;

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

#include "material.h"

struct AkAccessor;

typedef struct AkInput {
  const char        *semanticRaw;
  struct AkInput    *next;
  struct AkAccessor *accessor;
  void              *reserved; /* private member */

  uint32_t           index; /* TEXCOORD0, TEXCOORD1... */
  bool               isIndexed;
  AkInputSemantic    semantic;
  uint32_t           indexOffset;
  uint32_t           set;
  
  /* TODO: WILL BE DELETED */
//  AkURL              source;
} AkInput;

typedef struct AkInstanceGeometry {
  AkInstanceBase          base;
  AkMaterialBinding      *objectBindings;
  struct AkInstanceMorph *morpher;
  struct AkInstanceSkin  *skinner;
} AkInstanceGeometry;

typedef struct AkInstanceNode {
  struct AkInstanceNode *next;
  struct AkInstanceNode *prev;
  struct AkNode         *owner;
  struct AkNode         *target;
  AkURL                 *reserved; /* unresolved/source-side URL */
  const char            *name;
  const char            *proxy;
} AkInstanceNode;

/*
 * TODO: separate all instances to individual nodes?
 */
struct AkMatrix;
struct AkBoundingBox;
struct AkTransform;

typedef struct AkSceneCamera {
  struct AkSceneCamera *next;
  AkCamera             *camera;
  AkInstanceBase       *firstInstance;
  uint32_t              useCount;
} AkSceneCamera;

typedef struct AkSceneLight {
  struct AkSceneLight *next;
  AkLight             *light;
  AkInstanceBase      *firstInstance;
  uint32_t             useCount;
} AkSceneLight;

typedef struct AkSceneCameraList {
  AkSceneCamera *first;
  AkSceneCamera *last;
  uint32_t       count;
  uint32_t       useCount;
} AkSceneCameraList;

typedef struct AkSceneLightList {
  AkSceneLight *first;
  AkSceneLight *last;
  uint32_t      count;
  uint32_t      useCount;
} AkSceneLightList;

typedef struct AkScene {
  struct AkScene        *next;
  const char            *name;
  struct AkNode         *node;
  struct AkNode         *firstCamNode; /* first found camera use */
  AkSceneCameraList      cameras;      /* unique cameras used by scene */
  AkSceneLightList       lights;       /* unique lights used by scene */
  struct AkBoundingBox  *bbox;
  AkTree                *extra;
} AkScene;

typedef struct AkMorph      AkMorph;
typedef struct AkSkin       AkSkin;
typedef struct AkGeometry   AkGeometry;
typedef struct AkMaterial   AkMaterial;
typedef struct AkAnimation  AkAnimation;
typedef struct AkBuffer     AkBuffer;
typedef struct AkAccessor   AkAccessor;
typedef struct AkTexture    AkTexture;
typedef struct AkSampler    AkSampler;
typedef struct AkImage      AkImage;

#define AK_DEFINE_LIB(T, Name)                                                \
  typedef struct Name {                                                       \
    T       *first;                                                           \
    T       *last;                                                            \
    uint32_t count;                                                           \
  } Name

AK_DEFINE_LIB(AkCamera,      AkCameraLib);
AK_DEFINE_LIB(AkLight,       AkLightLib);
AK_DEFINE_LIB(AkMaterial,    AkMaterialLib);
AK_DEFINE_LIB(AkGeometry,    AkGeometryLib);
AK_DEFINE_LIB(AkScene,       AkSceneLib);
AK_DEFINE_LIB(struct AkNode, AkNodeLib);
AK_DEFINE_LIB(AkAnimation,   AkAnimationLib);
AK_DEFINE_LIB(AkBuffer,      AkBufferLib);
AK_DEFINE_LIB(AkAccessor,    AkAccessorLib);
AK_DEFINE_LIB(AkTexture,     AkTextureLib);
AK_DEFINE_LIB(AkSampler,     AkSamplerLib);
AK_DEFINE_LIB(AkImage,       AkImageLib);
AK_DEFINE_LIB(AkMorph,       AkMorphLib);
AK_DEFINE_LIB(AkSkin,        AkSkinLib);

#undef AK_DEFINE_LIB

typedef struct AkLibrary {
  AkCameraLib      cameras;
  AkLightLib       lights;
  AkMaterialLib    materials;
  AkGeometryLib    geometries;
  AkSceneLib       scenes;
  AkNodeLib        nodes;
  AkAnimationLib   animations;

  AkBufferLib      buffers;
  AkAccessorLib    accessors;
  AkTextureLib     textures;
  AkSamplerLib     samplers;
  AkImageLib       images;
  AkMorphLib       morphs;
  AkSkinLib        skins;
} AkLibrary;

typedef const char* (*AkFetchFromURLHandler)(const char * __restrict url);

typedef struct AkDoc {
  AkDocInf   *inf;
  AkCoordSys *coordSys;
  AkUnit     *unit;
  AkTree     *extra;
  void       *reserved;
  void       *userData;
  float       loadMillis;
  AkLibrary   lib;
  AkScene    *scene;

  /* KHR_materials_variants: document-level variant names. */
  struct AkMaterialVariant *materialVariants;
  uint32_t                  materialVariantCount;

  /* 3MF/X3D/USD-style document-level material/property groups. */
  AkMaterialPropertyRegistry materialProperties;
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
#include "gsplat.h"

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
