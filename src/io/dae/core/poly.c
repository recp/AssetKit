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
      AkUIntArray *intArray;
      profStep = DAE_PROF_START(dst);
      
      if ((xml_strtoui_array(heap, poly, xml->val, &intArray) == AK_OK)) {
        if (mode == AK_POLY_POLYLIST) {
          poly->base.indices = intArray;
        } else if (mode == AK_POLY_POLYGONS) {
          /* TODO: do this for POLYLIST if vcount not exists */
          flist_sp_insert(&polyi, intArray);
          polygonsCount++;
          indicesCount += intArray->count;
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
    FListItem   *p;
    AkUIntArray *indices, *vcount;
    AkUInt      *pIndices, *pVcount;

    /* alloc indices array */
    indices = ak_heap_alloc(heap,
                            poly,
                            sizeof(*indices) + sizeof(AkUInt) * indicesCount);
    indices->count = indicesCount;

    /* alloc vcount */
    vcount = ak_heap_alloc(heap,
                           poly,
                           sizeof(*vcount) + sizeof(AkUInt) * polygonsCount);
    vcount->count = polygonsCount;

    pIndices = indices->items;
    pVcount  = vcount->items;

    p = polyi;
    while (p) {
      AkUIntArray *intArray;

      intArray = p->data;

      memcpy(pIndices, intArray->items, sizeof(AkUInt) * intArray->count);

      *pVcount++ = (AkUInt)intArray->count / st;
      pIndices  += intArray->count;

      ak_free(intArray);

      p = p->next;
    }

    poly->base.indices = indices;
    poly->vcount       = vcount;

    flist_sp_destroy(&polyi);
  }

  DAE_PROF_ACC(dst, profGeomPolygons, profGeomPolygonsCount, profStart);

  return poly;
}
