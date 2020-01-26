/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "skin.h"
#include "../../array.h"
#include "source.h"
#include "enum.h"

AkResult _assetkit_hide
dae_skin(AkXmlState * __restrict xst,
         void       * __restrict memParent,
         bool                    asObject,
         AkSkin    ** __restrict dest) {
  AkObject     *obj;
  AkSkin       *skin;
  AkSource     *last_source;
  void         *memPtr;
  AkXmlElmState xest;

  if (asObject) {
    obj = ak_objAlloc(xst->heap,
                      memParent,
                      sizeof(*skin),
                      AK_CONTROLLER_SKIN,
                      true);

    skin = ak_objGet(obj);

    memPtr = obj;
  } else {
    skin = ak_heap_calloc(xst->heap, memParent, sizeof(*skin));
    memPtr = skin;
  }

  ak_xml_attr_url(xst, _s_dae_source, memPtr, &skin->baseGeom);

  last_source = NULL;

  ak_xest_init(xest, _s_dae_skin)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_bind_shape_matrix)) {
      char *content;
      content = ak_xml_rawval(xst);
      if (content) {
        ak_strtof(&content, skin->bindShapeMatrix[0], 16);
        glm_mat4_transpose(skin->bindShapeMatrix);
        xmlFree(content);
      } else {
        glm_mat4_identity(skin->bindShapeMatrix);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_source)) {
      AkSource *source;
      AkResult ret;

      ret = dae_source(xst, memPtr, NULL, 0, &source);
      if (ret == AK_OK) {
        if (last_source)
          last_source->next = source;
        else
          skin->source = source;

        last_source = source;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_joints)) {
      AkXmlElmState xest2;

      ak_xest_init(xest2, _s_dae_joints)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInput *input;

          input = ak_heap_calloc(xst->heap, memPtr, sizeof(*input));
          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

          if (!input->semanticRaw)
            ak_free(input);
          else {
            AkURL *url;
            AkEnum inputSemantic;

            inputSemantic = dae_enumInputSemantic(input->semanticRaw);

            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_SEMANTIC_OTHER;

            input->semantic = inputSemantic;

            url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
            rb_insert(xst->inputmap, input, url);

            if (inputSemantic == AK_INPUT_SEMANTIC_JOINT) {
              skin->reserved[0] = input;
            } else if (inputSemantic == AK_INPUT_SEMANTIC_INV_BIND_MATRIX) {
              skin->reserved[1] = input;
            } else {
              /* do not support other inputs until needed,
                 probably will not. */
              ak_free(input);
            }
          }
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

    } else if (ak_xml_eqelm(xst, _s_dae_vertex_weights)) {
      AkBoneWeights *weights;
      AkXmlElmState  xest2;

      weights = ak_heap_calloc(xst->heap, memPtr, sizeof(*weights));

      ak_xest_init(xest2, _s_dae_vertex_weights)

      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_input)) {
          AkInput *input;
          input = ak_heap_calloc(xst->heap, memPtr, sizeof(*input));
          input->semanticRaw = ak_xml_attr(xst, input, _s_dae_semantic);

          if (!input->semanticRaw)
            ak_free(input);
          else {
            AkURL *url;
            AkEnum inputSemantic;

            inputSemantic = dae_enumInputSemantic(input->semanticRaw);

            if (inputSemantic < 0)
              inputSemantic = AK_INPUT_SEMANTIC_OTHER;

            input->semantic = inputSemantic;
            input->offset   = ak_xml_attrui(xst, _s_dae_offset);

            url = ak_xmlAttrGetURL(xst, _s_dae_source, input);
            rb_insert(xst->inputmap, input, url);

            if (inputSemantic == AK_INPUT_SEMANTIC_JOINT) {
              skin->reserved[2] = input;
            } else if (inputSemantic == AK_INPUT_SEMANTIC_WEIGHT) {
              skin->reserved[3] = input;
            } else {
              /* do not support other inputs until needed,
                 probably will not. */
              ak_free(input);
            }

            skin->reserved2++;
          }
        } else if (ak_xml_eqelm(xst, _s_dae_vcount)) {
          char *content;
          content = ak_xml_rawval(xst);

          if (content) {
            size_t    count, sz, i;
            uint32_t *pSum, *pCount, next;

            if ((count = ak_strtok_count_fast(content, NULL)) > 0) {
              sz = count * sizeof(uint32_t);

              /*

               we use same temp array to store sum of counts to avoid to
               create new pointer. Array layout:

               | pCount | pCountSum |

               */

              weights->counts = pCount = ak_heap_alloc(xst->heap,
                                                       weights,
                                                       2 * sz);

              /* must equal to position count, we may fix this in postscript */
              weights->nVertex = count;

              ak_strtoui_fast(content, pCount, count);

              /* calculate sum */
              pSum = weights->counts + count;
              for (next = i = 0; i < count; i++) {
                pSum[i] = next;
                next    = pCount[i] + next;
              }
            }

            xmlFree(content);
          }
        } else if (ak_xml_eqelm(xst, _s_dae_v)) {
          char *content;
          content = ak_xml_rawval(xst);

          if (content) {
            AkUIntArray *intArray;
            AkResult     ret;

            ret = ak_strtoui_array(xst->heap, weights, content, &intArray);
            if (ret == AK_OK)
              skin->reserved[4] = intArray;

            xmlFree(content);
          }
        } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
          dae_extra(xst, weights, &weights->extra);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      skin->weights = (void *)weights;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      dae_extra(xst, memPtr, &skin->extra);
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
