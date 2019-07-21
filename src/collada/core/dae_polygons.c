/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_polygons.h"
#include "dae_enums.h"
#include "../../array.h"
#include "../../data.h"

AkResult _assetkit_hide
dae_polygon(AkXmlState * __restrict xst,
            void       * __restrict memParent,
            const char             *elm,
            AkPolygonMode           mode,
            AkPolygon ** __restrict dest) {
  AkPolygon     *polygon;
  AkInput       *last_input;
  FListItem     *polygons;
  AkXmlElmState  xest;
  uint32_t       indexoff, polygonsCount, indicesCount, st;

  polygon = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*polygon));

  polygon->haveHoles     = false;
  polygon->base.type     = AK_MESH_PRIMITIVE_TYPE_POLYGONS;

  polygon->base.name     = ak_xml_attr(xst, polygon, _s_dae_name);
  polygon->base.material = ak_xml_attr(xst, polygon, _s_dae_material);
  polygon->base.count    = ak_xml_attrui(xst, _s_dae_count);

  /*
   _xml_readAttrUsingFnWithDef(polygon->count,
                              _s_dae_count,
                              0,
                              strtoul, NULL, 10);
   */

  last_input    = NULL;
  polygons      = NULL;
  indexoff      = 0;
  polygonsCount = 0;
  indicesCount  = 0;

  ak_xest_init(xest, elm)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(xst->heap, polygon, sizeof(*input));
      input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      if (!input->semanticRaw)
        ak_free(input);
      else {
        AkEnum inputSemantic;

        inputSemantic = dae_enumInputSemantic(input->semanticRaw);
        input->offset = ak_xml_attrui(xst, _s_dae_offset);
        input->set    = ak_xml_attrui(xst, _s_dae_set);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->semantic = inputSemantic;

        if ((uint32_t)input->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          if (last_input)
            last_input->next = input;
          else
            polygon->base.input = input;

          last_input = input;

          polygon->base.inputCount++;

          if (input->offset > indexoff)
            indexoff = input->offset;

          ak_xml_attr_url(xst, _s_dae_source, input, &input->source);
        } else {
          /* don't store VERTEX because it will be duplicated to all prims */
          polygon->base.reserved1 = input->offset;
          polygon->base.reserved2 = input->set;
          ak_free(input);
        }
      }
    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *intArray;
        AkResult ret;

        if (mode == AK_POLYGON_MODE_POLYLIST) {
          ret = ak_strtoui_array(xst->heap, polygon, content, &intArray);
          if (ret == AK_OK)
            polygon->base.indices = intArray;
        } else if (mode == AK_POLYGON_MODE_POLYGONS) {
          ret = ak_strtoui_array(xst->heap, polygon, content, &intArray);
          if (ret == AK_OK) {
            flist_sp_insert(&polygons, intArray);
            polygonsCount++;
            indicesCount += intArray->count;
          }
        }

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_vcount)
               && mode == AK_POLYGON_MODE_POLYLIST) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *uintArray;
        AkResult     ret;

        ret = ak_strtoui_array(xst->heap, polygon, content, &uintArray);
        if (ret == AK_OK)
          polygon->vcount = uintArray;

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_ph)) {
      /* TODO: */
      /*
      AkPolygon      *polygon;
      AkDoubleArrayL *last_array;

      polygon = ak_heap_calloc(heap, polygon, sizeof(*polygon), false);
      polygon->mode = mode;
      polygon->haveHoles = true;

      last_array = NULL;

      do {
        if (ak_xml_beginelm(xst, (_s_dae_ph);

        if (ak_xml_eqelm(xst, _s_dae_p)) {
          char *content;

          content = ak_xml_rawval(xst);

          if (content) {
            AkUIntArray *intArray;
            AkResult ret;

            ret = ak_strtoui_array(heap, polygon, content, &intArray);
            if (ret == AK_OK)
              polygon->base.indices = intArray;

            xmlFree(content);
          }
        } else if (ak_xml_eqelm(xst, _s_dae_h)) {
          char *content;

          content = ak_xml_rawval(xst);

          if (content) {
            AkDoubleArrayL *doubleArray;
            AkResult ret;

            ret = ak_strtod_arrayL(heap, polygon, content, &doubleArray);
            if (ret == AK_OK) {
              if (last_array)
                last_array->next = doubleArray;
              else
                polygon->holes = doubleArray;

              last_array = doubleArray;
            }

            xmlFree(content);
          }
        } else {
          ak_xml_skipelm(xst);
        }

        / end element /
        ak_xml_endelm(xst);
      } while (xst->nodeRet);
      */
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, polygon, &polygon->base.extra);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  polygon->base.indexStride = st = indexoff + 1;

  if (mode == AK_POLYGON_MODE_POLYGONS) {
    FListItem   *p;
    AkUIntArray *indices, *vcount;
    AkUInt      *pIndices, *pVcount;

    /* alloc indices array */
    indices = ak_heap_alloc(xst->heap,
                            polygon,
                            sizeof(*indices) + sizeof(AkUInt) * indicesCount);
    indices->count = indicesCount;

    /* alloc vcount */
    vcount = ak_heap_alloc(xst->heap,
                           polygon,
                           sizeof(*vcount) + sizeof(AkUInt) * polygonsCount);
    vcount->count = polygonsCount;

    pIndices = indices->items;
    pVcount  = vcount->items;

    p = polygons;
    while (p) {
      AkUIntArray *intArray;

      intArray = p->data;

      memcpy(pIndices, intArray->items, sizeof(AkUInt) * intArray->count);

      *pVcount++ = (AkUInt)intArray->count / st;
      pIndices  += intArray->count;

      ak_free(intArray);

      p = p->next;
    }

    polygon->base.indices = indices;
    polygon->vcount       = vcount;

    flist_sp_destroy(&polygons);
  }

  *dest = polygon;

  return AK_OK;
}
