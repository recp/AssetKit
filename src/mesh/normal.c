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
#include "../id.h"
#include "../data.h"
#include "../strpool.h"

#include "index.h"

#include <cglm/cglm.h>

AK_HIDE
void
ak_meshPrimGenNormals(AkMeshPrimitive    * __restrict prim);

AK_EXPORT
bool
ak_meshPrimNeedsNormals(AkMeshPrimitive * __restrict prim) {
  AkAccessor   *acc;
  AkInput      *input;
  bool          ret;

  if (!prim
      || (prim->type != AK_PRIMITIVE_TRIANGLES
          && prim->type != AK_PRIMITIVE_POLYGONS))
    return false;

  ret   = true;
  input = prim->input;
  while (input) {
    if (input->semantic == AK_INPUT_NORMAL) {
      if (!(acc = input->accessor) || !acc->buffer)
        return ret;
      ret = false;
      break;
    }

    input = input->next;
  }

  return ret;
}

AK_EXPORT
bool
ak_meshNeedsNormals(AkMesh * __restrict mesh) {
  AkMeshPrimitive *prim;
  bool ret;

  ret  = false;
  prim = mesh->primitive;
  while (prim) {
    ret |= ak_meshPrimNeedsNormals(prim);
    if (ret)
      break;
    prim = prim->next;
  }

  return ret;
}

AK_INLINE
AkUInt
ak_normalIndexGet(const AkIndexArray * __restrict indices,
                  size_t                          index) {
  switch (indices->componentType) {
    case AKT_UBYTE:
      return ((const uint8_t *)(const void *)indices->items)[index];
    case AKT_USHORT:
      return ((const uint16_t *)(const void *)indices->items)[index];
    case AKT_UINT:
      return ((const uint32_t *)(const void *)indices->items)[index];
    default:
      return 0;
  }
}

AK_INLINE
void
ak_normalIndexSet(AkIndexArray * __restrict indices,
                  size_t                    index,
                  AkUInt                    value) {
  switch (indices->componentType) {
    case AKT_UBYTE:
      ((uint8_t *)(void *)indices->items)[index] = (uint8_t)value;
      break;
    case AKT_USHORT:
      ((uint16_t *)(void *)indices->items)[index] = (uint16_t)value;
      break;
    case AKT_UINT:
      ((uint32_t *)(void *)indices->items)[index] = value;
      break;
    default:
      break;
  }
}

AK_INLINE
void
ak_normalCopyTuple(AkIndexArray       * __restrict dst,
                   const AkIndexArray * __restrict src,
                   size_t                          dstOff,
                   size_t                          srcOff,
                   AkUInt                          st) {
  AkUInt i;

  for (i = 0; i < st; i++)
    ak_normalIndexSet(dst, dstOff + i, ak_normalIndexGet(src, srcOff + i));
}

static
AkUInt
ak_normalGeneratedCount(AkMeshPrimitive * __restrict prim,
                        uint32_t                     count) {
  if (prim->type == AK_PRIMITIVE_POLYGONS) {
    AkPolygon *poly;
    AkUInt    *vc_it;
    AkUInt     normalCount;
    size_t     i;

    poly = (AkPolygon *)prim;
    if (!poly->vcount)
      return count / 3;

    vc_it       = poly->vcount->items;
    normalCount = 0;
    for (i = 0; i < poly->vcount->count; i++) {
      if (vc_it[i] >= 3)
        normalCount++;
    }

    return normalCount;
  }

  if (prim->type == AK_PRIMITIVE_TRIANGLES) {
    AkTriangles *tri;

    tri = (AkTriangles *)prim;
    switch (tri->mode) {
      case AK_TRIANGLES:
        return count / 3;
      case AK_TRIANGLE_FAN:
      case AK_TRIANGLE_STRIP:
        return count >= 3 ? count - 2 : 0;
      default:
        return 0;
    }
  }

  return 0;
}

AK_HIDE
void
ak_meshPrimGenNormals(AkMeshPrimitive * __restrict prim) {
  AkDataContext *dctx;
  AkDoc         *doc;
  AkIndexArray  *srcIndices;
  AkIndexArray  *inpIndices;
  AkFloat       *pos;
  AkHeap        *heap;
  AkInput       *input, *nextInput;
  AkBuffer      *posBuff, *buff;
  AkAccessor    *posAcc, *acc;
  AkTypeId       componentType;
  AkUInt         st, newst, generatedNormals, indexMax;
  AkInt          vo, pos_st;
  uint32_t       count, tuple;

  if ((prim->type    != AK_PRIMITIVE_TRIANGLES
       && prim->type != AK_PRIMITIVE_POLYGONS)
      || !prim->pos
      || !(posAcc     = prim->pos->accessor)
      || !(posBuff    = posAcc->buffer)
      || (vo          = prim->pos->offset) == -1)
    return;

  heap   = ak_heap_getheap(prim);
  doc    = ak_heap_data(heap);
  pos    = posBuff->data;
  pos_st = posAcc->componentCount;
  st     = prim->indexStride;

  if (st == 0
      || !(srcIndices = ak_meshPrimitiveMaterializeIndices(prim))
      || srcIndices->count == 0)
    return;

  count            = (uint32_t)srcIndices->count / st;
  generatedNormals = ak_normalGeneratedCount(prim, count);
  if (generatedNormals == 0)
    return;

  newst = st + 1;
  indexMax = ak_indicesMax(srcIndices);
  if (generatedNormals - 1 > indexMax)
    indexMax = generatedNormals - 1;
  componentType = ak_indexComponentTypeForMax(indexMax);

  /* TODO: for now join this into existing indices,
           but in the future use separate to fix indices  */
  inpIndices = ak_indexArrayAlloc(heap, prim, count * newst, componentType);
  if (!inpIndices)
    return;

  inpIndices->max = indexMax;

  for (tuple = 0; tuple < count; tuple++)
    ak_normalCopyTuple(inpIndices,
                       srcIndices,
                       (size_t)tuple * newst,
                       (size_t)tuple * st,
                       st);

  dctx = ak_data_new(prim, 64, sizeof(vec3), ak_cmp_vec3);
  if (!dctx) {
    ak_free(inpIndices);
    return;
  }

  switch (prim->type) {
    case AK_PRIMITIVE_POLYGONS: {
      AkPolygon *poly;
      AkUInt    *vc_it;
      float     *a, *b, *c;
      vec3       v1, v2, n;
      size_t     i, j, k, vc, ist;
      AkUInt     idx;

      poly = (AkPolygon *)prim;

      if (!poly->vcount) {
        for (i = 0; i < count; i += 3) {
          ist = i * st + vo;

          a = pos + ak_normalIndexGet(srcIndices, ist)           * pos_st;
          b = pos + ak_normalIndexGet(srcIndices, ist + st)      * pos_st;
          c = pos + ak_normalIndexGet(srcIndices, ist + st + st) * pos_st;

          glm_vec3_sub(a, b, v1);
          glm_vec3_sub(b, c, v2);

          glm_vec3_cross(v1, v2, n);
          glm_vec3_normalize(n);

          idx = ak_data_append(dctx, n);

          for (j = i; j < i + 3; j++)
            ak_normalIndexSet(inpIndices, j * newst + st, idx);
        }
        break;
      }

      vc_it = poly->vcount->items;

      for (i = k = 0; k < poly->vcount->count; k++) {
        vc = vc_it[k];

        /* TODO: normals for lines or points ? */
        if (vc < 3) {
          i += vc;
          continue;
        }

        ist = i * st + vo;

        a = pos + ak_normalIndexGet(srcIndices, ist)           * pos_st;
        b = pos + ak_normalIndexGet(srcIndices, ist + st)      * pos_st;
        c = pos + ak_normalIndexGet(srcIndices, ist + st + st) * pos_st;

        glm_vec3_sub(a, b, v1);
        glm_vec3_sub(b, c, v2);

        glm_vec3_cross(v1, v2, n);
        glm_vec3_normalize(n);

        idx = ak_data_append(dctx, n);

        for (j = i; j < i + vc; j++) {
          ak_normalIndexSet(inpIndices, j * newst + st, idx);
        }

        i += vc;
      }
      break;
    }
    case AK_PRIMITIVE_TRIANGLES: {
      AkTriangles *tri;
      float *a, *b, *c;
      vec3   v1, v2, n;
      AkUInt i, j, idx, ist;

      tri = (AkTriangles *)prim;
      switch (tri->mode) {
        case AK_TRIANGLES:
          for (i = 0; i < count; i += 3 /* 3: triangle */) {
            ist = i * st + vo;

            a = pos + ak_normalIndexGet(srcIndices, ist)           * pos_st;
            b = pos + ak_normalIndexGet(srcIndices, ist + st)      * pos_st;
            c = pos + ak_normalIndexGet(srcIndices, ist + st + st) * pos_st;

            glm_vec3_sub(a, b, v1);
            glm_vec3_sub(b, c, v2);

            glm_vec3_cross(v1, v2, n);
            glm_vec3_normalize(n);

            idx = ak_data_append(dctx, n);

            for (j = i; j < i + 3; j++) {
              ak_normalIndexSet(inpIndices, j * newst + st, idx);
            }
          }
          break;
        case AK_TRIANGLE_FAN: {
          float *central = pos + ak_normalIndexGet(srcIndices, vo) * pos_st;
          for (i = 1; i < count - 1; i++) {
            a = central;
            b = pos + ak_normalIndexGet(srcIndices, vo + i * st) * pos_st;
            c = pos + ak_normalIndexGet(srcIndices, vo + (i + 1) * st) * pos_st;

            // Calculate normal
            glm_vec3_sub(b, a, v1);
            glm_vec3_sub(c, a, v2);
            glm_vec3_cross(v1, v2, n);
            glm_vec3_normalize(n);

            idx = ak_data_append(dctx, n);

            // Assign normals to central, current, and next vertex
            ak_normalIndexSet(inpIndices, st, idx);
            ak_normalIndexSet(inpIndices, i * newst + st, idx);
            ak_normalIndexSet(inpIndices, (i + 1) * newst + st, idx);
          }
          break;
        }
        case AK_TRIANGLE_STRIP: {
          for (i = 0; i < count - 2; i++) {
            a = pos + ak_normalIndexGet(srcIndices, vo + i * st) * pos_st;
            b = pos + ak_normalIndexGet(srcIndices, vo + (i + 1) * st) * pos_st;
            c = pos + ak_normalIndexGet(srcIndices, vo + (i + 2) * st) * pos_st;

            // Calculate normal
            glm_vec3_sub(b, a, v1);
            glm_vec3_sub(c, b, v2);
            glm_vec3_cross(v1, v2, n);
            glm_vec3_normalize(n);

            idx = ak_data_append(dctx, n);

            // Assign normals to the three vertices of the triangle
            ak_normalIndexSet(inpIndices, i * newst + st, idx);
            ak_normalIndexSet(inpIndices, (i + 1) * newst + st, idx);
            ak_normalIndexSet(inpIndices, (i + 2) * newst + st, idx);
          }
          break;
        }

        default:
          break;
      }
      break;
    }
    default:
      ak_free(inpIndices);
      return;
  }

  acc = ak_heap_calloc(heap, doc, sizeof(*acc));
  ak_setypeid(acc, AKT_ACCESSOR);

  acc->componentCount    = 3;
  acc->count             = (uint32_t)dctx->itemcount;
  acc->componentType     = AKT_FLOAT;
  acc->componentSize     = AK_COMPONENT_SIZE_VEC3;
  acc->bytesPerComponent = ak_typeDesc(acc->componentType)->size;
  acc->byteStride        = acc->componentCount * acc->bytesPerComponent;
  acc->fillByteSize      = acc->byteStride;
  acc->byteLength        = acc->count * acc->byteStride;

  buff                   = ak_heap_calloc(heap, doc, sizeof(*buff));
  buff->data             = ak_heap_alloc(heap, buff, acc->byteLength);
  buff->length           = acc->byteLength;

  acc->buffer            = buff;

  flist_sp_insert(&doc->lib.accessors, acc);
  flist_sp_insert(&doc->lib.buffers, buff);
  
  /* add input */
  input              = ak_heap_calloc(heap, prim, sizeof(*input));
  input->offset      = st;
  input->semantic    = AK_INPUT_NORMAL;
  input->semanticRaw = _s_NORMAL;

  nextInput = prim->input;
  if (nextInput) {
    while (nextInput->next)
      nextInput = nextInput->next;
    nextInput->next = input;
  } else {
    prim->input = input;
  }

  input->accessor = acc;

  prim->inputCount++;
  prim->indexStride++;

  ak_free(prim->indices);
  prim->indices = inpIndices;
  prim->indexAccessor = NULL;

  (void)ak_data_join(dctx, buff->data, 0, 0);
  ak_free(dctx);
}

AK_EXPORT
void
ak_meshGenNormals(AkMesh * __restrict mesh) {
  AkMeshEditHelper *edith;
  AkMeshPrimitive  *prim;

  ak_meshBeginEdit(mesh);

  prim  = mesh->primitive;
  edith = mesh->edith;

  while (prim) {
    ak_meshPrimGenNormals(prim);

    if (!edith->skipFixIndices)
      ak_primFixIndices(mesh, prim);

    prim = prim->next;
  }

  ak_meshEndEdit(mesh);
}
