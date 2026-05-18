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

#include "poly.h"
#include "enum.h"
#include "../../../array.h"
#include "../../../data.h"

AK_HIDE
AkPolygon*
dae_poly(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp,
         AkPolygonMode         mode) {
  AkPolygon *poly;
  FListItem *polyi;
  AkHeap    *heap;
  uint32_t   indexoff, polygonsCount, st;
  size_t     indicesCount;
  AkUInt     maxIndex;
  double     profStart, profStep;
  
  profStart = DAE_PROF_START(dst);
  heap = dst->heap;
  poly = ak_heap_calloc(heap, memp, sizeof(*poly));
  
  poly->haveHoles         = false;
  poly->base.type         = AK_PRIMITIVE_POLYGONS;

  poly->base.name         = DAE_XMLA_STRDUP8(xml, heap, name, poly);
  poly->base.bindmaterial = DAE_XMLA_STRDUP8(xml, heap, material, poly);
  poly->base.nPolygons    = xmla_u32(DAE_XMLA8(xml, count), 0);

  polyi         = NULL;
  indexoff      = 0;
  polygonsCount = 0;
  indicesCount  = 0;
  maxIndex      = 0;
  
  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, input)) {
      AkInput *inp;
      profStep = DAE_PROF_START(dst);

      inp              = dae_input_new(heap, poly);
      inp->semanticRaw = dae_semanticRaw(DAE_XMLA8(xml, semantic),
                                         heap,
                                         inp,
                                         &inp->semantic);

      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkURL *url;

        inp->offset   = xmla_u32(DAE_XMLA8(xml, offset), 0);
        inp->set      = xmla_u32(DAE_XMLA4(xml, set),    0);

        url = DAE_URL_FROM(xml, source, memp);
        rb_insert(dst->inputmap, inp, url);

        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          inp->next        = poly->base.input;
          poly->base.input = inp;
          poly->base.inputCount++;

          if (inp->offset > indexoff)
            indexoff = inp->offset;
        } else {
          dae_vertmap_add(dst, inp, &poly->base);
          /* don't store VERTEX because it will be duplicated to all prims */
          // poly->base.reserved1 = inp->offset;
          // poly->base.reserved2 = inp->set;
          // ak_free(inp);
        }
      }
      DAE_PROF_ACC(dst, profGeomInput, profGeomInputCount, profStep);
    } else if (DAE_XML_TAG_EQ8(xml, p) && xml->val) {
      AkIndexArray *indexArray;
      AkUInt        arrayMax;
      profStep = DAE_PROF_START(dst);
      
      arrayMax = 0;
      if ((xml_strtoindex_array_max(heap,
                                     poly,
                                     xml->val,
                                     &indexArray,
                                     &arrayMax) == AK_OK)) {
        if (arrayMax > maxIndex)
          maxIndex = arrayMax;
        if (mode == AK_POLY_POLYLIST) {
          poly->base.indices = indexArray;
        } else if (mode == AK_POLY_POLYGONS) {
          /* TODO: do this for POLYLIST if vcount not exists */
          flist_sp_insert(&polyi, indexArray);
          polygonsCount++;
          indicesCount += indexArray->count;
        }
      }
      DAE_PROF_ACC(dst, profGeomIndexArray, profGeomIndexArrayCount, profStep);
    } else if (DAE_XML_TAG_EQ8(xml, vcount) && xml->val) {
      AkUIntArray *intArray;
      profStep = DAE_PROF_START(dst);
      if ((xml_strtoui_array(heap, poly, xml->val, &intArray) == AK_OK)) {
        poly->vcount = intArray;
      }
      DAE_PROF_ACC(dst, profGeomIndexArray, profGeomIndexArrayCount, profStep);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      poly->base.extra = tree_fromxml(heap, poly, xml);
    }
    xml = xml->next;
  }

  poly->base.indexStride = st = indexoff + 1;
  
  if (mode == AK_POLY_POLYGONS) {
    FListItem    *p;
    AkIndexArray *indices;
    AkUIntArray  *vcount;
    AkUInt       *pVcount;
    AkTypeId      componentType;
    uint8_t      *pIndices;
    size_t        itemSize;

    /* alloc indices array */
    componentType = ak_indexComponentTypeForMax(maxIndex);
    indices       = ak_indexArrayAlloc(heap, poly, indicesCount, componentType);
    if (!indices)
      goto finish;
    indices->max = maxIndex;
    itemSize     = ak_indexComponentSize(componentType);

    /* alloc vcount */
    vcount = ak_heap_alloc(heap,
                           poly,
                           sizeof(*vcount) + sizeof(AkUInt) * polygonsCount);
    vcount->count = polygonsCount;

    pIndices = (uint8_t *)indices->items;
    pVcount  = vcount->items;

    p = polyi;
    while (p) {
      AkIndexArray *indexArray;

      indexArray = p->data;

      if (indexArray->componentType == componentType) {
        memcpy(pIndices, indexArray->items, itemSize * indexArray->count);
      } else {
        size_t i;

        switch (componentType) {
          case AKT_UBYTE: {
            uint8_t *dst;

            dst = (uint8_t *)pIndices;
            for (i = 0; i < indexArray->count; i++)
              dst[i] = (uint8_t)ak_indexArrayGet(indexArray, i);
            break;
          }
          case AKT_USHORT: {
            uint16_t *dst;

            dst = (uint16_t *)(void *)pIndices;
            for (i = 0; i < indexArray->count; i++)
              dst[i] = (uint16_t)ak_indexArrayGet(indexArray, i);
            break;
          }
          case AKT_UINT: {
            uint32_t *dst;

            dst = (uint32_t *)(void *)pIndices;
            for (i = 0; i < indexArray->count; i++)
              dst[i] = ak_indexArrayGet(indexArray, i);
            break;
          }
          default:
            break;
        }
      }

      *pVcount++ = (AkUInt)indexArray->count / st;
      pIndices  += itemSize * indexArray->count;

      ak_free(indexArray);

      p = p->next;
    }

    poly->base.indices = indices;
    poly->vcount         = vcount;

    flist_sp_destroy(&polyi);
  }

finish:
  DAE_PROF_ACC(dst, profGeomPolygons, profGeomPolygonsCount, profStart);

  return poly;
}
