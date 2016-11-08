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
ak_dae_polygon(AkHeap * __restrict heap,
               void * __restrict memParent,
               xmlTextReaderPtr reader,
               const char * elm,
               AkPolygonMode mode,
               AkPolygon ** __restrict dest) {
  AkDoc         *doc;
  AkPolygon     *polygon;
  AkInput       *last_input;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  doc     = ak_heap_data(heap);
  polygon = ak_heap_calloc(heap, memParent, sizeof(*polygon), false);
  polygon->mode      = mode;
  polygon->haveHoles = false;
  polygon->base.type = AK_MESH_PRIMITIVE_TYPE_POLYGONS;

  _xml_readAttr(polygon, polygon->base.name, _s_dae_name);
  _xml_readAttr(polygon, polygon->base.material, _s_dae_material);
  /* 
   _xml_readAttrUsingFnWithDef(polygon->count,
                              _s_dae_count,
                              0,
                              strtoul, NULL, 10);
   */

  last_input = NULL;

  do {
    _xml_beginElement(elm);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(heap, polygon, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);

      ak_url_from_attr(reader,
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

      _xml_readAttrUsingFn(input->offset,
                           _s_dae_offset,
                           (AkUInt)strtoul, NULL, 10);

      _xml_readAttrUsingFn(input->set,
                           _s_dae_set,
                           (AkUInt)strtoul, NULL, 10);

      if (last_input)
        last_input->base.next = &input->base;
      else
        polygon->base.input = input;

      last_input = input;

      polygon->base.inputCount++;

      /* attach vertices for convenience */
      if (input->base.semantic == AK_INPUT_SEMANTIC_VERTEX)
        polygon->base.vertices = ak_getObjectByUrl(&input->base.source);
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkUIntArray *intArray;
        AkResult ret;

        ret = ak_strtoui_array(heap, polygon, content, &intArray);
        if (ret == AK_OK)
          polygon->base.indices = intArray;
        
        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_vcount)
               && mode == AK_POLYGON_MODE_POLYLIST) {
      char *content;
      _xml_readMutText(content);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(heap, polygon, content, &intArray);
        if (ret == AK_OK)
          polygon->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_ph)) {
      /* TODO: */
      /*
      AkPolygon      *polygon;
      AkDoubleArrayL *last_array;

      polygon = ak_heap_calloc(heap, polygon, sizeof(*polygon), false);
      polygon->mode = mode;
      polygon->haveHoles = true;

      last_array = NULL;

      do {
        _xml_beginElement(_s_dae_ph);

        if (_xml_eqElm(_s_dae_p)) {
          char *content;

          _xml_readMutText(content);

          if (content) {
            AkUIntArray *intArray;
            AkResult ret;

            ret = ak_strtoui_array(heap, polygon, content, &intArray);
            if (ret == AK_OK)
              polygon->base.indices = intArray;

            xmlFree(content);
          }
        } else if (_xml_eqElm(_s_dae_h)) {
          char *content;

          _xml_readMutText(content);

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
          _xml_skipElement;
        }

        / end element /
        _xml_endElement;
      } while (nodeRet);
      */
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap,
                          polygon,
                          nodePtr,
                          &tree,
                          NULL);
      polygon->base.extra = tree;
      
      _xml_skipElement;
    } else {
      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = polygon;

  return AK_OK;
}
