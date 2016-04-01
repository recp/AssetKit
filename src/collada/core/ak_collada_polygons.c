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
ak_dae_polygons(void * __restrict memParent,
                xmlTextReaderPtr reader,
                const char * elm,
                AkPolygonMode mode,
                bool asObject,
                AkPolygons ** __restrict dest) {
  AkObject       *obj;
  AkPolygons     *polygons;
  AkPolygon      *last_polygon;
  AkInput        *last_input;
  void           *memPtr;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  if (asObject) {
    obj = ak_objAlloc(memParent,
                      sizeof(*polygons),
                      0,
                      true);

    polygons = ak_objGet(obj);
    memPtr = obj;
  } else {
    polygons = ak_calloc(memParent, sizeof(*polygons), 1);
    memPtr = polygons;
  }

  _xml_readAttr(memPtr, polygons->name, _s_dae_name);
  _xml_readAttr(memPtr, polygons->material, _s_dae_material);
  _xml_readAttrUsingFnWithDef(polygons->count,
                              _s_dae_count,
                              0,
                              strtoul, NULL, 10);

  last_polygon = NULL;
  last_input   = NULL;

  do {
    _xml_beginElement(elm);

    if (_xml_eqElm(_s_dae_input)) {
      AkInput *input;

      input = ak_calloc(memPtr, sizeof(*input), 1);

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

      if (last_input)
        last_input->base.next = &input->base;
      else
        polygons->input = input;

      last_input = input;
    } else if (_xml_eqElm(_s_dae_p)) {
      char *content;

      _xml_readMutText(content);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;
        ret = ak_strtod_array(memPtr, content, &doubleArray);
        if (ret == AK_OK) {
          if (mode == AK_POLYGON_MODE_POLYLIST) {
            if (!last_polygon) {
              AkPolygon * polygon;

              polygon = ak_calloc(memParent, sizeof(*polygon), 1);
              polygon->mode = mode;
              polygon->haveHoles = false;

              polygons->polygon = polygon;
              last_polygon = polygon;
            }
          } else {
            AkPolygon *polygon;
            polygon = ak_calloc(memPtr, sizeof(*polygon), 1);
            polygon->mode = mode;
            polygon->haveHoles = false;

            if (last_polygon)
              last_polygon->next = polygon;
            else
              polygons->polygon = polygon;

            last_polygon = polygon;
          }
          
          last_polygon->primitives = doubleArray;
        }
        
        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_vcount)
               && mode == AK_POLYGON_MODE_POLYLIST) {
      char *content;
      _xml_readMutText(content);

      if (content && last_polygon) {
        AkIntArray *intArray;
        AkResult    ret;

        ret = ak_strtoi_array(memPtr, content, &intArray);
        if (ret == AK_OK) {
          if (!last_polygon) {
            AkPolygon * polygon;

            polygon = ak_calloc(memParent, sizeof(*polygon), 1);
            polygon->mode = mode;

            polygons->polygon = polygon;
            last_polygon = polygon;
          }

          last_polygon->vcount = intArray;
        }

        xmlFree(content);
      }
    } else if (_xml_eqElm(_s_dae_ph)) {
      AkPolygon      *polygon;
      AkDoubleArrayL *last_array;

      polygon = ak_calloc(memPtr, sizeof(*polygon), 1);
      polygon->mode = mode;
      polygon->haveHoles = true;

      last_array = NULL;

      do {
        _xml_beginElement(_s_dae_ph);

        if (_xml_eqElm(_s_dae_p)) {
          char *content;

          _xml_readMutText(content);

          if (content) {
            AkDoubleArray *doubleArray;
            AkResult ret;

            ret = ak_strtod_array(memPtr, content, &doubleArray);
            if (ret == AK_OK)
              polygon->primitives = doubleArray;

            xmlFree(content);
          }
        } else if (_xml_eqElm(_s_dae_h)) {
          char *content;

          _xml_readMutText(content);

          if (content) {
            AkDoubleArrayL *doubleArray;
            AkResult ret;

            ret = ak_strtod_arrayL(memPtr, content, &doubleArray);
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

      if (last_polygon)
        last_polygon->next = polygon;
      else
        polygons->polygon = polygon;

      last_polygon = polygon;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(memPtr, nodePtr, &tree, NULL);
      polygons->extra = tree;
      
      _xml_skipElement;
    } else {
      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = polygons;

  return AK_OK;
}
