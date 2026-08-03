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

static
bool
dae_poly_parse_outer(DAEState     * __restrict dst,
                     AkPolygon    * __restrict poly,
                     xml_t        * __restrict value,
                     FListItem   ** __restrict polyi,
                     uint32_t     * __restrict polygonsCount,
                     size_t       * __restrict indicesCount,
                     AkUInt       * __restrict maxIndex) {
  AkIndexArray *indexArray;
  AkUInt        arrayMax;

  if (!value)
    return false;

  arrayMax = 0;
  if (xml_strtoindex_array_max(dst->heap,
                               poly,
                               value,
                               &indexArray,
                               &arrayMax) != AK_OK)
    return false;

  if (arrayMax > *maxIndex)
    *maxIndex = arrayMax;

  flist_sp_insert(polyi, indexArray);
  (*polygonsCount)++;
  *indicesCount += indexArray->count;
  return true;
}

static
void
dae_poly_append_hole(AkPolygon     * __restrict poly,
                     AkPolygonHole * __restrict hole) {
  AkPolygonHole *last;

  if (!poly->holes) {
    poly->holes = hole;
    return;
  }

  last = poly->holes;
  while (last->next)
    last = last->next;
  last->next = hole;
}

static
void
dae_poly_parse_ph(DAEState     * __restrict dst,
                  AkPolygon    * __restrict poly,
                  xml_t        * __restrict ph,
                  FListItem   ** __restrict polyi,
                  uint32_t     * __restrict polygonsCount,
                  size_t       * __restrict indicesCount,
                  AkUInt       * __restrict maxIndex) {
  AkPolygonHole *holes;
  AkPolygonHole *holesTail;
  xml_t         *ring;
  uint32_t       polygonIndex;
  bool           haveOuter;

  holes        = NULL;
  holesTail    = NULL;
  polygonIndex = *polygonsCount;
  haveOuter    = false;
  ring         = ph->val;

  while (ring) {
    if (!haveOuter && DAE_XML_TAG_EQ8(ring, p) && ring->val) {
      haveOuter = dae_poly_parse_outer(dst,
                                       poly,
                                       ring->val,
                                       polyi,
                                       polygonsCount,
                                       indicesCount,
                                       maxIndex);
    } else if (DAE_XML_TAG_EQ8(ring, h) && ring->val) {
      AkPolygonHole *hole;
      AkUInt          arrayMax;

      hole = ak_heap_calloc(dst->heap, poly, sizeof(*hole));
      if (!hole)
        goto next;

      arrayMax = 0;
      if (xml_strtoui_array_max(dst->heap,
                                hole,
                                ring->val,
                                &hole->indices,
                                &arrayMax) != AK_OK) {
        ak_free(hole);
        goto next;
      }

      hole->polygonIndex = polygonIndex;
      if (arrayMax > *maxIndex)
        *maxIndex = arrayMax;

      if (!holes)
        holes = hole;
      else
        holesTail->next = hole;
      holesTail = hole;
    }

  next:
    ring = ring->next;
  }

  if (!haveOuter) {
    while (holes) {
      AkPolygonHole *next;

      next = holes->next;
      ak_free(holes);
      holes = next;
    }
    return;
  }

  while (holes) {
    AkPolygonHole *next;

    next = holes->next;
    holes->next = NULL;
    dae_poly_append_hole(poly, holes);
    holes = next;
  }
  poly->haveHoles = poly->holes != NULL;
}

AK_HIDE
AkPolygon*
dae_poly(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp,
         AkPolygonMode         mode,
         AkVertices * __restrict fallbackVertices) {
  AkPolygon *poly;
  FListItem *polyi;
  AkHeap    *heap;
  uint32_t   indexoff, polygonsCount, st;
  size_t     indicesCount;
  AkUInt     maxIndex;
  
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

      inp              = dae_input_new(heap, poly);
      inp->semanticRaw = dae_semanticRaw(DAE_XMLA8(xml, semantic),
                                         heap,
                                         inp,
                                         &inp->semantic);

      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkURL *url;

        if (inp->semantic == AK_INPUT_COLOR)
          dst->hasMeshColorInputs = true;

        inp->indexOffset = xmla_u32(DAE_XMLA8(xml, offset), 0);
        inp->set         = xmla_u32(DAE_XMLA4(xml, set),    0);

        url = DAE_URL_FROM(dst, xml, source, memp);
        rb_insert(dst->inputmap, inp, url);

        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          inp->next        = poly->base.input;
          poly->base.input = inp;
          poly->base.inputCount++;

          if (inp->indexOffset > indexoff)
            indexoff = inp->indexOffset;
        } else {
          dae_vertmap_add(dst, inp, &poly->base, fallbackVertices);
          if (inp->indexOffset > indexoff)
            indexoff = inp->indexOffset;

          /* don't store VERTEX because it will be duplicated to all prims */
        }
      }
    } else if (DAE_XML_TAG_EQ8(xml, p) && xml->val) {
      if (mode == AK_POLY_POLYLIST) {
        AkIndexArray *indexArray;
        AkUInt        arrayMax;

        arrayMax = 0;
        if (xml_strtoindex_array_max(heap,
                                     poly,
                                     xml->val,
                                     &indexArray,
                                     &arrayMax) == AK_OK) {
          poly->base.indices = indexArray;
          if (arrayMax > maxIndex)
            maxIndex = arrayMax;
        }
      } else if (mode == AK_POLY_POLYGONS) {
        dae_poly_parse_outer(dst,
                             poly,
                             xml->val,
                             &polyi,
                             &polygonsCount,
                             &indicesCount,
                             &maxIndex);
      }
    } else if (mode == AK_POLY_POLYGONS
               && DAE_XML_TAG_EQ8(xml, ph)
               && xml->val) {
      dae_poly_parse_ph(dst,
                        poly,
                        xml,
                        &polyi,
                        &polygonsCount,
                        &indicesCount,
                        &maxIndex);
    } else if (DAE_XML_TAG_EQ8(xml, vcount) && xml->val) {
      AkUIntArray *intArray;
      if ((xml_strtoui_array(heap, poly, xml->val, &intArray) == AK_OK)) {
        poly->vcount = intArray;
      }
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      poly->base.extra = tree_fromxml(heap, poly, xml);
      if (poly->base.extra)
        ak_extra_set(&poly->base, poly->base.extra);
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
    poly->base.nPolygons = polygonsCount;

    if (poly->holes) {
      AkPolygonHole *hole;

      /*
       * The temporary forward list prepends outer contours, so the packed
       * primitive order is reversed.  Keep hole ownership aligned with that
       * established ordering.
       */
      for (hole = poly->holes; hole; hole = hole->next)
        hole->polygonIndex = polygonsCount - 1u - hole->polygonIndex;
    }

    flist_sp_destroy(&polyi);
  }

finish:
  return poly;
}
