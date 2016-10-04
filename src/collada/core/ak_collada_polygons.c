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
                bool asObject,
                AkPolygon ** __restrict dest) {
  AkObject       *obj;
  AkPolygon      *polygon;
  AkInput        *last_input;
  void           *memPtr;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  if (asObject) {
    obj = ak_objAlloc(heap,
                      memParent,
                      sizeof(*polygon),
                      0,
                      true,
                      false);

    polygon = ak_objGet(obj);
    memPtr = obj;
  } else {
    polygon = ak_heap_calloc(heap, memParent, sizeof(*polygon), false);
    memPtr  = polygon;
  }

  polygon->mode      = mode;
  polygon->haveHoles = false;

  _xml_readAttr(memPtr, polygon->name, _s_dae_name);
  _xml_readAttr(memPtr, polygon->material, _s_dae_material);
  /* 
   _xml_readAttrUsingFnWithDef(polygon->count,
                              _s_dae_count,
                              0,
                              strtoul, NULL, 10);
   */

  last_input   = NULL;

  do {
    _xml_beginElement(elm);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;

      input = ak_heap_calloc(heap, memPtr, sizeof(*input), false);

      _xml_readAttr(input, input->base.semanticRaw, _s_dae_semantic);
      _xml_readAttr(input, input->base.source, _s_dae_source);

      if (!input->base.semanticRaw || !input->base.source)
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
        polygon->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkUIntArray *intArray;
        AkResult ret;

        ret = ak_strtoui_array(heap, memPtr, content, &intArray);
        if (ret == AK_OK)
          polygon->indices = intArray;
        
        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_vcount)
               && mode == AK_POLYGON_MODE_POLYLIST) {
      char *content;
      _xml_readMutText(content);

      if (content) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(heap, memPtr, content, &intArray);
        if (ret == AK_OK)
          polygon->vcount = intArray;

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_ph)) {
      AkPolygon      *polygon;
      AkDoubleArrayL *last_array;

      polygon = ak_heap_calloc(heap, memPtr, sizeof(*polygon), false);
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

            ret = ak_strtoui_array(heap, memPtr, content, &intArray);
            if (ret == AK_OK)
              polygon->indices = intArray;

            xmlFree(content);
          }
        } else if (_xml_eqElm(_s_dae_h)) {
          char *content;

          _xml_readMutText(content);

          if (content) {
            AkDoubleArrayL *doubleArray;
            AkResult ret;

            ret = ak_strtod_arrayL(heap, memPtr, content, &doubleArray);
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

        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap,
                          memPtr,
                          nodePtr,
                          &tree,
                          NULL);
      polygon->extra = tree;
      
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
