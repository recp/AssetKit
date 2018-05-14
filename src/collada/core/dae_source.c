/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_source.h"
#include "../core/dae_asset.h"
#include "../core/dae_technique.h"
#include "../core/dae_accessor.h"

#define k_s_dae_asset      1000
#define k_s_dae_techniquec 1001
#define k_s_dae_technique  1002

static ak_enumpair sourceMap[] = {
  {_s_dae_bool_array,   AKT_BOOL},
  {_s_dae_float_array,  AKT_FLOAT},
  {_s_dae_IDREF_array,  AKT_IDREF},
  {_s_dae_int_array,    AKT_INT},
  {_s_dae_Name_array,   AKT_NAME},
  {_s_dae_SIDREF_array, AKT_SIDREF},
  {_s_dae_token_array,  AKT_TOKEN},
  {_s_dae_asset,        k_s_dae_asset},
  {_s_dae_techniquec,   k_s_dae_techniquec},
  {_s_dae_technique,    k_s_dae_technique}
};

static size_t sourceMapLen = 0;

AkResult _assetkit_hide
ak_dae_source(AkXmlState * __restrict xst,
              void       * __restrict memParent,
              AkSource  ** __restrict dest) {
  AkSource     *source;
  AkBuffer     *buffer;
  AkTechnique  *last_tq;
  AkXmlElmState xest;

  buffer = NULL;
  source = ak_heap_calloc(xst->heap, memParent, sizeof(*source));
  ak_setypeid(source, AKT_SOURCE);

  ak_xml_readid(xst, source);
  source->name = ak_xml_attr(xst, source, _s_dae_name);

  if (xmlTextReaderIsEmptyElement(xst->reader))
    goto done;

  if (sourceMapLen == 0) {
    sourceMapLen = AK_ARRAY_LEN(sourceMap);
    qsort(sourceMap,
          sourceMapLen,
          sizeof(sourceMap[0]),
          ak_enumpair_cmp);
  }

  last_tq = NULL;

  ak_xest_init(xest, _s_dae_source)

  do {
    const ak_enumpair *found;
    char              *content;
    size_t             arraySize;
    AkUInt             arrayCount;

    if (ak_xml_begin(&xest))
      break;

    found = bsearch(xst->nodeName,
                    sourceMap,
                    sourceMapLen,
                    sizeof(sourceMap[0]),
                    ak_enumpair_cmp2);
    if (!found) {
      ak_xml_skipelm(xst);
      goto skip;
    }

    arraySize = arrayCount = 0;
    switch (found->val) {
      case AKT_FLOAT:
      case AKT_INT:
      case AKT_BOOL:
      case AKT_SIDREF:
      case AKT_IDREF:
      case AKT_NAME:
      case AKT_TOKEN: {
        buffer = ak_heap_alloc(xst->heap, source, sizeof(*buffer));
        ak_xml_readid(xst, buffer);

        arrayCount = ak_xml_attrui(xst, _s_dae_count);
        arraySize  = sizeof(AkFloat) * arrayCount;

        buffer->name     = ak_xml_attr(xst, buffer, _s_dae_name);
        buffer->length   = arraySize;
        buffer->reserved = found->val;

        source->buffer   = buffer;
      }
    }

    switch (found->val) {
      case k_s_dae_asset: {
        (void)ak_dae_assetInf(xst, source, NULL);
        break;
      }

      case AKT_FLOAT: {
        /*
         Removed for now:
          ak_xml_attrui(xst, _s_dae_digits);
          ak_xml_attrui(xst, _s_dae_magnitude);
        */

        if ((content = ak_xml_rawval(xst))) {
          buffer->data = ak_heap_alloc(xst->heap, buffer, arraySize);
          ak_strtomf(&content,
                     buffer->data,
                     1,
                     arrayCount);
          xmlFree(content);
        }

        break;
      }

      case AKT_INT: {
        /*
         Removed for now:
          ak_xml_attrui_def(xst, _s_dae_minInclusive, -2147483647);
          ak_xml_attrui_def(xst, _s_dae_maxInclusive,  2147483647);
         */

        if ((content = ak_xml_rawval(xst))) {
          buffer->reserved = AKT_INT;
          buffer->data = ak_heap_alloc(xst->heap, buffer, arraySize);
          ak_strtomi(&content,
                     buffer->data,
                     1,
                     arrayCount);
          xmlFree(content);
        }
        break;
      }

      case AKT_BOOL: {
        if ((content = ak_xml_rawval(xst))) {
          buffer->data = ak_heap_alloc(xst->heap, buffer, arraySize);
          ak_strtomb(&content,
                     buffer->data,
                     1,
                     arrayCount);
          xmlFree(content);
        }
        break;
      }

      case AKT_IDREF:
      case AKT_NAME:
      case AKT_SIDREF:
      case AKT_TOKEN: {
        char     *pData;
        char     *tok;
        char    **iter;
        size_t    arrayDataSize;
		    uint32_t  idx;

        /*
         |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|

         the last one is pointer to all data
         */
        arraySize = sizeof(char *) * (arrayCount + 1);
        if ((content = ak_xml_rawval(xst))) {
          arrayDataSize = strlen(content) + arrayCount /* NULL */;
          iter = buffer->data = ak_heap_alloc(xst->heap,
                                              buffer,
                                              arraySize);

          pData = ak_heap_alloc(xst->heap,
                                buffer,
                                arrayDataSize);

          iter[arrayCount] = pData;

          idx = 0;
          for (tok = strtok(content, " \t\r\n");
               tok;
               tok = strtok(NULL, " \t\r\n")) {
            if (idx >= arrayCount)
              break;

            strcpy(pData, tok);
            iter[idx++] = pData;

            pData += strlen(tok);
            *pData++ = '\0';
          }
          xmlFree(content);
        }
        break;
      }
      case k_s_dae_techniquec: {
        AkAccessor   *accessor;
        AkXmlElmState xest2;

        ak_xest_init(xest2, _s_dae_techniquec)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_accessor))
            if (ak_dae_accessor(xst, source, &accessor) == AK_OK)
              source->tcommon = accessor;
          
          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        break;
      }
      case k_s_dae_technique: {
        AkTechnique *tq;
        AkResult ret;

        tq = NULL;
        ret = ak_dae_technique(xst, source, &tq);
        if (ret == AK_OK) {
          if (last_tq)
            last_tq->next = tq;
          else
            source->technique = tq;

          last_tq = tq;
        }
        break;
      }
      default:
        ak_xml_skipelm(xst);
        break;
    }

  skip:
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

done:
  *dest = source;

  return AK_OK;
}
