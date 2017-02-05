/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_geom_h
#define ak_geom_h
#ifdef __cplusplus
extern "C" {
#endif

#include "ak-map.h"
#include "ak-bbox.h"

typedef enum AkGeometryType {
  AK_GEOMETRY_TYPE_MESH   = 0,
  AK_GEOMETRY_TYPE_SPLINE = 1,
  AK_GEOMETRY_TYPE_BREP   = 2
} AkGeometryType;

struct AkGeometry;
struct AkMesh;

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
  struct AkMesh          *mesh;
  AkBoundingBox          *bbox; /* per-primitive bbox */
  const char             *name;
  const char             *material;
  AkInput                *input;
  AkVertices             *vertices;
  AkUIntArray            *indices;
  AkTree                 *extra;
  struct AkMeshPrimitive *next;
  size_t                  count;
  uint32_t                inputCount;
  uint32_t                indexStride;
} AkMeshPrimitive;

typedef struct AkLines {
  AkMeshPrimitive base;
  AkLineMode      mode;
} AkLines;

typedef struct AkPolygon {
  AkMeshPrimitive base;
  AkDoubleArrayL *holes;
  AkUIntArray    *vcount;
  AkPolygonMode   mode;
  AkBool          haveHoles;
} AkPolygon;

typedef struct AkTriangles {
  AkMeshPrimitive base;
  AkTriangleMode  mode;
} AkTriangles;

typedef struct AkMesh {
  ak_asset_base
  struct AkGeometry *geom;
  const char        *convexHullOf;
  AkSource          *source;
  AkVertices        *vertices;
  AkMeshPrimitive   *primitive;
  AkBoundingBox     *bbox;
  uint32_t           primitiveCount;
  AkTree            *extra;
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
  /* const char * sid; */

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
  const char        *name;
  AkObject          *gdata;
  AkTree            *extra;
  AkMap             *materialMap;
  AkBoundingBox     *bbox;
  struct AkGeometry *next;
} AkGeometry;

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
 * @param mesh mesh
 *
 * @return boolean
 */
AK_EXPORT
bool
ak_meshPrimNeedsNormals(AkMeshPrimitive * __restrict prim);
#ifdef __cplusplus
}
#endif
#endif /* ak_geom_h */
