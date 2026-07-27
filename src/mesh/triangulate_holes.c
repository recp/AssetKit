/*
 * Copyright (C) 2026 Recep Aslantas
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

#include "triangulate_holes.h"

#include <float.h>
#include <math.h>
#include <string.h>

#define AK_TESS_EPSILON 1.0e-12

typedef struct AkTessVertex {
  const AkUInt *tuple;
  double        point[3];
  double        x;
  double        y;
  bool          outer;
} AkTessVertex;

static
bool
ak_tess_same_point(const AkTessVertex *a,
                   const AkTessVertex *b) {
  return fabs(a->x - b->x) <= AK_TESS_EPSILON
         && fabs(a->y - b->y) <= AK_TESS_EPSILON;
}

static
double
ak_tess_cross(const AkTessVertex *a,
              const AkTessVertex *b,
              const AkTessVertex *c) {
  return (b->x - a->x) * (c->y - a->y)
         - (b->y - a->y) * (c->x - a->x);
}

static
double
ak_tess_ring_area(const AkTessVertex *ring, size_t count) {
  double area;
  size_t i;

  area = 0.0;
  for (i = 0u; i < count; i++) {
    const AkTessVertex *a;
    const AkTessVertex *b;

    a = &ring[i];
    b = &ring[(i + 1u) % count];
    area += a->x * b->y - b->x * a->y;
  }
  return area * 0.5;
}

static
void
ak_tess_reverse(AkTessVertex *ring, size_t count) {
  size_t i;

  for (i = 0u; i < count / 2u; i++) {
    AkTessVertex tmp;

    tmp = ring[i];
    ring[i] = ring[count - 1u - i];
    ring[count - 1u - i] = tmp;
  }
}

static
bool
ak_tess_position(const AkPolygon *poly,
                 const AkUInt    *tuple,
                 double           out[3]) {
  const AkAccessor *acc;
  const AkBuffer   *buffer;
  const uint8_t    *data;
  size_t            stride;
  size_t            offset;
  AkUInt            index;
  uint32_t          c;

  if (!poly
      || !tuple
      || !poly->base.pos
      || !(acc = poly->base.pos->accessor)
      || !(buffer = acc->buffer)
      || !buffer->data
      || acc->componentCount < 3u
      || poly->base.pos->indexOffset >= poly->base.indexStride)
    return false;

  index = tuple[poly->base.pos->indexOffset];
  if (index >= acc->count)
    return false;

  stride = acc->byteStride ? acc->byteStride : acc->fillByteSize;
  if (stride == 0u)
    return false;

  if ((size_t)index > (SIZE_MAX - acc->byteOffset) / stride)
    return false;
  offset = acc->byteOffset + (size_t)index * stride;

  if (offset > buffer->length
      || acc->fillByteSize > buffer->length - offset)
    return false;
  data = (const uint8_t *)buffer->data + offset;

  if (acc->componentType == AKT_FLOAT) {
    for (c = 0u; c < 3u; c++) {
      float value;

      memcpy(&value, data + (size_t)c * sizeof(value), sizeof(value));
      out[c] = value;
    }
    return true;
  }

  if (acc->componentType == AKT_DOUBLE) {
    for (c = 0u; c < 3u; c++) {
      double value;

      memcpy(&value, data + (size_t)c * sizeof(value), sizeof(value));
      out[c] = value;
    }
    return true;
  }

  return false;
}

static
uint32_t
ak_tess_projection_axis(const AkTessVertex *outer, size_t count) {
  double nx, ny, nz;
  size_t i;

  nx = 0.0;
  ny = 0.0;
  nz = 0.0;
  for (i = 0u; i < count; i++) {
    const double *a;
    const double *b;

    a = outer[i].point;
    b = outer[(i + 1u) % count].point;
    nx += (a[1] - b[1]) * (a[2] + b[2]);
    ny += (a[2] - b[2]) * (a[0] + b[0]);
    nz += (a[0] - b[0]) * (a[1] + b[1]);
  }

  nx = fabs(nx);
  ny = fabs(ny);
  nz = fabs(nz);
  if (nx >= ny && nx >= nz)
    return 0u;
  if (ny >= nz)
    return 1u;
  return 2u;
}

static
void
ak_tess_project(AkTessVertex *ring, size_t count, uint32_t dropAxis) {
  size_t i;

  for (i = 0u; i < count; i++) {
    if (dropAxis == 0u) {
      ring[i].x = ring[i].point[1];
      ring[i].y = ring[i].point[2];
    } else if (dropAxis == 1u) {
      ring[i].x = ring[i].point[0];
      ring[i].y = ring[i].point[2];
    } else {
      ring[i].x = ring[i].point[0];
      ring[i].y = ring[i].point[1];
    }
  }
}

static
double
ak_tess_orient_xy(double ax,
                  double ay,
                  double bx,
                  double by,
                  double cx,
                  double cy) {
  return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

static
bool
ak_tess_on_segment(double ax,
                   double ay,
                   double bx,
                   double by,
                   double px,
                   double py) {
  return px >= fmin(ax, bx) - AK_TESS_EPSILON
         && px <= fmax(ax, bx) + AK_TESS_EPSILON
         && py >= fmin(ay, by) - AK_TESS_EPSILON
         && py <= fmax(ay, by) + AK_TESS_EPSILON;
}

static
bool
ak_tess_segments_intersect(const AkTessVertex *a,
                           const AkTessVertex *b,
                           const AkTessVertex *c,
                           const AkTessVertex *d) {
  double o1, o2, o3, o4;

  o1 = ak_tess_orient_xy(a->x, a->y, b->x, b->y, c->x, c->y);
  o2 = ak_tess_orient_xy(a->x, a->y, b->x, b->y, d->x, d->y);
  o3 = ak_tess_orient_xy(c->x, c->y, d->x, d->y, a->x, a->y);
  o4 = ak_tess_orient_xy(c->x, c->y, d->x, d->y, b->x, b->y);

  if (((o1 > AK_TESS_EPSILON && o2 < -AK_TESS_EPSILON)
       || (o1 < -AK_TESS_EPSILON && o2 > AK_TESS_EPSILON))
      && ((o3 > AK_TESS_EPSILON && o4 < -AK_TESS_EPSILON)
          || (o3 < -AK_TESS_EPSILON && o4 > AK_TESS_EPSILON)))
    return true;

  if (fabs(o1) <= AK_TESS_EPSILON
      && ak_tess_on_segment(a->x, a->y, b->x, b->y, c->x, c->y))
    return true;
  if (fabs(o2) <= AK_TESS_EPSILON
      && ak_tess_on_segment(a->x, a->y, b->x, b->y, d->x, d->y))
    return true;
  if (fabs(o3) <= AK_TESS_EPSILON
      && ak_tess_on_segment(c->x, c->y, d->x, d->y, a->x, a->y))
    return true;
  if (fabs(o4) <= AK_TESS_EPSILON
      && ak_tess_on_segment(c->x, c->y, d->x, d->y, b->x, b->y))
    return true;

  return false;
}

static
bool
ak_tess_point_in_ring(const AkTessVertex *ring,
                      size_t              count,
                      double              x,
                      double              y) {
  bool inside;
  size_t i, j;

  inside = false;
  j = count - 1u;
  for (i = 0u; i < count; j = i++) {
    const AkTessVertex *a;
    const AkTessVertex *b;

    a = &ring[i];
    b = &ring[j];
    if ((a->y > y) != (b->y > y)
        && x < (b->x - a->x) * (y - a->y) / (b->y - a->y) + a->x)
      inside = !inside;
  }
  return inside;
}

static
bool
ak_tess_bridge_visible(const AkTessVertex *holeVertex,
                       const AkTessVertex *outerVertex,
                       const AkTessVertex *contour,
                       size_t              contourCount,
                       const AkTessVertex *hole,
                       size_t              holeCount,
                       const AkTessVertex *outer,
                       size_t              outerCount) {
  AkTessVertex midpoint;
  size_t i;

  if (ak_tess_same_point(holeVertex, outerVertex))
    return false;

  for (i = 0u; i < contourCount; i++) {
    const AkTessVertex *a;
    const AkTessVertex *b;

    a = &contour[i];
    b = &contour[(i + 1u) % contourCount];
    if (ak_tess_same_point(a, outerVertex)
        || ak_tess_same_point(b, outerVertex))
      continue;
    if (ak_tess_segments_intersect(holeVertex, outerVertex, a, b))
      return false;
  }

  for (i = 0u; i < holeCount; i++) {
    const AkTessVertex *a;
    const AkTessVertex *b;

    a = &hole[i];
    b = &hole[(i + 1u) % holeCount];
    if (ak_tess_same_point(a, holeVertex)
        || ak_tess_same_point(b, holeVertex))
      continue;
    if (ak_tess_segments_intersect(holeVertex, outerVertex, a, b))
      return false;
  }

  memset(&midpoint, 0, sizeof(midpoint));
  midpoint.x = (holeVertex->x + outerVertex->x) * 0.5;
  midpoint.y = (holeVertex->y + outerVertex->y) * 0.5;
  return ak_tess_point_in_ring(outer, outerCount, midpoint.x, midpoint.y)
         && !ak_tess_point_in_ring(hole, holeCount, midpoint.x, midpoint.y);
}

static
bool
ak_tess_bridge_hole(const AkTessVertex *outer,
                    size_t              outerCount,
                    AkTessVertex       *contour,
                    size_t             *contourCount,
                    AkTessVertex       *scratch,
                    size_t              capacity,
                    AkTessVertex       *hole,
                    size_t              holeCount) {
  double bestDistance;
  size_t holeIndex;
  size_t outerIndex;
  size_t bestOuter;
  size_t i;
  size_t out;

  holeIndex = 0u;
  for (i = 1u; i < holeCount; i++) {
    if (hole[i].x > hole[holeIndex].x
        || (fabs(hole[i].x - hole[holeIndex].x) <= AK_TESS_EPSILON
            && hole[i].y < hole[holeIndex].y))
      holeIndex = i;
  }

  bestOuter = SIZE_MAX;
  bestDistance = DBL_MAX;
  for (outerIndex = 0u; outerIndex < *contourCount; outerIndex++) {
    double dx, dy, distance;

    if (!contour[outerIndex].outer)
      continue;
    if (!ak_tess_bridge_visible(&hole[holeIndex],
                                &contour[outerIndex],
                                contour,
                                *contourCount,
                                hole,
                                holeCount,
                                outer,
                                outerCount))
      continue;

    dx = contour[outerIndex].x - hole[holeIndex].x;
    dy = contour[outerIndex].y - hole[holeIndex].y;
    distance = dx * dx + dy * dy;
    if (distance < bestDistance) {
      bestDistance = distance;
      bestOuter = outerIndex;
    }
  }

  if (bestOuter == SIZE_MAX
      || *contourCount > capacity - holeCount - 2u)
    return false;

  out = 0u;
  for (i = 0u; i <= bestOuter; i++)
    scratch[out++] = contour[i];

  for (i = 0u; i < holeCount; i++)
    scratch[out++] = hole[(holeIndex + i) % holeCount];

  scratch[out++] = hole[holeIndex];
  scratch[out++] = contour[bestOuter];

  for (i = bestOuter + 1u; i < *contourCount; i++)
    scratch[out++] = contour[i];

  memcpy(contour, scratch, out * sizeof(*contour));
  *contourCount = out;
  return true;
}

static
bool
ak_tess_point_strictly_in_triangle(const AkTessVertex *a,
                                   const AkTessVertex *b,
                                   const AkTessVertex *c,
                                   const AkTessVertex *p) {
  return ak_tess_orient_xy(a->x, a->y, b->x, b->y, p->x, p->y)
           > AK_TESS_EPSILON
         && ak_tess_orient_xy(b->x, b->y, c->x, c->y, p->x, p->y)
           > AK_TESS_EPSILON
         && ak_tess_orient_xy(c->x, c->y, a->x, a->y, p->x, p->y)
           > AK_TESS_EPSILON;
}

static
void
ak_tess_write_tuple(AkIndexArray *indices,
                    size_t       *cursor,
                    const AkUInt *tuple,
                    uint32_t      stride) {
  uint32_t i;

  switch (indices->componentType) {
    case AKT_UBYTE: {
      uint8_t *items;

      items = (uint8_t *)indices->items;
      for (i = 0u; i < stride; i++)
        items[(*cursor)++] = (uint8_t)tuple[i];
      break;
    }
    case AKT_USHORT: {
      uint16_t *items;

      items = (uint16_t *)(void *)indices->items;
      for (i = 0u; i < stride; i++)
        items[(*cursor)++] = (uint16_t)tuple[i];
      break;
    }
    case AKT_UINT: {
      uint32_t *items;

      items = (uint32_t *)(void *)indices->items;
      for (i = 0u; i < stride; i++)
        items[(*cursor)++] = tuple[i];
      break;
    }
    default:
      break;
  }
}

static
bool
ak_tess_ear_clip(const AkTessVertex *contour,
                 size_t              contourCount,
                 size_t             *active,
                 AkIndexArray       *output,
                 size_t             *outputCursor,
                 uint32_t            stride,
                 uint32_t           *triangleCount) {
  size_t activeCount;
  size_t guard;

  if (contourCount < 3u)
    return false;

  for (activeCount = 0u; activeCount < contourCount; activeCount++)
    active[activeCount] = activeCount;

  guard = contourCount * contourCount;
  activeCount = contourCount;
  while (activeCount > 3u && guard-- > 0u) {
    bool clipped;
    size_t i;

    clipped = false;
    for (i = 0u; i < activeCount; i++) {
      size_t ia, ib, ic;
      size_t j;
      bool contains;

      ia = active[(i + activeCount - 1u) % activeCount];
      ib = active[i];
      ic = active[(i + 1u) % activeCount];
      if (ak_tess_cross(&contour[ia], &contour[ib], &contour[ic])
          <= AK_TESS_EPSILON)
        continue;

      contains = false;
      for (j = 0u; j < activeCount; j++) {
        size_t ip;

        ip = active[j];
        if (ip == ia || ip == ib || ip == ic
            || ak_tess_same_point(&contour[ip], &contour[ia])
            || ak_tess_same_point(&contour[ip], &contour[ib])
            || ak_tess_same_point(&contour[ip], &contour[ic]))
          continue;
        if (ak_tess_point_strictly_in_triangle(&contour[ia],
                                               &contour[ib],
                                               &contour[ic],
                                               &contour[ip])) {
          contains = true;
          break;
        }
      }
      if (contains)
        continue;

      ak_tess_write_tuple(output,
                          outputCursor,
                          contour[ia].tuple,
                          stride);
      ak_tess_write_tuple(output,
                          outputCursor,
                          contour[ib].tuple,
                          stride);
      ak_tess_write_tuple(output,
                          outputCursor,
                          contour[ic].tuple,
                          stride);
      (*triangleCount)++;

      memmove(&active[i],
              &active[i + 1u],
              (activeCount - i - 1u) * sizeof(*active));
      activeCount--;
      clipped = true;
      break;
    }

    if (!clipped) {
      bool removed;
      size_t i;

      removed = false;
      for (i = 0u; i < activeCount; i++) {
        size_t ia, ib, ic;

        ia = active[(i + activeCount - 1u) % activeCount];
        ib = active[i];
        ic = active[(i + 1u) % activeCount];
        if (!ak_tess_same_point(&contour[ia], &contour[ib])
            && !ak_tess_same_point(&contour[ib], &contour[ic])
            && fabs(ak_tess_cross(&contour[ia],
                                  &contour[ib],
                                  &contour[ic])) > AK_TESS_EPSILON)
          continue;

        memmove(&active[i],
                &active[i + 1u],
                (activeCount - i - 1u) * sizeof(*active));
        activeCount--;
        removed = true;
        break;
      }
      if (!removed)
        return false;
    }
  }

  if (activeCount != 3u
      || ak_tess_cross(&contour[active[0]],
                       &contour[active[1]],
                       &contour[active[2]]) <= AK_TESS_EPSILON)
    return false;

  ak_tess_write_tuple(output,
                      outputCursor,
                      contour[active[0]].tuple,
                      stride);
  ak_tess_write_tuple(output,
                      outputCursor,
                      contour[active[1]].tuple,
                      stride);
  ak_tess_write_tuple(output,
                      outputCursor,
                      contour[active[2]].tuple,
                      stride);
  (*triangleCount)++;
  return true;
}

static
void
ak_tess_free_holes(AkPolygon *poly) {
  AkPolygonHole *hole;

  hole = poly->holes;
  while (hole) {
    AkPolygonHole *next;

    next = hole->next;
    ak_free(hole);
    hole = next;
  }
  poly->holes = NULL;
  poly->haveHoles = false;
}

AK_HIDE
uint32_t
ak_meshTriangulatePolyHoles(AkPolygon * __restrict poly) {
  AkIndexArray  *source;
  AkIndexArray  *output;
  AkPolygonHole *hole;
  AkHeap        *heap;
  size_t         sourceCursor;
  size_t         outputCursor;
  size_t         maxContourCount;
  size_t         totalTriangles;
  uint32_t       polygonIndex;
  uint32_t       triangleCount;
  uint32_t       stride;

  if (!poly
      || !poly->haveHoles
      || !poly->holes
      || !poly->vcount
      || !(source = ak_meshPrimitiveMaterializeIndices(&poly->base))
      || !poly->base.pos
      || !(heap = ak_heap_getheap(poly)))
    return 0u;

  stride = poly->base.indexStride;
  if (stride == 0u)
    return 0u;

  maxContourCount = 0u;
  totalTriangles = 0u;
  for (polygonIndex = 0u;
       polygonIndex < poly->vcount->count;
       polygonIndex++) {
    size_t contourCount;
    size_t outerCount;

    outerCount = poly->vcount->items[polygonIndex];
    if (outerCount < 3u)
      return 0u;

    contourCount = outerCount;
    for (hole = poly->holes; hole; hole = hole->next) {
      size_t holeCount;

      if (hole->polygonIndex != polygonIndex)
        continue;
      if (!hole->indices
          || hole->indices->count % stride != 0u
          || (holeCount = hole->indices->count / stride) < 3u)
        return 0u;
      if (contourCount > SIZE_MAX - holeCount - 2u)
        return 0u;
      contourCount += holeCount + 2u;
    }

    if (contourCount < 3u || totalTriangles > SIZE_MAX - (contourCount - 2u))
      return 0u;
    totalTriangles += contourCount - 2u;
    if (contourCount > maxContourCount)
      maxContourCount = contourCount;
  }

  if (totalTriangles == 0u
      || totalTriangles > SIZE_MAX / (3u * stride))
    return 0u;

  output = ak_indexArrayAlloc(heap,
                              poly,
                              totalTriangles * 3u * stride,
                              source->componentType);
  if (!output)
    return 0u;
  output->max = source->max;

  sourceCursor = 0u;
  outputCursor = 0u;
  triangleCount = 0u;
  for (polygonIndex = 0u;
       polygonIndex < poly->vcount->count;
       polygonIndex++) {
    AkTessVertex *outer;
    AkTessVertex *contour;
    AkTessVertex *scratch;
    AkTessVertex *holeRing;
    AkUInt       *outerTuples;
    size_t       *active;
    size_t        contourCount;
    size_t        outerCount;
    size_t        maxHoleCount;
    uint32_t      dropAxis;
    size_t        i, j;

    outerCount = poly->vcount->items[polygonIndex];
    maxHoleCount = 0u;
    for (hole = poly->holes; hole; hole = hole->next) {
      size_t holeCount;

      if (hole->polygonIndex != polygonIndex)
        continue;
      holeCount = hole->indices->count / stride;
      if (holeCount > maxHoleCount)
        maxHoleCount = holeCount;
    }

    outer = ak_heap_alloc(heap, poly, outerCount * sizeof(*outer));
    contour = ak_heap_alloc(heap, poly, maxContourCount * sizeof(*contour));
    scratch = ak_heap_alloc(heap, poly, maxContourCount * sizeof(*scratch));
    holeRing = ak_heap_alloc(heap,
                             poly,
                             (maxHoleCount ? maxHoleCount : 1u)
                               * sizeof(*holeRing));
    outerTuples = ak_heap_alloc(heap,
                                poly,
                                outerCount * stride * sizeof(*outerTuples));
    active = ak_heap_alloc(heap, poly, maxContourCount * sizeof(*active));
    if (!outer || !contour || !scratch || !holeRing || !outerTuples || !active)
      goto fail_polygon;

    memset(outer, 0, outerCount * sizeof(*outer));
    for (i = 0u; i < outerCount; i++) {
      AkUInt *tuple;

      tuple = &outerTuples[i * stride];
      for (j = 0u; j < stride; j++)
        tuple[j] = ak_indexArrayGet(source, sourceCursor + i * stride + j);
      outer[i].tuple = tuple;
      outer[i].outer = true;
      if (!ak_tess_position(poly, tuple, outer[i].point))
        goto fail_polygon;
    }
    sourceCursor += outerCount * stride;

    dropAxis = ak_tess_projection_axis(outer, outerCount);
    ak_tess_project(outer, outerCount, dropAxis);
    if (fabs(ak_tess_ring_area(outer, outerCount)) <= AK_TESS_EPSILON)
      goto fail_polygon;
    if (ak_tess_ring_area(outer, outerCount) < 0.0)
      ak_tess_reverse(outer, outerCount);

    memcpy(contour, outer, outerCount * sizeof(*contour));
    contourCount = outerCount;

    for (hole = poly->holes; hole; hole = hole->next) {
      size_t holeCount;

      if (hole->polygonIndex != polygonIndex)
        continue;

      holeCount = hole->indices->count / stride;
      memset(holeRing, 0, holeCount * sizeof(*holeRing));
      for (i = 0u; i < holeCount; i++) {
        const AkUInt *tuple;

        tuple = &hole->indices->items[i * stride];
        holeRing[i].tuple = tuple;
        if (!ak_tess_position(poly, tuple, holeRing[i].point))
          goto fail_polygon;
      }
      ak_tess_project(holeRing, holeCount, dropAxis);
      if (fabs(ak_tess_ring_area(holeRing, holeCount)) <= AK_TESS_EPSILON)
        goto fail_polygon;
      if (ak_tess_ring_area(holeRing, holeCount) > 0.0)
        ak_tess_reverse(holeRing, holeCount);

      if (!ak_tess_bridge_hole(outer,
                               outerCount,
                               contour,
                               &contourCount,
                               scratch,
                               maxContourCount,
                               holeRing,
                               holeCount))
        goto fail_polygon;
    }

    if (!ak_tess_ear_clip(contour,
                          contourCount,
                          active,
                          output,
                          &outputCursor,
                          stride,
                          &triangleCount))
      goto fail_polygon;

    ak_free(active);
    ak_free(outerTuples);
    ak_free(holeRing);
    ak_free(scratch);
    ak_free(contour);
    ak_free(outer);
    continue;

  fail_polygon:
    if (active) ak_free(active);
    if (outerTuples) ak_free(outerTuples);
    if (holeRing) ak_free(holeRing);
    if (scratch) ak_free(scratch);
    if (contour) ak_free(contour);
    if (outer) ak_free(outer);
    ak_free(output);
    return 0u;
  }

  if (triangleCount != totalTriangles
      || outputCursor != totalTriangles * 3u * stride) {
    ak_free(output);
    return 0u;
  }

  ak_free(poly->base.indices);
  poly->base.indices = output;
  poly->base.indexAccessor = NULL;
  ak_tess_free_holes(poly);
  poly->base.nPolygons = triangleCount;
  poly->base.type = AK_PRIMITIVE_TRIANGLES;
  ((AkTriangles *)poly)->mode = AK_TRIANGLES;

  ak_free(poly->vcount);
  poly->vcount = NULL;
  return triangleCount;
}
