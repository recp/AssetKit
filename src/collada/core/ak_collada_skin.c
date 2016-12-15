/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_skin.h"
#include "../../ak_array.h"
#include "ak_collada_source.h"
#include "ak_collada_enums.h"

AkResult _assetkit_hide
ak_dae_skin(AkXmlState * __restrict xst,
            void * __restrict memParent,
            bool asObject,
            AkSkin ** __restrict dest) {
  AkObject *obj;
  AkSkin   *skin;
  AkSource *last_source;
  void     *memPtr;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*skin),
                      0,
                      true);

    skin = ak_objGet(obj);

    memPtr = obj;
  } else {
    skin = ak_heap_calloc(xst->heap, memParent, sizeof(*skin));
    memPtr = skin;
  }

  skin->baseMesh = ak_xml_attr(xst, memPtr, _s_dae_source);

  last_source = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_skin))
      break;

    if (ak_xml_eqelm(xst, _s_dae_bind_shape_matrix)) {
      char *content;
      content = ak_xml_rawval(xst);

      if (content) {
        AkDoubleArray *doubleArray;
        AkResult ret;

        ret = ak_strtod_array(xst->heap, memPtr, content, &doubleArray);
        if (ret == AK_OK)
          skin->bindShapeMatrix = doubleArray;

        xmlFree(content);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = ak_dae_source(xst, memPtr, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          skin->source = source;

        last_source = source;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_joints)) {
      AkJoints     *joints;
      AkInputBasic *last_input;

      joints = ak_heap_calloc(xst->heap, memPtr, sizeof(*joints));

      last_input = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_joints))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInputBasic *input;

          input = ak_heap_calloc(xst->heap,
                                 joints,
                                 sizeof(*input));

          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

          ak_xml_attr_url(xst,
                          _s_dae_source,
                          input,
                          &input->source);

          if (!input->semanticRaw || !input->source.url)
            ak_free(input);
          else {
            AkEnum inputSemantic;
            inputSemantic = ak_dae_enumInputSemantic(input->semanticRaw);

            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_SEMANTIC_OTHER;

            input->semantic = inputSemantic;
          }

          if (last_input)
            last_input->next = input;
          else
            joints->input = input;

          last_input = input;
        } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(xst->reader);
          tree = NULL;

          ak_tree_fromXmlNode(xst->heap,
                              joints,
                              nodePtr,
                              &tree,
                              NULL);
          joints->extra = tree;

          ak_xml_skipelm(xst);

        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      skin->joints = joints;
    } else if (ak_xml_eqelm(xst, _s_dae_vertex_weights)) {
      AkVertexWeights *vertexWeights;
      AkInput         *last_input;

      vertexWeights = ak_heap_calloc(xst->heap,
                                     memPtr,
                                     sizeof(*vertexWeights));

      last_input = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_vertex_weights))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInput *input;
          input = ak_heap_calloc(xst->heap,
                                 vertexWeights,
                                 sizeof(*input));

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
            vertexWeights->input = input;

          last_input = input;
        } else if (ak_xml_eqelm(xst, _s_dae_vcount)) {
          char *content;
          content = ak_xml_rawval(xst);

          if (content) {
            AkIntArray *intArray;
            AkResult    ret;

            ret = ak_strtoi_array(xst->heap,
                                  vertexWeights,
                                  content,
                                  &intArray);
            if (ret == AK_OK)
              vertexWeights->vcount = intArray;

            xmlFree(content);
          }
        } else if (ak_xml_eqelm(xst, _s_dae_v)) {
          char *content;
          content = ak_xml_rawval(xst);

          if (content) {
            AkDoubleArray *doubleArray;
            AkResult       ret;

            ret = ak_strtod_array(xst->heap,
                                  vertexWeights,
                                  content,
                                  &doubleArray);
            if (ret == AK_OK)
              vertexWeights->v = doubleArray;

            xmlFree(content);
          }
        } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(xst->reader);
          tree = NULL;

          ak_tree_fromXmlNode(xst->heap,
                              vertexWeights,
                              nodePtr,
                              &tree,
                              NULL);
          vertexWeights->extra = tree;

          ak_xml_skipelm(xst);

        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      skin->vertexWeights = vertexWeights;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          memPtr,
                          nodePtr,
                          &tree,
                          NULL);
      skin->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  *dest = skin;

  return AK_OK;
}
