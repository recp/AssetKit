/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "poly.h"
#include "enum.h"
#include "../../array.h"
#include "../../data.h"

_assetkit_hide
AkPolygon*
dae_poly(DAEState * __restrict dst,
         xml_t    * __restrict xml,
         void     * __restrict memp,
         AkPolygonMode         mode) {
  AkPolygon *poly;
  FListItem *polyi;
  AkHeap    *heap;
  uint32_t   indexoff, polygonsCount, indicesCount, st;
  
  heap = dst->heap;
  poly = ak_heap_calloc(heap, memp, sizeof(*poly));
  
  poly->haveHoles         = false;
  poly->base.type         = AK_PRIMITIVE_POLYGONS;

  poly->base.name         = xmla_strdup_by(xml, heap, _s_dae_name, poly);
  poly->base.bindmaterial = xmla_strdup_by(xml, heap, _s_dae_material, poly);
  poly->base.count        = xmla_u32(xmla(xml, _s_dae_count), 0);

  polyi         = NULL;
  indexoff      = 0;
  polygonsCount = 0;
  indicesCount  = 0;
  
  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;

      inp              = ak_heap_calloc(heap, poly, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);

      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkEnum inputSemantic;

        inputSemantic = dae_semantic(inp->semanticRaw);
        inp->semantic = inputSemantic;

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        inp->semantic = inputSemantic;
        inp->offset   = xmla_u32(xmla(xml, _s_dae_offset), 0);
        inp->set      = xmla_u32(xmla(xml, _s_dae_set),    0);

        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          AkURL *url;

          inp->semantic = dae_semantic(inp->semanticRaw);

          inp->next        = poly->base.input;
          poly->base.input = inp;
          poly->base.inputCount++;

          if (inp->offset > indexoff)
            indexoff = inp->offset;

          url = url_from(xml, _s_dae_source, memp);
          rb_insert(dst->inputmap, inp, url);
        } else {
          /* don't store VERTEX because it will be duplicated to all prims */
          poly->base.reserved1 = inp->offset;
          poly->base.reserved2 = inp->set;
          ak_free(inp);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_p)) {
      AkUIntArray *intArray;
      char        *content;
      
      if ((content = xml->val)) {
        if ((ak_strtoui_array(heap, poly, content, &intArray) == AK_OK)) {
          if (mode == AK_POLY_POLYLIST) {
            poly->base.indices = intArray;
          } else if (mode == AK_POLY_POLYGONS) {
            flist_sp_insert(&polyi, intArray);
            polygonsCount++;
            indicesCount += intArray->count;
          }
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_vcount)) {
      AkUIntArray *intArray;
      char        *content;
      
      if ((content = xml->val)) {
        if ((ak_strtoui_array(heap, poly, content, &intArray) == AK_OK)) {
          poly->vcount = intArray;
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_vcount)) {
      /* TODO: */
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
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

  return poly;
}
