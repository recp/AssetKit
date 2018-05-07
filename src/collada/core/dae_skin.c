/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_skin.h"
#include "../../array.h"
#include "dae_source.h"
#include "dae_enums.h"

AkResult _assetkit_hide
ak_dae_skin(AkXmlState * __restrict xst,
            void * __restrict memParent,
            bool asObject,
            AkSkin ** __restrict dest) {
  AkObject     *obj;
  AkSkin       *skin;
  AkSource     *last_source;
  void         *memPtr;
  AkXmlElmState xest;

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

  ak_xest_init(xest, _s_dae_skin)

  do {
    if (ak_xml_begin(&xest))
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
      AkInput      *last_input;
      AkXmlElmState xest2;

      joints = ak_heap_calloc(xst->heap, memPtr, sizeof(*joints));

      last_input = NULL;

      ak_xest_init(xest2, _s_dae_joints)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInput *input;

          input = ak_heap_calloc(xst->heap, joints, sizeof(*input));
          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

          if (!input->semanticRaw)
            ak_free(input);
          else {
            AkEnum inputSemantic;
            inputSemantic = ak_dae_enumInputSemantic(input->semanticRaw);

            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_SEMANTIC_OTHER;

            input->semantic = inputSemantic;

            ak_xml_attr_url(xst, _s_dae_source, input, &input->source);

            if (last_input)
              last_input->next = input;
            else
              joints->input = input;

            last_input = input;
          }
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
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      skin->joints = joints;
    } else if (ak_xml_eqelm(xst, _s_dae_vertex_weights)) {
      AkVertexWeights *vertexWeights;
      AkInput         *last_input;
      AkXmlElmState    xest2;

      vertexWeights = ak_heap_calloc(xst->heap,
                                     memPtr,
                                     sizeof(*vertexWeights));

      last_input = NULL;

      ak_xest_init(xest2, _s_dae_vertex_weights)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInput *input;
          input = ak_heap_calloc(xst->heap, vertexWeights, sizeof(*input));
          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

          if (!input->semanticRaw || !input->source.url)
            ak_free(input);
          else {
            AkEnum inputSemantic;
            inputSemantic = ak_dae_enumInputSemantic(input->semanticRaw);

            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_SEMANTIC_OTHER;

            input->semantic = inputSemantic;
            input->offset   = ak_xml_attrui(xst, _s_dae_offset);
            input->set      = ak_xml_attrui(xst, _s_dae_set);

            ak_xml_attr_url(xst, _s_dae_source, input, &input->source);

            if (last_input)
              last_input->next = input;
            else
              vertexWeights->input = input;

            last_input = input;
          }
        } else if (ak_xml_eqelm(xst, _s_dae_vcount)) {
          char *content;
          content = ak_xml_rawval(xst);

          if (content) {
            AkUIntArray *intArray;
            AkResult     ret;

            ret = ak_strtoui_array(xst->heap,
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
        if (ak_xml_end(&xest2))
          break;
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
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = skin;

  return AK_OK;
}
