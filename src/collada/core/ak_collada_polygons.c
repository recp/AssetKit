/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_polygons.h"
#include "ak_collada_enums.h"
#include "../../ak_array.h"

AkResult _assetkit_hide
ak_dae_polygon(AkXmlState * __restrict xst,
               void * __restrict memParent,
               const char * elm,
               AkPolygonMode mode,
               AkPolygon ** __restrict dest) {
  AkPolygon    *polygon;
  AkInput      *last_input;
  AkXmlElmState xest;
  uint32_t      indexoff;

  polygon = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*polygon));

  polygon->mode      = mode;
  polygon->haveHoles = false;
  polygon->base.type = AK_MESH_PRIMITIVE_TYPE_POLYGONS;

  polygon->base.name     = ak_xml_attr(xst, polygon, _s_dae_name);
  polygon->base.material = ak_xml_attr(xst, polygon, _s_dae_material);
  polygon->base.count    = ak_xml_attrui64(xst, _s_dae_count);

  /*
   _xml_readAttrUsingFnWithDef(polygon->count,
                              _s_dae_count,
                              0,
                              strtoul, NULL, 10);
   */

  last_input = NULL;
  indexoff   = 0;

  ak_xest_init(xest, elm)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(xst->heap, polygon, sizeof(*input));
      input->base.semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

      ak_xml_attr_url(xst,
                      _s_dae_source,
                      input,
                      &input->base.source);

      if (!input->base.semanticRaw || !input->base.source.url)
        ak_free(input);
      else {
        AkEnum inputSemantic;
        inputSemantic = ak_dae_enumInputSemantic(input->base.semanticRaw);

        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;

        input->base.semantic = inputSemantic;
      }

      input->offset = ak_xml_attrui(xst, _s_dae_offset);
      input->set    = ak_xml_attrui(xst, _s_dae_set);

      if (last_input)
        last_input->base.next = &input->base;
      else
        polygon->base.input = input;

      last_input = input;

      polygon->base.inputCount++;

      /* attach vertices for convenience */
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX)
        polygon->base.vertices = ak_getObjectByUrl(&input->base.source);

      if (input->offset > indexoff)
        indexoff = input->offset;
    } else if (ak_xml_eqelm(xst, _s_dae_p)) {
      char *content;

      content = ak_xml_rawval(xst);

      if (content) {
        AkUIntArray *intArray;
        AkResult ret;

        ret = ak_strtoui_array(xst->heap, polygon, content, &intArray);
        if (ret == AK_OK)
          polygon->base.indices = intArray;

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_vcount)
               && mode == AK_POLYGON_MODE_POLYLIST) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(xst->heap, polygon, content, &intArray);
        if (ret == AK_OK)
          polygon->vcount = intArray;

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
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          polygon,
                          nodePtr,
                          &tree,
                          NULL);
      polygon->base.extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  polygon->base.indexStride = indexoff + 1;

  *dest = polygon;

  return AK_OK;
}
