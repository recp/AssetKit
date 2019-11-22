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
#include "../core/dae_enums.h"

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
dae_source(AkXmlState * __restrict xst,
           void       * __restrict memParent,
           AkEnum                (*asEnum)(const char *name),
           uint32_t                enumLen,
           AkSource  ** __restrict dest) {
  AkSource     *source;
  AkBuffer     *buffer;
  AkTechnique  *last_tq;
  AkXmlElmState xest;
  bool          isName;

  isName = false;
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

    arrayCount       = ak_xml_attrui(xst, _s_dae_count);
    buffer           = ak_heap_alloc(xst->heap, source, sizeof(*buffer));
    buffer->name     = ak_xml_attr(xst, buffer, _s_dae_name);
    buffer->reserved = found->val;
    source->buffer   = buffer;

    ak_xml_readid(xst, buffer);

    switch (found->val) {
      case k_s_dae_asset: {
        (void)dae_assetInf(xst, source, NULL);
        break;
      }

      case AKT_FLOAT: {
        /*
         Removed for now:
          ak_xml_attrui(xst, _s_dae_digits);
          ak_xml_attrui(xst, _s_dae_magnitude);
        */

        if ((content = ak_xml_rawval(xst))) {
          buffer->length = sizeof(AkFloat) * arrayCount;
          buffer->data   = ak_heap_alloc(xst->heap, buffer, buffer->length);
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
          buffer->length   = sizeof(AkInt) * arrayCount;
          buffer->reserved = AKT_INT;
          buffer->data     = ak_heap_alloc(xst->heap, buffer, buffer->length);
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
          buffer->length = sizeof(AkBool) * arrayCount;
          buffer->data   = ak_heap_alloc(xst->heap, buffer, buffer->length);
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
		    uint32_t  idx;

        /*
         |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|

         the last one is pointer to all data
         */

        isName = true;
        if ((content = ak_xml_rawval(xst))) {
          if (asEnum) {
            AkEnum enumValue;

            buffer->length = enumLen * arrayCount;
            buffer->data   = ak_heap_alloc(xst->heap, buffer, buffer->length);
            pData          = buffer->data;

            idx = 0;
            for (tok = strtok(content, " \t\r\n");
                 tok;
                 tok = strtok(NULL, " \t\r\n")) {
              if (idx >= arrayCount)
                break;

              enumValue = asEnum(tok);
              memcpy(pData + idx * enumLen, &enumValue, enumLen);

              idx++;
            }
          } else {
            buffer->length = sizeof(char *) * (arrayCount + 1)
                               + strlen(content) + arrayCount /* NULL */;
            iter  = buffer->data = ak_heap_alloc(xst->heap,
                                                 buffer,
                                                 buffer->length);
            pData = (char *)buffer->data + sizeof(char *) * (arrayCount + 1);

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
            if (dae_accessor(xst, source, &accessor) == AK_OK) {
              source->tcommon = accessor;

              if (asEnum) {
                accessor->byteStride = enumLen;
                /* accessor->byteLength = accessor->count * enumLen; */
              }
            }
          
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
        ret = dae_technique(xst, source, &tq);
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

  if (source->tcommon
      && isName
      && asEnum) {
    source->tcommon->bound  = 1;
    source->tcommon->stride = 1;
  }

  *dest = source;

  return AK_OK;
}
