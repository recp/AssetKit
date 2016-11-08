/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_source.h"
#include "../core/ak_collada_asset.h"
#include "../core/ak_collada_technique.h"

#define k_s_dae_asset        100
#define k_s_dae_techniquec   101
#define k_s_dae_technique    102

static ak_enumpair sourceMap[] = {
  {_s_dae_bool_array,   AK_SOURCE_ARRAY_TYPE_BOOL},
  {_s_dae_float_array,  AK_SOURCE_ARRAY_TYPE_FLOAT},
  {_s_dae_IDREF_array,  AK_SOURCE_ARRAY_TYPE_IDREF},
  {_s_dae_int_array,    AK_SOURCE_ARRAY_TYPE_INT},
  {_s_dae_Name_array,   AK_SOURCE_ARRAY_TYPE_NAME},
  {_s_dae_SIDREF_array, AK_SOURCE_ARRAY_TYPE_SIDREF},
  {_s_dae_token_array,  AK_SOURCE_ARRAY_TYPE_TOKEN},
  {_s_dae_asset,        k_s_dae_asset},
  {_s_dae_techniquec,   k_s_dae_techniquec},
  {_s_dae_technique,    k_s_dae_technique},
};

static size_t sourceMapLen = 0;

AkResult _assetkit_hide
ak_dae_source(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkSource ** __restrict dest) {
  AkSource          *source;
  AkTechnique       *last_tq;
  AkTechniqueCommon *last_tc;

  source = ak_heap_calloc(xst->heap, memParent, sizeof(*source), true);

  _xml_readId(source);
  _xml_readAttr(source, source->name, _s_dae_name);

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
  last_tc = NULL;

  do {
    const ak_enumpair *found;

    if (ak_xml_beginelm(xst, _s_dae_source))
      break;

    found = bsearch(xst->nodeName,
                    sourceMap,
                    sourceMapLen,
                    sizeof(sourceMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_asset: {
        AkAssetInf *assetInf;
        AkResult ret;

        assetInf = NULL;
        ret = ak_dae_assetInf(xst, source, &assetInf);
        if (ret == AK_OK)
          source->inf = assetInf;

        break;
      }
      case AK_SOURCE_ARRAY_TYPE_BOOL: {
        char         *content;
        AkObject     *obj;
        AkBoolArrayN *boolArray;
        AkUInt64      arrayCount;
        size_t        arraySize;

        _xml_readAttrUsingFnWithDef(arrayCount,
                                    _s_dae_count,
                                    0,
                                    strtoul, NULL, 10);

        arraySize = sizeof(AkBool) * arrayCount;
        obj = ak_objAlloc(xst->heap,
                          source,
                          sizeof(*boolArray) + arraySize,
                          AK_SOURCE_ARRAY_TYPE_BOOL,
                          true,
                          true);
        boolArray = ak_objGet(obj);

        _xml_readId(obj);
        _xml_readAttr(obj, boolArray->name, _s_dae_name);

        boolArray->count = arrayCount;

        content = ak_xml_rawval(xst);;
        if (content) {
          ak_strtomb(&content,
                     boolArray->items,
                     1,
                     (AkUInt)arrayCount);

          source->data = obj;
          xmlFree(content);
        } else {
          ak_free(obj);
        }

        break;
      }
      case AK_SOURCE_ARRAY_TYPE_FLOAT: {
        char          *content;
        AkObject      *obj;
        AkFloatArrayN *floatAray;
        AkUInt64       arrayCount;
        size_t         arraySize;

        _xml_readAttrUsingFnWithDef(arrayCount,
                                    _s_dae_count,
                                    0,
                                    strtoul, NULL, 10);

        arraySize = sizeof(AkFloat) * arrayCount;
        obj = ak_objAlloc(xst->heap,
                          source,
                          sizeof(*floatAray) + arraySize,
                          AK_SOURCE_ARRAY_TYPE_FLOAT,
                          true,
                          true);
        floatAray = ak_objGet(obj);

        _xml_readId(obj);
        _xml_readAttr(obj, floatAray->name, _s_dae_name);

        _xml_readAttrUsingFnWithDef(floatAray->digits,
                                    _s_dae_digits,
                                    7, /* default */
                                    (AkInt)strtoul, NULL, 10);

        _xml_readAttrUsingFnWithDef(floatAray->magnitude,
                                    _s_dae_magnitude,
                                    38, /* default */
                                    (AkInt)strtoul, NULL, 10);

        floatAray->count = arrayCount;
        content = ak_xml_rawval(xst);

        if (content) {
          ak_strtomf(&content,
                     floatAray->items,
                     1,
                     (AkUInt)arrayCount);
          source->data = obj;
          xmlFree(content);
        } else {
          ak_free(obj);
        }

        break;
      }
      case AK_SOURCE_ARRAY_TYPE_INT: {
        char        *content;
        AkObject    *obj;
        AkIntArrayN *intAray;
        AkUInt64     arrayCount;
        size_t       arraySize;

        _xml_readAttrUsingFnWithDef(arrayCount,
                                    _s_dae_count,
                                    0,
                                    strtoul, NULL, 10);

        arraySize = sizeof(AkInt) * arrayCount;
        obj = ak_objAlloc(xst->heap,
                          source,
                          sizeof(*intAray) + arraySize,
                          AK_SOURCE_ARRAY_TYPE_INT,
                          true,
                          true);
        intAray = ak_objGet(obj);

        _xml_readId(obj);
        _xml_readAttr(obj, intAray->name, _s_dae_name);

        _xml_readAttrUsingFnWithDef(intAray->minInclusive,
                                    _s_dae_minInclusive,
                                    -2147483647, /* default */
                                    (AkInt)strtoul, NULL, 10);

        _xml_readAttrUsingFnWithDef(intAray->maxInclusive,
                                    _s_dae_maxInclusive,
                                    2147483647, /* default */
                                    (AkInt)strtoul, NULL, 10);

        intAray->count = arrayCount;
        content = ak_xml_rawval(xst);

        if (content) {
          ak_strtomi(&content,
                     intAray->items,
                     1,
                     (AkUInt)arrayCount);

          source->data = obj;
          xmlFree(content);
        } else {
          ak_free(obj);
        }

        break;
      }
      case AK_SOURCE_ARRAY_TYPE_IDREF:
      case AK_SOURCE_ARRAY_TYPE_NAME:
      case AK_SOURCE_ARRAY_TYPE_SIDREF:
      case AK_SOURCE_ARRAY_TYPE_TOKEN: {
        char          *content;
        AkObject       *obj;
        AkStringArrayN *stringAray;
        char           *pData;
        char           *tok;
        AkUInt64        arrayCount;
        size_t          arraySize;
        size_t          arrayDataSize;
        AkUInt64        idx;

        _xml_readAttrUsingFnWithDef(arrayCount,
                                    _s_dae_count,
                                    0,
                                    strtoul, NULL, 10);

        /*
         |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|

         the last one is pointer to all data
         */
        arraySize = sizeof(char *) * (arrayCount + 1);

        obj = ak_objAlloc(xst->heap,
                          source,
                          sizeof(*stringAray) + arraySize,
                          found->val,
                          true,
                          true);
        stringAray = ak_objGet(obj);
        stringAray->count = arrayCount;

        _xml_readId(obj);
        _xml_readAttr(obj, stringAray->name, _s_dae_name);

        content = ak_xml_rawval(xst);
        if (content) {
          arrayDataSize = strlen(content) + arrayCount /* NULL */;
          pData = ak_heap_alloc(xst->heap,
                                stringAray,
                                arrayDataSize,
                                false);

          stringAray->items[arrayCount] = pData;

          idx = 0;
          for (tok = strtok(content, " \t\r\n");
               tok;
               tok = strtok(NULL, " \t\r\n")) {
            if (idx >= arrayCount)
              break;

            strcpy(pData, tok);
            stringAray->items[idx++] = pData;

            pData += strlen(tok);
            *pData++ = '\0';
          }

          source->data = obj;
          xmlFree(content);
        } else {
          ak_free(obj);
        }

        break;
      }
      case k_s_dae_techniquec: {
        AkTechniqueCommon *tc;
        AkResult ret;

        tc = NULL;
        ret = ak_dae_techniquec(xst, source, &tc);
        if (ret == AK_OK) {
          if (last_tc)
            last_tc->next = tc;
          else
            source->techniqueCommon = tc;

          last_tc = tc;
        }
        
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
        ak_xml_skipelm(xst);;
        break;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

done:
  *dest = source;

  return AK_OK;
}
