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

#include "../common.h"
#include "../io/common/primitive.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AK_PRINT_VALIDATE_MAX_NODE_DEPTH 128u

typedef IOFloatRows AkPrintPositionRows;

typedef struct AkPrintEdge {
  uint32_t a;
  uint32_t b;
} AkPrintEdge;

typedef struct AkPrintMeshValidation {
  AkPrintValidationFlags checks;
  AkPrintValidationFlags found;
  AkPrintEdge           *edges;
  size_t                 edgeCount;
  size_t                 edgeCapacity;
} AkPrintMeshValidation;

static
const char*
ak_print_strdup(AkHeap * __restrict heap,
                void   * __restrict parent,
                const char * __restrict str) {
  return str && heap ? ak_heap_strdup(heap, parent, str) : NULL;
}

static
void
ak_print_identity_matrix(float matrix[16]) {
  matrix[0] = 1.0f; matrix[1] = 0.0f; matrix[2] = 0.0f; matrix[3] = 0.0f;
  matrix[4] = 0.0f; matrix[5] = 1.0f; matrix[6] = 0.0f; matrix[7] = 0.0f;
  matrix[8] = 0.0f; matrix[9] = 0.0f; matrix[10] = 1.0f; matrix[11] = 0.0f;
  matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

static
void
ak_print_copy_matrix(float                       dst[16],
                     const float * __restrict   src) {
  if (src)
    memcpy(dst, src, sizeof(float) * 16u);
  else
    ak_print_identity_matrix(dst);
}

#define AK_PRINT_APPEND(PRINT, FIRST, LAST, ITEM, COUNT)                      \
  do {                                                                        \
    AK_APPEND_FLINK((PRINT)->FIRST, (PRINT)->LAST, (ITEM));                   \
    (PRINT)->COUNT++;                                                         \
  } while (0)

AK_EXPORT
AkPrintDocument*
ak_printDocument(AkDoc * __restrict doc) {
  AkDocPrivate *priv;

  priv = ak__docPrivate(doc, false);
  return priv ? priv->print : NULL;
}

AK_EXPORT
AkPrintDocument*
ak_printDocumentEnsure(AkDoc * __restrict doc) {
  AkDocPrivate    *priv;
  AkHeap          *heap;
  AkPrintDocument *print;

  priv = ak__docPrivate(doc, true);
  if (!priv)
    return NULL;

  if (priv->print)
    return priv->print;

  heap = ak_heap_getheap(priv);
  if (!heap)
    return NULL;

  print = ak_heap_calloc(heap, priv, sizeof(*print));
  if (!print)
    return NULL;

  priv->print = print;
  return print;
}

AK_EXPORT
bool
ak_printHasFeature(const AkPrintDocument * __restrict print,
                   AkPrintFeatureFlags                 features) {
  return print && (print->features & features) == features;
}

AK_EXPORT
void
ak_printSetFeature(AkPrintDocument   * __restrict print,
                   AkPrintFeatureFlags             features) {
  if (print)
    print->features |= features;
}

AK_EXPORT
void
ak_printSetUnsupportedFeature(AkPrintDocument   * __restrict print,
                              AkPrintFeatureFlags             features) {
  if (!print)
    return;

  print->unsupportedFeatures |= features;
  print->validationFlags     |= AK_PRINT_VALIDATION_UNSUPPORTED_FEATURE;
}

static
bool
ak_print_validate_rows_init(AkPrintPositionRows * __restrict rows,
                            AkAccessor          * __restrict acc) {
  if (!acc
      || !acc->buffer
      || !acc->buffer->data
      || acc->count == 0
      || acc->componentCount < 3u)
    return false;

  return io_float_rows_init(rows, acc);
}

static
void
ak_print_validate_rows_destroy(AkPrintPositionRows * __restrict rows) {
  io_float_rows_destroy(rows);
  memset(rows, 0, sizeof(*rows));
}

static
bool
ak_print_validate_position(AkPrintPositionRows * __restrict rows,
                           uint32_t                         index,
                           float                            out[3]) {
  const float *row;

  if (!rows || !rows->accessor || index >= rows->accessor->count)
    return false;

  row = io_float_rows_get(rows, index);

  out[0] = row[0];
  out[1] = row[1];
  out[2] = row[2];

  return isfinite(out[0]) && isfinite(out[1]) && isfinite(out[2]);
}

static
uint32_t
ak_print_validate_triangle_count(AkMeshPrimitive * __restrict prim) {
  uint32_t vertexCount;

  if (!prim)
    return 0;

  vertexCount = io_primitive_vertex_count(prim);
  if (prim->type == AK_PRIMITIVE_TRIANGLES) {
    AkTriangleMode mode;

    mode = ((AkTriangles *)prim)->mode;
    if (mode == 0)
      mode = AK_TRIANGLES;
    if (vertexCount < 3u)
      return 0;
    if (mode == AK_TRIANGLE_STRIP || mode == AK_TRIANGLE_FAN)
      return vertexCount - 2u;
    return vertexCount / 3u;
  }

  if (prim->type == AK_PRIMITIVE_POLYGONS) {
    AkPolygon *poly;
    size_t     i;
    uint32_t   count;

    poly  = (AkPolygon *)prim;
    count = 0u;
    if (!poly->vcount)
      return 0;
    for (i = 0; i < poly->vcount->count; i++) {
      uint32_t vc;

      vc = poly->vcount->items[i];
      if (vc >= 3u)
        count += vc - 2u;
    }
    return count;
  }

  return 0;
}

static
bool
ak_print_validate_prepare_edges(AkPrintMeshValidation * __restrict v,
                                AkMeshPrimitive       * __restrict prim) {
  uint32_t triCount;

  if ((v->checks & (AK_PRINT_VALIDATION_OPEN_BOUNDARY
                    | AK_PRINT_VALIDATION_NON_MANIFOLD)) == 0u)
    return true;

  triCount = ak_print_validate_triangle_count(prim);
  if (triCount == 0u)
    return true;
#if SIZE_MAX <= UINT32_MAX
  if ((size_t)triCount > (SIZE_MAX / 3u) / sizeof(*v->edges))
    return false;
#endif

  v->edgeCapacity = (size_t)triCount * 3u;
  v->edges        = malloc(sizeof(*v->edges) * v->edgeCapacity);
  return v->edges != NULL;
}

static
void
ak_print_validate_edge(AkPrintMeshValidation * __restrict v,
                       uint32_t                           a,
                       uint32_t                           b) {
  AkPrintEdge *edge;

  if (!v->edges || v->edgeCount >= v->edgeCapacity)
    return;

  edge = &v->edges[v->edgeCount++];
  if (a < b) {
    edge->a = a;
    edge->b = b;
  } else {
    edge->a = b;
    edge->b = a;
  }
}

static
int
ak_print_edge_cmp(const void * __restrict lhs,
                  const void * __restrict rhs) {
  const AkPrintEdge *a;
  const AkPrintEdge *b;

  a = lhs;
  b = rhs;
  if (a->a < b->a) return -1;
  if (a->a > b->a) return 1;
  if (a->b < b->b) return -1;
  if (a->b > b->b) return 1;
  return 0;
}

static
bool
ak_print_validate_degenerate(const float a[3],
                             const float b[3],
                             const float c[3]) {
  double abx, aby, abz;
  double acx, acy, acz;
  double cx, cy, cz;
  double area2;

  abx = (double)b[0] - (double)a[0];
  aby = (double)b[1] - (double)a[1];
  abz = (double)b[2] - (double)a[2];
  acx = (double)c[0] - (double)a[0];
  acy = (double)c[1] - (double)a[1];
  acz = (double)c[2] - (double)a[2];

  cx = aby * acz - abz * acy;
  cy = abz * acx - abx * acz;
  cz = abx * acy - aby * acx;
  area2 = cx * cx + cy * cy + cz * cz;

  return area2 == 0.0;
}

static
void
ak_print_validate_triangle(AkPrintMeshValidation * __restrict v,
                           AkPrintPositionRows   * __restrict rows,
                           AkMeshPrimitive       * __restrict prim,
                           AkInput               * __restrict posInput,
                           uint32_t                           v0,
                           uint32_t                           v1,
                           uint32_t                           v2) {
  AkUInt idx0;
  AkUInt idx1;
  AkUInt idx2;
  float  p0[3];
  float  p1[3];
  float  p2[3];
  bool   degenerate;

  idx0 = io_primitive_input_index(prim, posInput, v0);
  idx1 = io_primitive_input_index(prim, posInput, v1);
  idx2 = io_primitive_input_index(prim, posInput, v2);

  degenerate = idx0 == idx1
               || idx1 == idx2
               || idx2 == idx0
               || idx0 > UINT32_MAX
               || idx1 > UINT32_MAX
               || idx2 > UINT32_MAX
               || !ak_print_validate_position(rows, (uint32_t)idx0, p0)
               || !ak_print_validate_position(rows, (uint32_t)idx1, p1)
               || !ak_print_validate_position(rows, (uint32_t)idx2, p2)
               || ak_print_validate_degenerate(p0, p1, p2);

  if (degenerate) {
    v->found |= AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES;
    return;
  }

  ak_print_validate_edge(v, (uint32_t)idx0, (uint32_t)idx1);
  ak_print_validate_edge(v, (uint32_t)idx1, (uint32_t)idx2);
  ak_print_validate_edge(v, (uint32_t)idx2, (uint32_t)idx0);
}

static
void
ak_print_validate_triangle_primitive(AkPrintMeshValidation * __restrict v,
                                     AkPrintPositionRows   * __restrict rows,
                                     AkMeshPrimitive       * __restrict prim,
                                     AkInput               * __restrict posInput) {
  IOTriangleIter iter;
  uint32_t       tri[3];

  if (!io_triangle_iter_init(&iter, prim))
    return;

  while (io_triangle_iter_next(&iter, tri))
    ak_print_validate_triangle(v, rows, prim, posInput, tri[0], tri[1], tri[2]);
}

static
void
ak_print_validate_polygon_primitive(AkPrintMeshValidation * __restrict v,
                                    AkPrintPositionRows   * __restrict rows,
                                    AkMeshPrimitive       * __restrict prim,
                                    AkInput               * __restrict posInput) {
  AkPolygon *poly;
  size_t     cursor;
  size_t     i;

  poly = (AkPolygon *)prim;
  if (!poly->vcount)
    return;

  cursor = 0u;
  for (i = 0u; i < poly->vcount->count; i++) {
    uint32_t vc;
    uint32_t j;

    vc = poly->vcount->items[i];
    if (vc < 3u) {
      cursor += vc;
      continue;
    }

    for (j = 1u; j + 1u < vc; j++)
      ak_print_validate_triangle(v,
                                 rows,
                                 prim,
                                 posInput,
                                 (uint32_t)cursor,
                                 (uint32_t)(cursor + j),
                                 (uint32_t)(cursor + j + 1u));
    cursor += vc;
  }
}

static
void
ak_print_validate_finalize_edges(AkPrintMeshValidation * __restrict v) {
  size_t i;

  if (!v->edges || v->edgeCount == 0u)
    return;

  qsort(v->edges, v->edgeCount, sizeof(*v->edges), ak_print_edge_cmp);

  i = 0u;
  while (i < v->edgeCount) {
    size_t j;
    size_t count;

    j = i + 1u;
    while (j < v->edgeCount
           && v->edges[j].a == v->edges[i].a
           && v->edges[j].b == v->edges[i].b)
      j++;

    count = j - i;
    if (count == 1u)
      v->found |= AK_PRINT_VALIDATION_OPEN_BOUNDARY;
    else if (count > 2u)
      v->found |= AK_PRINT_VALIDATION_NON_MANIFOLD;

    i = j;
  }
}

static
AkPrintValidationFlags
ak_print_validate_primitive(AkMeshPrimitive      * __restrict prim,
                            AkPrintValidationFlags           checks) {
  AkPrintMeshValidation v;
  AkPrintPositionRows   rows;
  AkInput              *posInput;

  if (!prim
      || (prim->type != AK_PRIMITIVE_TRIANGLES
          && prim->type != AK_PRIMITIVE_POLYGONS))
    return AK_PRINT_VALIDATION_NONE;

  posInput = io_primitive_find_input(prim, AK_INPUT_POSITION);
  if (!posInput || !posInput->accessor)
    return AK_PRINT_VALIDATION_NONE;
  if (!ak_print_validate_rows_init(&rows, posInput->accessor))
    return AK_PRINT_VALIDATION_NONE;

  memset(&v, 0, sizeof(v));
  v.checks = checks;
  if (ak_print_validate_prepare_edges(&v, prim)) {
    if (prim->type == AK_PRIMITIVE_TRIANGLES)
      ak_print_validate_triangle_primitive(&v, &rows, prim, posInput);
    else
      ak_print_validate_polygon_primitive(&v, &rows, prim, posInput);
    ak_print_validate_finalize_edges(&v);
  }

  free(v.edges);
  ak_print_validate_rows_destroy(&rows);

  return v.found & checks;
}

static
bool
ak_print_validate_mesh_instance_negative_scale(AkInstanceGeometry * __restrict inst,
                                               mat4                             world) {
  AkGeometry *geom;
  void       *obj;

  if (!inst)
    return false;

  obj  = ak_instanceObject(&inst->base);
  geom = obj;
  return geom
         && geom->gdata
         && geom->gdata->type == AK_GEOMETRY_MESH
         && glm_mat4_det(world) < 0.0f;
}

static
bool
ak_print_validate_node_negative_scale(AkNode   * __restrict node,
                                      mat4                   parentWorld,
                                      uint32_t               depth) {
  AkInstanceBase *base;
  AkInstanceNode *nodeRef;
  AkNode         *child;
  AkMatrix        localMatrix;
  mat4            world;

  if (!node)
    return false;
  if (depth > AK_PRINT_VALIDATE_MAX_NODE_DEPTH)
    return false;

  ak_transformCombine(node->transform, localMatrix.val[0]);
  glm_mat4_mul(parentWorld, localMatrix.val, world);

  for (base = node->geometry ? &node->geometry->base : NULL;
       base;
       base = base->next) {
    if (base->type == AK_INSTANCE_GEOMETRY
        && ak_print_validate_mesh_instance_negative_scale(
             (AkInstanceGeometry *)base, world))
      return true;
  }

  for (child = node->chld; child; child = child->next) {
    if (ak_print_validate_node_negative_scale(child, world, depth + 1u))
      return true;
  }

  for (nodeRef = node->node; nodeRef; nodeRef = nodeRef->next) {
    AkNode *target;

    target = ak_instanceNodeTarget(nodeRef);
    if (target
        && ak_print_validate_node_negative_scale(target, world, depth + 1u))
      return true;
  }

  return false;
}

static
AkPrintValidationFlags
ak_print_validate_scene_transforms(AkDoc * __restrict doc) {
  mat4 identity;

  if (!doc || !doc->scene || !doc->scene->node)
    return AK_PRINT_VALIDATION_NONE;

  glm_mat4_identity(identity);
  return ak_print_validate_node_negative_scale(doc->scene->node, identity, 0u)
         ? AK_PRINT_VALIDATION_NEGATIVE_SCALE
         : AK_PRINT_VALIDATION_NONE;
}

AK_EXPORT
AkPrintValidationFlags
ak_printValidate(AkDoc                 * __restrict doc,
                 AkPrintValidationFlags             checks) {
  static const AkPrintValidationFlags implemented =
    AK_PRINT_VALIDATION_NON_MANIFOLD
    | AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES
    | AK_PRINT_VALIDATION_OPEN_BOUNDARY
    | AK_PRINT_VALIDATION_NEGATIVE_SCALE;
  static const AkPrintValidationFlags meshChecks =
    AK_PRINT_VALIDATION_NON_MANIFOLD
    | AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES
    | AK_PRINT_VALIDATION_OPEN_BOUNDARY;
  AkPrintDocument      *print;
  AkPrintValidationFlags found;
  AkPrintValidationFlags activeMeshChecks;
  AkGeometry           *geom;

  if (!doc)
    return AK_PRINT_VALIDATION_NONE;
  if (checks == AK_PRINT_VALIDATION_NONE)
    checks = implemented;
  else
    checks &= implemented;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return AK_PRINT_VALIDATION_NONE;

  found = AK_PRINT_VALIDATION_NONE;
  activeMeshChecks = checks & meshChecks;
  if (activeMeshChecks != 0u) {
    for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
      AkMesh          *mesh;
      AkMeshPrimitive *prim;

      if (!geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
        continue;

      mesh = ak_objGet(geom->gdata);
      for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next)
        found |= ak_print_validate_primitive(prim, activeMeshChecks);
    }
  }

  if ((checks & AK_PRINT_VALIDATION_NEGATIVE_SCALE) != 0u)
    found |= ak_print_validate_scene_transforms(doc);

  print->validationFlags &= ~checks;
  print->validationFlags |= found;

  return found;
}

AK_EXPORT
AkPrintPackagePart*
ak_printAddPackagePart(AkDoc                  * __restrict doc,
                       AkPrintPackagePartType              type,
                       const char            * __restrict name,
                       const char            * __restrict contentType,
                       const char            * __restrict relationshipType) {
  AkPrintDocument    *print;
  AkPrintPackagePart *part;
  AkHeap             *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  part = ak_heap_calloc(heap, print, sizeof(*part));
  if (!part)
    return NULL;

  part->type = type;
  if (name)
    part->name = ak_heap_strdup(heap, part, name);
  if (contentType)
    part->contentType = ak_heap_strdup(heap, part, contentType);
  if (relationshipType)
    part->relationshipType = ak_heap_strdup(heap, part, relationshipType);

  AK_PRINT_APPEND(print, parts, lastPart, part, packagePartCount);
  print->features |= AK_PRINT_FEATURE_PACKAGE;

  return part;
}

AK_EXPORT
bool
ak_printSetPackagePartData(AkDoc              * __restrict doc,
                           AkPrintPackagePart * __restrict part,
                           const void         * __restrict data,
                           size_t                          size) {
  AkHeap *heap;
  void   *copy;

  if (!doc || !part)
    return false;
  if (!data && size > 0u)
    return false;

  part->data = NULL;
  part->size = 0u;
  if (size == 0u)
    return true;

  heap = ak_heap_getheap(doc);
  if (!heap)
    return false;

  copy = ak_heap_alloc(heap, part, size);
  if (!copy)
    return false;

  memcpy(copy, data, size);
  part->data = copy;
  part->size = size;
  return true;
}

AK_EXPORT
AkPrintPackagePart*
ak_printAddPackagePartData(AkDoc                  * __restrict doc,
                           AkPrintPackagePartType              type,
                           const char            * __restrict name,
                           const char            * __restrict contentType,
                           const char            * __restrict relationshipType,
                           const void            * __restrict data,
                           size_t                             size) {
  AkPrintPackagePart *part;

  part = ak_printAddPackagePart(doc, type, name, contentType, relationshipType);
  if (!part)
    return NULL;
  if (!ak_printSetPackagePartData(doc, part, data, size))
    return NULL;

  return part;
}

AK_EXPORT
bool
ak_printSetPackagePartRelationship(AkDoc              * __restrict doc,
                                   AkPrintPackagePart * __restrict part,
                                   const char         * __restrict relationshipId,
                                   const char         * __restrict targetMode) {
  AkHeap *heap;

  if (!doc || !part)
    return false;

  heap = ak_heap_getheap(doc);
  if (!heap)
    return false;

  part->relationshipId         = NULL;
  part->relationshipTargetMode = NULL;
  if (relationshipId)
    part->relationshipId = ak_heap_strdup(heap, part, relationshipId);
  if (targetMode)
    part->relationshipTargetMode = ak_heap_strdup(heap, part, targetMode);

  return (!relationshipId || part->relationshipId)
         && (!targetMode || part->relationshipTargetMode);
}

AK_EXPORT
AkPrintProductionItem*
ak_printAddProductionItem(AkDoc                  * __restrict doc,
                          AkPrintProductionItemType           type,
                          const char            * __restrict uuid,
                          const char            * __restrict path,
                          const char            * __restrict partNumber,
                          const char            * __restrict modelResolution,
                          uint32_t                           objectId,
                          uint32_t                           parentObjectId) {
  AkPrintDocument       *print;
  AkPrintProductionItem *item;
  AkHeap                *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  item = ak_heap_calloc(heap, print, sizeof(*item));
  if (!item)
    return NULL;

  item->type           = type;
  item->objectId       = objectId;
  item->parentObjectId = parentObjectId;
  if (uuid)
    item->uuid = ak_heap_strdup(heap, item, uuid);
  if (path)
    item->path = ak_heap_strdup(heap, item, path);
  if (partNumber)
    item->partNumber = ak_heap_strdup(heap, item, partNumber);
  if (modelResolution)
    item->modelResolution = ak_heap_strdup(heap, item, modelResolution);

  AK_PRINT_APPEND(print,
                  productionItems,
                  lastProductionItem,
                  item,
                  productionItemCount);
  print->features |= AK_PRINT_FEATURE_PRODUCTION;

  return item;
}

AK_EXPORT
AkPrintSliceStack*
ak_printAddSliceStack(AkDoc      * __restrict doc,
                      const char * __restrict path,
                      uint32_t                id,
                      float                   zBottom) {
  AkPrintDocument  *print;
  AkPrintSliceStack *stack;
  AkHeap           *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  stack = ak_heap_calloc(heap, print, sizeof(*stack));
  if (!stack)
    return NULL;

  stack->id      = id;
  stack->zBottom = zBottom;
  stack->path    = ak_print_strdup(heap, stack, path);

  AK_PRINT_APPEND(print, sliceStacks, lastSliceStack, stack, sliceStackCount);
  print->features |= AK_PRINT_FEATURE_SLICE;

  return stack;
}

AK_EXPORT
AkPrintSliceRef*
ak_printAddSliceRef(AkDoc      * __restrict doc,
                    const char * __restrict path,
                    uint32_t                stackId,
                    float                   zTop) {
  AkPrintDocument *print;
  AkPrintSliceRef *ref;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  ref = ak_heap_calloc(heap, print, sizeof(*ref));
  if (!ref)
    return NULL;

  ref->stackId = stackId;
  ref->zTop    = zTop;
  ref->path    = ak_print_strdup(heap, ref, path);

  AK_PRINT_APPEND(print, sliceRefs, lastSliceRef, ref, sliceRefCount);
  print->features |= AK_PRINT_FEATURE_SLICE;

  return ref;
}

AK_EXPORT
AkPrintSlice*
ak_printAddSlice(AkDoc      * __restrict doc,
                 const char * __restrict path,
                 uint32_t                stackId,
                 float                   zTop,
                 uint32_t                vertexCount,
                 uint32_t                polygonCount,
                 uint32_t                segmentCount) {
  AkPrintDocument *print;
  AkPrintSlice    *slice;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  slice = ak_heap_calloc(heap, print, sizeof(*slice));
  if (!slice)
    return NULL;

  slice->path         = ak_print_strdup(heap, slice, path);
  slice->stackId      = stackId;
  slice->zTop         = zTop;
  slice->vertexCount  = vertexCount;
  slice->polygonCount = polygonCount;
  slice->segmentCount = segmentCount;

  AK_PRINT_APPEND(print, slices, lastSlice, slice, sliceCount);
  print->features |= AK_PRINT_FEATURE_SLICE;

  return slice;
}

AK_EXPORT
AkPrintSliceObject*
ak_printAddSliceObject(AkDoc      * __restrict doc,
                       const char * __restrict path,
                       const char * __restrict slicePath,
                       const char * __restrict meshResolution,
                       uint32_t                objectId,
                       uint32_t                sliceStackId) {
  AkPrintDocument  *print;
  AkPrintSliceObject *object;
  AkHeap           *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  object = ak_heap_calloc(heap, print, sizeof(*object));
  if (!object)
    return NULL;

  object->path           = ak_print_strdup(heap, object, path);
  object->slicePath      = ak_print_strdup(heap, object, slicePath);
  object->meshResolution = ak_print_strdup(heap, object, meshResolution);
  object->objectId       = objectId;
  object->sliceStackId   = sliceStackId;

  AK_PRINT_APPEND(print,
                  sliceObjects,
                  lastSliceObject,
                  object,
                  sliceObjectCount);
  print->features |= AK_PRINT_FEATURE_SLICE;

  return object;
}

AK_EXPORT
AkPrintBeamLattice*
ak_printAddBeamLattice(AkDoc      * __restrict doc,
                       const char * __restrict path,
                       uint32_t                objectId,
                       float                   minLength,
                       float                   radius,
                       const char * __restrict clippingMode,
                       const char * __restrict cap,
                       const char * __restrict ballMode,
                       float                   ballRadius,
                       uint32_t                clippingMesh,
                       uint32_t                representationMesh,
                       uint32_t                pid,
                       uint32_t                pindex,
                       uint32_t                flags) {
  AkPrintDocument    *print;
  AkPrintBeamLattice *lattice;
  AkHeap             *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  lattice = ak_heap_calloc(heap, print, sizeof(*lattice));
  if (!lattice)
    return NULL;

  lattice->path               = ak_print_strdup(heap, lattice, path);
  lattice->clippingMode       = ak_print_strdup(heap, lattice, clippingMode);
  lattice->cap                = ak_print_strdup(heap, lattice, cap);
  lattice->ballMode           = ak_print_strdup(heap, lattice, ballMode);
  lattice->objectId           = objectId;
  lattice->minLength          = minLength;
  lattice->radius             = radius;
  lattice->ballRadius         = ballRadius;
  lattice->clippingMesh       = clippingMesh;
  lattice->representationMesh = representationMesh;
  lattice->pid                = pid;
  lattice->pindex             = pindex;
  lattice->flags              = flags;

  AK_PRINT_APPEND(print,
                  beamLattices,
                  lastBeamLattice,
                  lattice,
                  beamLatticeCount);
  print->features |= AK_PRINT_FEATURE_BEAM_LATTICE;

  return lattice;
}

AK_EXPORT
AkPrintBeam*
ak_printAddBeam(AkDoc               * __restrict doc,
                AkPrintBeamLattice  * __restrict lattice,
                uint32_t                         v1,
                uint32_t                         v2,
                float                            r1,
                float                            r2,
                uint32_t                         p1,
                uint32_t                         p2,
                uint32_t                         pid,
                const char          * __restrict cap1,
                const char          * __restrict cap2,
                uint32_t                         flags) {
  AkPrintDocument *print;
  AkPrintBeam     *beam;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  beam = ak_heap_calloc(heap, print, sizeof(*beam));
  if (!beam)
    return NULL;

  beam->cap1  = ak_print_strdup(heap, beam, cap1);
  beam->cap2  = ak_print_strdup(heap, beam, cap2);
  beam->v1    = v1;
  beam->v2    = v2;
  beam->r1    = r1;
  beam->r2    = r2;
  beam->p1    = p1;
  beam->p2    = p2;
  beam->pid   = pid;
  beam->flags = flags;

  AK_PRINT_APPEND(print, beams, lastBeam, beam, beamCount);
  if (lattice)
    lattice->beamCount++;
  print->features |= AK_PRINT_FEATURE_BEAM_LATTICE;

  return beam;
}

AK_EXPORT
AkPrintBeamBall*
ak_printAddBeamBall(AkDoc               * __restrict doc,
                    AkPrintBeamLattice  * __restrict lattice,
                    uint32_t                         vindex,
                    float                            radius,
                    uint32_t                         p,
                    uint32_t                         pid,
                    uint32_t                         flags) {
  AkPrintDocument *print;
  AkPrintBeamBall *ball;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  ball = ak_heap_calloc(heap, print, sizeof(*ball));
  if (!ball)
    return NULL;

  ball->vindex = vindex;
  ball->radius = radius;
  ball->p      = p;
  ball->pid    = pid;
  ball->flags  = flags;

  AK_PRINT_APPEND(print, beamBalls, lastBeamBall, ball, beamBallCount);
  if (lattice)
    lattice->ballCount++;
  print->features |= AK_PRINT_FEATURE_BEAM_LATTICE;

  return ball;
}

AK_EXPORT
AkPrintBeamSet*
ak_printAddBeamSet(AkDoc               * __restrict doc,
                   AkPrintBeamLattice  * __restrict lattice,
                   const char          * __restrict name,
                   const char          * __restrict identifier,
                   uint32_t                         refCount,
                   uint32_t                         ballRefCount) {
  AkPrintDocument *print;
  AkPrintBeamSet  *set;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  set = ak_heap_calloc(heap, print, sizeof(*set));
  if (!set)
    return NULL;

  set->name         = ak_print_strdup(heap, set, name);
  set->identifier   = ak_print_strdup(heap, set, identifier);
  set->refCount     = refCount;
  set->ballRefCount = ballRefCount;

  AK_PRINT_APPEND(print, beamSets, lastBeamSet, set, beamSetCount);
  if (lattice)
    lattice->beamSetCount++;
  print->features |= AK_PRINT_FEATURE_BEAM_LATTICE;

  return set;
}

AK_EXPORT
AkPrintBooleanShape*
ak_printAddBooleanShape(AkDoc                   * __restrict doc,
                        const char              * __restrict path,
                        const char              * __restrict basePath,
                        uint32_t                             objectId,
                        uint32_t                             baseObjectId,
                        AkPrintBooleanOperation              operation,
                        const float             * __restrict matrix,
                        uint32_t                             flags) {
  AkPrintDocument     *print;
  AkPrintBooleanShape *shape;
  AkHeap              *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  shape = ak_heap_calloc(heap, print, sizeof(*shape));
  if (!shape)
    return NULL;

  shape->path         = ak_print_strdup(heap, shape, path);
  shape->basePath     = ak_print_strdup(heap, shape, basePath);
  shape->objectId     = objectId;
  shape->baseObjectId = baseObjectId;
  shape->operation    = operation == AK_PRINT_BOOLEAN_OPERATION_UNKNOWN
                        ? AK_PRINT_BOOLEAN_OPERATION_UNION
                        : operation;
  shape->flags        = flags;
  ak_print_copy_matrix(shape->matrix, matrix);

  AK_PRINT_APPEND(print,
                  booleanShapes,
                  lastBooleanShape,
                  shape,
                  booleanShapeCount);
  print->features |= AK_PRINT_FEATURE_BOOLEAN;

  return shape;
}

AK_EXPORT
AkPrintBooleanOperand*
ak_printAddBooleanOperand(AkDoc                  * __restrict doc,
                          AkPrintBooleanShape    * __restrict shape,
                          const char             * __restrict path,
                          uint32_t                            objectId,
                          const float            * __restrict matrix,
                          uint32_t                            flags) {
  AkPrintDocument       *print;
  AkPrintBooleanOperand *operand;
  AkHeap                *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  operand = ak_heap_calloc(heap, print, sizeof(*operand));
  if (!operand)
    return NULL;

  operand->path     = ak_print_strdup(heap, operand, path);
  operand->objectId = objectId;
  operand->flags    = flags;
  ak_print_copy_matrix(operand->matrix, matrix);

  AK_PRINT_APPEND(print,
                  booleanOperands,
                  lastBooleanOperand,
                  operand,
                  booleanOperandCount);
  if (shape)
    shape->operandCount++;
  print->features |= AK_PRINT_FEATURE_BOOLEAN;

  return operand;
}

AK_EXPORT
AkPrintDisplacement2D*
ak_printAddDisplacement2D(AkDoc      * __restrict doc,
                          const char * __restrict path,
                          uint32_t                id,
                          const char * __restrict imagePath,
                          const char * __restrict channel,
                          const char * __restrict tileStyleU,
                          const char * __restrict tileStyleV,
                          const char * __restrict filter,
                          uint32_t                flags) {
  AkPrintDocument       *print;
  AkPrintDisplacement2D *displacement;
  AkHeap                *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  displacement = ak_heap_calloc(heap, print, sizeof(*displacement));
  if (!displacement)
    return NULL;

  displacement->path       = ak_print_strdup(heap, displacement, path);
  displacement->imagePath  = ak_print_strdup(heap, displacement, imagePath);
  displacement->channel    = ak_print_strdup(heap, displacement, channel);
  displacement->tileStyleU = ak_print_strdup(heap, displacement, tileStyleU);
  displacement->tileStyleV = ak_print_strdup(heap, displacement, tileStyleV);
  displacement->filter     = ak_print_strdup(heap, displacement, filter);
  displacement->id         = id;
  displacement->flags      = flags;

  AK_PRINT_APPEND(print,
                  displacement2Ds,
                  lastDisplacement2D,
                  displacement,
                  displacement2DCount);
  print->features |= AK_PRINT_FEATURE_DISPLACEMENT;

  return displacement;
}

AK_EXPORT
AkPrintNormVectorGroup*
ak_printAddNormVectorGroup(AkDoc      * __restrict doc,
                           const char * __restrict path,
                           uint32_t                id) {
  AkPrintDocument        *print;
  AkPrintNormVectorGroup *group;
  AkHeap                 *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  group = ak_heap_calloc(heap, print, sizeof(*group));
  if (!group)
    return NULL;

  group->path = ak_print_strdup(heap, group, path);
  group->id   = id;

  AK_PRINT_APPEND(print,
                  normVectorGroups,
                  lastNormVectorGroup,
                  group,
                  normVectorGroupCount);
  print->features |= AK_PRINT_FEATURE_DISPLACEMENT;

  return group;
}

AK_EXPORT
AkPrintNormVector*
ak_printAddNormVector(AkDoc                 * __restrict doc,
                      AkPrintNormVectorGroup * __restrict group,
                      float                              x,
                      float                              y,
                      float                              z) {
  AkPrintDocument *print;
  AkPrintNormVector *vector;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  vector = ak_heap_calloc(heap, print, sizeof(*vector));
  if (!vector)
    return NULL;

  vector->x = x;
  vector->y = y;
  vector->z = z;

  AK_PRINT_APPEND(print, normVectors, lastNormVector, vector, normVectorCount);
  if (group)
    group->vectorCount++;
  print->features |= AK_PRINT_FEATURE_DISPLACEMENT;

  return vector;
}

AK_EXPORT
AkPrintDisp2DGroup*
ak_printAddDisp2DGroup(AkDoc      * __restrict doc,
                       const char * __restrict path,
                       uint32_t                id,
                       uint32_t                displacementId,
                       uint32_t                normVectorGroupId,
                       float                   height,
                       float                   offset,
                       uint32_t                flags) {
  AkPrintDocument    *print;
  AkPrintDisp2DGroup *group;
  AkHeap             *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  group = ak_heap_calloc(heap, print, sizeof(*group));
  if (!group)
    return NULL;

  group->path              = ak_print_strdup(heap, group, path);
  group->id                = id;
  group->displacementId    = displacementId;
  group->normVectorGroupId = normVectorGroupId;
  group->height            = height;
  group->offset            = offset;
  group->flags             = flags;

  AK_PRINT_APPEND(print, disp2DGroups, lastDisp2DGroup, group, disp2DGroupCount);
  print->features |= AK_PRINT_FEATURE_DISPLACEMENT;

  return group;
}

AK_EXPORT
AkPrintDisp2DCoord*
ak_printAddDisp2DCoord(AkDoc             * __restrict doc,
                       AkPrintDisp2DGroup * __restrict group,
                       float                         u,
                       float                         v,
                       uint32_t                      normVectorIndex,
                       float                         factor,
                       uint32_t                      flags) {
  AkPrintDocument    *print;
  AkPrintDisp2DCoord *coord;
  AkHeap             *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  coord = ak_heap_calloc(heap, print, sizeof(*coord));
  if (!coord)
    return NULL;

  coord->u               = u;
  coord->v               = v;
  coord->factor          = factor;
  coord->normVectorIndex = normVectorIndex;
  coord->flags           = flags;

  AK_PRINT_APPEND(print, disp2DCoords, lastDisp2DCoord, coord, disp2DCoordCount);
  if (group)
    group->coordCount++;
  print->features |= AK_PRINT_FEATURE_DISPLACEMENT;

  return coord;
}

AK_EXPORT
AkPrintDisplacementMesh*
ak_printAddDisplacementMesh(AkDoc      * __restrict doc,
                            const char * __restrict path,
                            uint32_t                objectId,
                            uint32_t                defaultGroupId,
                            uint32_t                flags) {
  AkPrintDocument         *print;
  AkPrintDisplacementMesh *mesh;
  AkHeap                  *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  mesh = ak_heap_calloc(heap, print, sizeof(*mesh));
  if (!mesh)
    return NULL;

  mesh->path           = ak_print_strdup(heap, mesh, path);
  mesh->objectId       = objectId;
  mesh->defaultGroupId = defaultGroupId;
  mesh->flags          = flags;

  AK_PRINT_APPEND(print,
                  displacementMeshes,
                  lastDisplacementMesh,
                  mesh,
                  displacementMeshCount);
  print->features |= AK_PRINT_FEATURE_DISPLACEMENT;

  return mesh;
}

AK_EXPORT
AkPrintDisplacementTriangle*
ak_printAddDisplacementTriangle(AkDoc                    * __restrict doc,
                                AkPrintDisplacementMesh  * __restrict mesh,
                                uint32_t                               groupId,
                                uint32_t                               d1,
                                uint32_t                               d2,
                                uint32_t                               d3,
                                uint32_t                               flags) {
  AkPrintDocument             *print;
  AkPrintDisplacementTriangle *triangle;
  AkHeap                      *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  triangle = ak_heap_calloc(heap, print, sizeof(*triangle));
  if (!triangle)
    return NULL;

  triangle->groupId = groupId;
  triangle->d1      = d1;
  triangle->d2      = d2;
  triangle->d3      = d3;
  triangle->flags   = flags;

  AK_PRINT_APPEND(print,
                  displacementTriangles,
                  lastDisplacementTriangle,
                  triangle,
                  displacementTriangleCount);
  if (mesh)
    mesh->triangleCount++;
  print->features |= AK_PRINT_FEATURE_DISPLACEMENT;

  return triangle;
}

AK_EXPORT
AkPrintImage3D*
ak_printAddImage3D(AkDoc      * __restrict doc,
                   const char * __restrict path,
                   uint32_t                id,
                   const char * __restrict name,
                   uint32_t                rowCount,
                   uint32_t                columnCount,
                   uint32_t                sheetCount) {
  AkPrintDocument *print;
  AkPrintImage3D  *image;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  image = ak_heap_calloc(heap, print, sizeof(*image));
  if (!image)
    return NULL;

  image->path        = ak_print_strdup(heap, image, path);
  image->name        = ak_print_strdup(heap, image, name);
  image->id          = id;
  image->rowCount    = rowCount;
  image->columnCount = columnCount;
  image->sheetCount  = sheetCount;

  AK_PRINT_APPEND(print, image3Ds, lastImage3D, image, image3DCount);
  print->features |= AK_PRINT_FEATURE_VOLUMETRIC;

  return image;
}

AK_EXPORT
AkPrintImageSheet*
ak_printAddImageSheet(AkDoc          * __restrict doc,
                      AkPrintImage3D * __restrict image,
                      const char     * __restrict path) {
  AkPrintDocument  *print;
  AkPrintImageSheet *sheet;
  AkHeap           *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  sheet = ak_heap_calloc(heap, print, sizeof(*sheet));
  if (!sheet)
    return NULL;

  sheet->path = ak_print_strdup(heap, sheet, path);

  AK_PRINT_APPEND(print, imageSheets, lastImageSheet, sheet, imageSheetCount);
  if (image)
    image->imageSheetCount++;
  print->features |= AK_PRINT_FEATURE_VOLUMETRIC;

  return sheet;
}

AK_EXPORT
AkPrintFunctionFromImage3D*
ak_printAddFunctionFromImage3D(AkDoc      * __restrict doc,
                               const char * __restrict path,
                               uint32_t                id,
                               const char * __restrict displayName,
                               uint32_t                image3DId,
                               float                   valueOffset,
                               float                   valueScale,
                               const char * __restrict filter,
                               const char * __restrict tileStyleU,
                               const char * __restrict tileStyleV,
                               const char * __restrict tileStyleW,
                               uint32_t                flags) {
  AkPrintDocument            *print;
  AkPrintFunctionFromImage3D *function;
  AkHeap                     *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  function = ak_heap_calloc(heap, print, sizeof(*function));
  if (!function)
    return NULL;

  function->path        = ak_print_strdup(heap, function, path);
  function->displayName = ak_print_strdup(heap, function, displayName);
  function->filter      = ak_print_strdup(heap, function, filter);
  function->tileStyleU  = ak_print_strdup(heap, function, tileStyleU);
  function->tileStyleV  = ak_print_strdup(heap, function, tileStyleV);
  function->tileStyleW  = ak_print_strdup(heap, function, tileStyleW);
  function->valueOffset = valueOffset;
  function->valueScale  = valueScale;
  function->id          = id;
  function->image3DId   = image3DId;
  function->flags       = flags;

  AK_PRINT_APPEND(print,
                  functionFromImage3Ds,
                  lastFunctionFromImage3D,
                  function,
                  functionFromImage3DCount);
  print->features |= AK_PRINT_FEATURE_VOLUMETRIC;

  return function;
}

AK_EXPORT
AkPrintImplicitFunction*
ak_printAddImplicitFunction(AkDoc      * __restrict doc,
                            const char * __restrict path,
                            uint32_t                id,
                            const char * __restrict displayName,
                            const char * __restrict xml,
                            uint32_t                flags) {
  AkPrintDocument         *print;
  AkPrintImplicitFunction *function;
  AkHeap                  *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  function = ak_heap_calloc(heap, print, sizeof(*function));
  if (!function)
    return NULL;

  function->path        = ak_print_strdup(heap, function, path);
  function->xml         = ak_print_strdup(heap, function, xml);
  function->displayName = ak_print_strdup(heap, function, displayName);
  function->id          = id;
  function->flags       = flags;

  AK_PRINT_APPEND(print,
                  implicitFunctions,
                  lastImplicitFunction,
                  function,
                  implicitFunctionCount);
  print->features |= AK_PRINT_FEATURE_VOLUMETRIC;

  return function;
}

AK_EXPORT
AkPrintVolumeData*
ak_printAddVolumeData(AkDoc      * __restrict doc,
                      const char * __restrict path,
                      uint32_t                id,
                      uint32_t                baseMaterialId,
                      uint32_t                flags) {
  AkPrintDocument  *print;
  AkPrintVolumeData *volume;
  AkHeap           *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  volume = ak_heap_calloc(heap, print, sizeof(*volume));
  if (!volume)
    return NULL;

  volume->path           = ak_print_strdup(heap, volume, path);
  volume->id             = id;
  volume->baseMaterialId = baseMaterialId;
  volume->flags          = flags;

  AK_PRINT_APPEND(print, volumeData, lastVolumeData, volume, volumeDataCount);
  print->features |= AK_PRINT_FEATURE_VOLUMETRIC;

  return volume;
}

AK_EXPORT
AkPrintVolumetricElement*
ak_printAddVolumetricElement(AkDoc                       * __restrict doc,
                             AkPrintVolumeData           * __restrict volume,
                             AkPrintVolumetricElementType             type,
                             uint32_t                                  functionId,
                             const char                   * __restrict channel,
                             const char                   * __restrict name,
                             const float                  * __restrict matrix,
                             float                                     minFeatureSize,
                             float                                     fallbackValue,
                             uint32_t                                  flags) {
  AkPrintDocument          *print;
  AkPrintVolumetricElement *element;
  AkHeap                   *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  element = ak_heap_calloc(heap, print, sizeof(*element));
  if (!element)
    return NULL;

  element->channel        = ak_print_strdup(heap, element, channel);
  element->name           = ak_print_strdup(heap, element, name);
  element->minFeatureSize = minFeatureSize;
  element->fallbackValue  = fallbackValue;
  element->functionId     = functionId;
  element->type           = type;
  element->flags          = flags;
  ak_print_copy_matrix(element->matrix, matrix);

  AK_PRINT_APPEND(print,
                  volumetricElements,
                  lastVolumetricElement,
                  element,
                  volumetricElementCount);
  if (volume) {
    switch (type) {
      case AK_PRINT_VOLUMETRIC_ELEMENT_MATERIAL_MAPPING:
        volume->materialMappingCount++;
        break;
      case AK_PRINT_VOLUMETRIC_ELEMENT_COLOR:
        volume->colorCount++;
        break;
      case AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY:
        volume->propertyCount++;
        break;
      default:
        break;
    }
  }
  print->features |= AK_PRINT_FEATURE_VOLUMETRIC;

  return element;
}

AK_EXPORT
AkPrintVolumetricMesh*
ak_printAddVolumetricMesh(AkDoc      * __restrict doc,
                          const char * __restrict path,
                          uint32_t                objectId,
                          uint32_t                volumeId,
                          uint32_t                flags) {
  AkPrintDocument       *print;
  AkPrintVolumetricMesh *mesh;
  AkHeap                *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  mesh = ak_heap_calloc(heap, print, sizeof(*mesh));
  if (!mesh)
    return NULL;

  mesh->path     = ak_print_strdup(heap, mesh, path);
  mesh->objectId = objectId;
  mesh->volumeId = volumeId;
  mesh->flags    = flags;

  AK_PRINT_APPEND(print,
                  volumetricMeshes,
                  lastVolumetricMesh,
                  mesh,
                  volumetricMeshCount);
  print->features |= AK_PRINT_FEATURE_VOLUMETRIC;

  return mesh;
}

AK_EXPORT
AkPrintLevelSet*
ak_printAddLevelSet(AkDoc        * __restrict doc,
                    const char   * __restrict path,
                    uint32_t                  objectId,
                    uint32_t                  functionId,
                    const char   * __restrict channel,
                    uint32_t                  meshId,
                    uint32_t                  volumeId,
                    const float  * __restrict matrix,
                    float                     minFeatureSize,
                    float                     fallbackValue,
                    uint32_t                  flags) {
  AkPrintDocument *print;
  AkPrintLevelSet *levelSet;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  levelSet = ak_heap_calloc(heap, print, sizeof(*levelSet));
  if (!levelSet)
    return NULL;

  levelSet->path           = ak_print_strdup(heap, levelSet, path);
  levelSet->channel        = ak_print_strdup(heap, levelSet, channel);
  levelSet->minFeatureSize = minFeatureSize;
  levelSet->fallbackValue  = fallbackValue;
  levelSet->objectId       = objectId;
  levelSet->functionId     = functionId;
  levelSet->meshId         = meshId;
  levelSet->volumeId       = volumeId;
  levelSet->flags          = flags;
  ak_print_copy_matrix(levelSet->matrix, matrix);

  AK_PRINT_APPEND(print, levelSets, lastLevelSet, levelSet, levelSetCount);
  print->features |= AK_PRINT_FEATURE_VOLUMETRIC;

  return levelSet;
}
