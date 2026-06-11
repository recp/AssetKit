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

#ifndef assetkit_geom_h
#define assetkit_geom_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "map.h"
#include "bbox.h"
#include "map.h"
#include "type.h"

struct RBTree;
struct AkGeometry;
struct AkMesh;
struct AkAccessor;
struct FListItem;
struct AkMaterial;
struct AkMaterialBinding;

typedef enum AkGeometryType {
  AK_GEOMETRY_MESH   = 1,
  AK_GEOMETRY_SPLINE = 2,
  AK_GEOMETRY_BREP   = 3
} AkGeometryType;

typedef enum AkGeometryEditFlags {
  AK_GEOM_EDIT_FLAG_ARRAYS  = 1,
  AK_GEOM_EDIT_FLAG_INDICES = 2,
  AK_GEOM_EDIT_FLAG_MUTEX   = 3
} AkGeometryEditFlags;

typedef enum AkMeshPrimitiveType {
  AK_PRIMITIVE_LINES      = 1,
  AK_PRIMITIVE_POLYGONS   = 2,
  AK_PRIMITIVE_TRIANGLES  = 3,
  AK_PRIMITIVE_POINTS     = 4
} AkMeshPrimitiveType;

typedef enum AkMeshPrimitiveFlags {
  AK_MESH_PRIMITIVE_FLAG_NONE           = 0,
  AK_MESH_PRIMITIVE_FLAG_SMOOTH_SHADING = 1u << 0
} AkMeshPrimitiveFlags;

typedef enum AkTriangleMode {
  AK_TRIANGLES      = 1,
  AK_TRIANGLE_STRIP = 3,
  AK_TRIANGLE_FAN   = 5
} AkTriangleMode;

typedef enum AkLineMode {
  AK_LINES      = 1,
  AK_LINE_LOOP  = 3,
  AK_LINE_STRIP = 5
} AkLineMode;

struct AkIndexArray {
  size_t   count;
  AkUInt   max;
  AkTypeId componentType;
  uint32_t padding;
  uint8_t  items[];
};

typedef struct AkVertices {
  /* const char   * id; */
  const char   *name;
  AkTree       *extra;
  AkInput      *input;
  uint32_t      inputCount;
} AkVertices;

/* CSR-style joint counts and offsets. */
typedef struct AkJointDesc {
  uint32_t *counts;
  size_t   *indexes;
  uint32_t  allCount;
} AkJointDesc;

typedef struct AkMeshPrimitive {
  struct AkMeshPrimitive *next;
  struct AkMesh          *mesh;
  AkBoundingBox          *bbox;   /* per-primitive bbox */
  const char             *name;
  const char             *bindmaterial;
  struct AkMaterial      *material;
  struct AkMaterialBinding *materialBindings;
  AkInput                *input;
  AkInput                *pos;
  /*
   glTF normal fast path:
     prim->indices       == NULL;
     prim->indexAccessor != NULL;

   Tuple-indexed or mutable edit path:
     prim->indices       != NULL;
     prim->indexAccessor == NULL;
   */
  AkIndexArray           *indices;
  struct AkAccessor      *indexAccessor;
  AkTree                 *extra;
  void                   *udata;
  AkMeshPrimitiveType     type;
  uint32_t                nPolygons;
  uint32_t                inputCount;
  AkFloat3                center;
  uint32_t                flags;

  /* Number of index slots per vertex tuple; 1 for unified indices. */
  uint32_t                indexStride;
  void                   *reserved;

  /* KHR_materials_variants: optional material overrides. */
  struct AkMaterialVariantMapping *variantMappings;
  uint32_t                         variantMappingCount;
  uint32_t                         materialBindingCount;

  /* KHR_gaussian_splatting: optional splat metadata. Splat inputs stay
     in the normal primitive input chain. */
  struct AkGaussianSplat          *gsplat;
} AkMeshPrimitive;

AK_INLINE
bool
ak_meshPrimitiveSmoothShading(const AkMeshPrimitive * __restrict prim) {
  return prim && (prim->flags & AK_MESH_PRIMITIVE_FLAG_SMOOTH_SHADING);
}

typedef struct AkLines {
  AkMeshPrimitive base;
  AkLineMode      mode;
} AkLines;

typedef struct AkPolygon {
  AkMeshPrimitive base;
  AkDoubleArrayL *holes;
  AkUIntArray    *vcount;
  AkBool          haveHoles;
} AkPolygon;

typedef struct AkTriangles {
  AkMeshPrimitive base;
  AkTriangleMode  mode;
} AkTriangles;

typedef struct AkMeshEditHelper {
  AkGeometryEditFlags flags;
  struct RBTree      *buffers;         /* new buffers               */
  struct AkBufferEditState *bufferList;
  void               *mutex;
  void               *duplicator;
  bool                skipFixIndices;
} AkMeshEditHelper;

typedef struct AkMesh {
  struct AkGeometry *geom;
  const char        *convexHullOf;
  AkMeshPrimitive   *primitive;
  AkBoundingBox     *bbox;
  AkTree            *extra;
  AkMeshEditHelper  *edith;
  struct FListItem  *skins;
  const char        *name;
  AkFloatArray      *weights;
  uint32_t           primitiveCount;
  AkFloat3           center;
} AkMesh;

typedef struct AkSpline {
  struct AkGeometry *geom;
  AkVertices        *cverts;
  AkTree            *extra;
  AkBool             closed;
} AkSpline;

typedef struct AkLine {
  AkFloat3 origin;
  AkFloat3 direction;
  AkTree  *extra;
} AkLine;

typedef struct AkCircle {
  AkFloat radius;
  AkTree *extra;
} AkCircle;

typedef struct AkEllipse {
  AkFloat2 radius;
  AkTree  *extra;
} AkEllipse;

typedef struct AkParabola {
  AkFloat focal;
  AkTree *extra;
} AkParabola;

typedef struct AkHyperbola {
  AkFloat2 radius;
  AkTree  *extra;
} AkHyperbola;

typedef struct AkNurbs {
  AkVertices *cverts;
  AkTree     *extra;
  AkUInt      degree;
  AkBool      closed;
} AkNurbs;

typedef struct AkCurve {
  AkFloatArrayL *orient;
  AkFloat3       origin;
  AkObject       *curve;
  struct AkCurve *next;
} AkCurve;

typedef struct AkCurves {
  AkCurve *curve;
  AkTree  *extra;
} AkCurves;

typedef struct AkCone {
  AkFloat radius;
  AkFloat angle;
  AkTree *extra;
} AkCone;

typedef struct AkPlane {
  AkFloat4 equation;
  AkTree  *extra;
} AkPlane;

typedef struct AkCylinder {
  AkFloat2 radius;
  AkTree  *extra;
} AkCylinder;

typedef struct AkNurbsSurface {
  AkVertices *cverts;
  AkTree     *extra;
  AkUInt      degree_u;
  AkUInt      degree_v;
  AkBool      closed_u;
  AkBool      closed_v;
} AkNurbsSurface;

typedef struct AkSphere {
  AkFloat radius;
  AkTree *extra;
} AkSphere;

typedef struct AkTorus {
  AkFloat2 radius;
  AkTree * extra;
} AkTorus;

typedef struct AkSweptSurface {
  AkCurve *curve;
  AkFloat3 direction;
  AkFloat3 origin;
  AkFloat3 axis;
  AkTree  *extra;
} AkSweptSurface;

typedef struct AkSurface {
  /* const char * sid; */

  const char       *name;
  AkObject         *surface;
  AkFloatArrayL    *orient;
  AkFloat3          origin;
  struct AkSurface *next;
} AkSurface;

typedef struct AkSurfaces {
  AkSurface *surface;
  AkTree    *extra;
} AkSurfaces;

typedef struct AkEdges {
  /* const char    * id; */
  const char    *name;
  AkInput       *input;
  AkUIntArray   *primitives;
  AkTree        *extra;
  AkUInt         count;
  uint32_t       inputCount;
} AkEdges;

typedef struct AkWires {
  /* const char    * id; */
  const char  *name;
  AkInput     *input;
  AkUIntArray *vcount;
  AkUIntArray *primitives;
  AkTree      *extra;
  AkUInt       count;
  uint32_t     inputCount;
} AkWires;

typedef struct AkFaces {
  /* const char    * id; */
  const char  *name;
  AkInput     *input;
  AkUIntArray *vcount;
  AkUIntArray *primitives;
  AkTree      *extra;
  AkUInt       count;
  uint32_t     inputCount;
} AkFaces;

typedef struct AkPCurves {
  /* const char    * id; */
  const char  *name;
  AkInput     *input;
  AkUIntArray *vcount;
  AkUIntArray *primitives;
  AkTree      *extra;
  AkUInt       count;
  uint32_t     inputCount;
} AkPCurves;

typedef struct AkShells {
  /* const char    * id; */
  const char  *name;
  AkInput     *input;
  AkUIntArray *vcount;
  AkUIntArray *primitives;
  AkTree      *extra;
  AkUInt       count;
  uint32_t     inputCount;
} AkShells;

typedef struct AkSolids {
  /* const char    * id; */
  const char  *name;
  AkInput     *input;
  AkUIntArray *vcount;
  AkUIntArray *primitives;
  AkTree      *extra;
  AkUInt       count;
  uint32_t     inputCount;
} AkSolids;

/* Boundary-representation geometry payload. */
typedef struct AkBoundryRep {
  struct AkGeometry *geom;
  AkCurves          *curves;
  AkCurves          *surfaceCurves;
  AkSurfaces        *surfaces;
  AkVertices        *vertices;
  AkEdges           *edges;
  AkWires           *wires;
  AkFaces           *faces;
  AkPCurves         *pcurves;
  AkShells          *shells;
  AkSolids          *solids;
  AkTree            *extra;
} AkBoundryRep;

typedef struct AkGeometry {
  /* const char * id; */
  struct AkGeometry *next;
  const char        *name;
  AkObject          *gdata;
  AkTree            *extra;
  AkMap             *materialMap;
  AkBoundingBox     *bbox;
} AkGeometry;

typedef enum AkMeshIsolateType {
  AK_MESH_ISOLATE_NONE      = 0 << 0,
  AK_MESH_ISOLATE_BUFFERS   = 1 << 1,
  AK_MESH_ISOLATE_ACCESSORS = 2 << 2
} AkMeshIsolateType;

/*!
 * @brief Total input count except VERTEX input
 *
 * returns primitive.inputCount - 1 + primitive.vertices.inputCount
 *
 * @return total input count
 */
AK_EXPORT
uint32_t
ak_meshInputCount(AkMesh * __restrict mesh);

/*!
 * @brief set material (symbol) for primitive
 *        actual material will set with bind_material/instance material
 *
 * @param prim     mesh primitive
 * @param material material
 *
 * @return result
 */
AK_EXPORT
AkResult
ak_meshSetMaterial(AkMeshPrimitive *prim,
                   const char      *material);

/*!
 * @brief triangulate all mesh primitives
 *
 * @param mesh mesh
 *
 * @return new triangles/faces count
 */
AK_EXPORT
uint32_t
ak_meshTriangulate(AkMesh * __restrict mesh);

/*!
 * @brief triangulate polygon
 *
 * @param poly polygon primitive
 *
 * @return new triangles/faces count
 */
AK_EXPORT
uint32_t
ak_meshTriangulatePoly(AkPolygon * __restrict poly);

/*!
 * @brief returns true if least one primitive doesn't have normals
 *
 * @param mesh mesh
 *
 * @return boolean
 */
AK_EXPORT
bool
ak_meshNeedsNormals(AkMesh * __restrict mesh);

/*!
 * @brief returns true if primitive doesn't have normals
 *
 * @param prim primitive
 *
 * @return boolean
 */
AK_EXPORT
bool
ak_meshPrimNeedsNormals(AkMeshPrimitive * __restrict prim);

/*!
 * @brief generate normals for all pritimives of mesh
 *
 * @param mesh mesh
 */
AK_EXPORT
void
ak_meshGenNormals(AkMesh * __restrict mesh);

/*!
 * @brief prepare mesh for edit, or enable edit mode with default attribs
 *
 * @param mesh mesh
 */
AK_EXPORT
void
ak_meshBeginEdit(AkMesh * __restrict mesh);

/*!
 * @brief prepare mesh for edit, or enable edit mode
 *
 * @param mesh  meshs
 * @param flags flags needed while edit mode
 */
AK_EXPORT
void
ak_meshBeginEditA(AkMesh  * __restrict mesh,
                  AkGeometryEditFlags flags);

/*!
 * @brief finish edit, disable edit mode, relese allocated memory for editing
 *
 * @param mesh mesh
 */
AK_EXPORT
void
ak_meshEndEdit(AkMesh * __restrict mesh);

AK_EXPORT
AkIndexArray*
ak_meshIndicesArrayFor(AkMesh          * __restrict mesh,
                       AkMeshPrimitive * __restrict prim,
                       bool                         readonly);

AK_EXPORT
AkTypeId
ak_indexComponentTypeForMax(AkUInt maxIndex);

AK_EXPORT
size_t
ak_indexComponentSize(AkTypeId componentType);

AK_EXPORT
AkIndexArray*
ak_indexArrayAlloc(AkHeap  * __restrict heap,
                   void    * __restrict parent,
                   size_t               count,
                   AkTypeId             componentType);

AK_EXPORT
AkIndexArray*
ak_indexArrayPromote(AkHeap        * __restrict heap,
                     void          * __restrict parent,
                     AkIndexArray  * __restrict indices,
                     AkTypeId                   componentType);

AK_EXPORT
AkUIntArray*
ak_indexArrayMaterializeUInt(AkHeap             * __restrict heap,
                             void               * __restrict parent,
                             const AkIndexArray * __restrict indices);

AK_EXPORT
AkUInt
ak_indexArrayGet(const AkIndexArray * __restrict indices,
                 size_t                          index);

AK_EXPORT
bool
ak_indexArraySet(AkHeap       * __restrict heap,
                 void         * __restrict parent,
                 AkIndexArray ** __restrict indices,
                 size_t                    index,
                 AkUInt                    value);

AK_EXPORT
AkIndexArray*
ak_meshPrimitiveMaterializeIndices(AkMeshPrimitive * __restrict prim);

AK_EXPORT
AkIndexArray*
ak_meshPrimitiveEnsureUIntIndices(AkMeshPrimitive * __restrict prim);

AK_EXPORT
bool
ak_meshPrimitiveIsTupleIndexed(const AkMeshPrimitive * __restrict prim);

AK_EXPORT
bool
ak_meshPrimitiveIsSingleIndexed(const AkMeshPrimitive * __restrict prim);

AK_EXPORT
AkResult
ak_meshPrimitiveEnsureSingleIndex(AkMeshPrimitive * __restrict prim);

AK_EXPORT
struct AkAccessor*
ak_meshPrimitiveSingleIndexAccessor(AkMeshPrimitive * __restrict prim);

AK_EXPORT
struct AkAccessor*
ak_meshPrimitiveIndexAccessor(AkMeshPrimitive * __restrict prim);

AK_EXPORT
AkUInt
ak_indicesMax(const AkIndexArray * __restrict indices);

AK_EXPORT
AkTypeId
ak_indicesComponentType(const AkIndexArray * __restrict indices);

AK_EXPORT
size_t
ak_meshPrimitiveIndexCount(const AkMeshPrimitive * __restrict prim);

AK_EXPORT
AkUInt
ak_meshPrimitiveIndexMax(const AkMeshPrimitive * __restrict prim);

AK_EXPORT
AkTypeId
ak_meshPrimitiveIndexComponentType(const AkMeshPrimitive * __restrict prim);

AK_EXPORT
AkBufferEditState*
ak_meshReserveBuffer(AkMesh * __restrict mesh,
                     void   * __restrict buffid,
                     size_t              itemSize,
                     uint32_t            stride,
                     size_t              acc_count);

AK_EXPORT
void
ak_meshReserveBufferForInput(AkMesh   * __restrict mesh,
                             AkInput  * __restrict input,
                             size_t                count);

AK_EXPORT
void
ak_meshReserveBuffers(AkMesh          * __restrict mesh,
                      AkMeshPrimitive * __restrict prim,
                      size_t                       count);

AK_EXPORT
AkResult
ak_meshFillBuffers(AkMesh * __restrict mesh);

AK_EXPORT
void
ak_moveIndices(AkMesh * __restrict mesh);

AK_EXPORT
void
ak_meshMoveBuffers(AkMesh * __restrict mesh);

AK_EXPORT
AkDuplicator*
ak_meshDuplicatorForIndices(AkMesh          * __restrict mesh,
                            AkMeshPrimitive * __restrict prim);

AK_EXPORT
void
ak_meshFixIndexBuffer(AkMesh          * __restrict mesh,
                      AkMeshPrimitive * __restrict prim,
                      AkDuplicator    * __restrict duplicator);


void
ak_meshReIndexInputs(AkMesh * __restrict mesh);

void
ak_inputNameIndexed(AkInput * __restrict input,
                    char    * __restrict buf);

AK_EXPORT
void
ak_inputNameBySet(AkInput * __restrict input,
                  char    * __restrict buf);

AK_EXPORT
AkInput*
ak_meshInputGet(AkMeshPrimitive *prim,
                const char      *inputSemantic,
                uint32_t         set);

AK_EXPORT
bool
ak_meshIsIsolated(AkMesh *mesh);

/*!
 * @brief current isolation rule
 *
 * 1. Separated buffer per accessor
 * 2. Separated accessor per input
 *
 * @return true if the rules are match
 */
AK_EXPORT
bool
ak_meshIsPrimIsolated(AkMeshPrimitive *prim);

AK_EXPORT
void
ak_meshIsolate(AkMesh *mesh);

AK_EXPORT
void
ak_meshIsolatePrim(AkMeshPrimitive *prim);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_geom_h */
