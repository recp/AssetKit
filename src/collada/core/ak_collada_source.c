/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_source.h"
#include "../ak_collada_asset.h"
#include "../ak_collada_technique.h"

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
ak_dae_source(void * __restrict memParent,
              xmlTextReaderPtr reader,
              AkSource ** __restrict dest) {

  AkSource            *source;
  ak_technique        *last_tq;
  ak_technique_common *last_tc;
  
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  source = ak_calloc(memParent, sizeof(*source), 1);

  _xml_readAttr(source, source->id, _s_dae_id);
  _xml_readAttr(source, source->name, _s_dae_name);

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

    _xml_beginElement(_s_dae_source);

    found = bsearch(nodeName,
                    sourceMap,
                    sourceMapLen,
                    sizeof(sourceMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_asset: {
        ak_assetinf *assetInf;
        int ret;

        assetInf = NULL;
        ret = ak_dae_assetInf(source, reader, &assetInf);
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
        obj = ak_objAlloc(source,
                          sizeof(*boolArray) + arraySize,
                          AK_SOURCE_ARRAY_TYPE_BOOL,
                          true);
        boolArray = ak_objGet(obj);

        _xml_readAttr(obj, boolArray->id, _s_dae_id);
        _xml_readAttr(obj, boolArray->name, _s_dae_name);

        _xml_readText(source, content);

        boolArray->base.count = arrayCount;
        ak_strtomb(&content,
                   boolArray->base.items,
                   1,
                   arrayCount);

        source->data = obj;

        ak_free(content);
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
        obj = ak_objAlloc(source,
                          sizeof(*floatAray) + arraySize,
                          AK_SOURCE_ARRAY_TYPE_FLOAT,
                          true);
        floatAray = ak_objGet(obj);

        _xml_readAttr(obj, floatAray->id, _s_dae_id);
        _xml_readAttr(obj, floatAray->name, _s_dae_name);

        _xml_readAttrUsingFnWithDef(floatAray->digits,
                                    _s_dae_digits,
                                    7, /* default */
                                    strtoul, NULL, 10);

        _xml_readAttrUsingFnWithDef(floatAray->magnitude,
                                    _s_dae_magnitude,
                                    38, /* default */
                                    strtoul, NULL, 10);

        _xml_readText(source, content);

        floatAray->base.count = arrayCount;
        ak_strtomf(&content,
                   floatAray->base.items,
                   1,
                   arrayCount);

        source->data = obj;

        ak_free(content);
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
        obj = ak_objAlloc(source,
                          sizeof(*intAray) + arraySize,
                          AK_SOURCE_ARRAY_TYPE_INT,
                          true);
        intAray = ak_objGet(obj);

        _xml_readAttr(obj, intAray->id, _s_dae_id);
        _xml_readAttr(obj, intAray->name, _s_dae_name);

        _xml_readAttrUsingFnWithDef(intAray->minInclusive,
                                    _s_dae_minInclusive,
                                    -2147483648, /* default */
                                    strtoul, NULL, 10);

        _xml_readAttrUsingFnWithDef(intAray->maxInclusive,
                                    _s_dae_maxInclusive,
                                    2147483647, /* default */
                                    strtoul, NULL, 10);

        _xml_readText(source, content);

        intAray->base.count = arrayCount;
        ak_strtomi(&content,
                   intAray->base.items,
                   1,
                   arrayCount);
        
        source->data = obj;

        ak_free(content);
        break;
      }
      case AK_SOURCE_ARRAY_TYPE_IDREF:
      case AK_SOURCE_ARRAY_TYPE_NAME:
      case AK_SOURCE_ARRAY_TYPE_SIDREF:
      case AK_SOURCE_ARRAY_TYPE_TOKEN: {
        char           *content;
        AkObject       *obj;
        AkStringArrayN *stringAray;
        char           *pData;
        char           *tok;
        AkUInt64        arrayCount;
        size_t          arraySize;
        size_t          arrayDataSize;
        int             idx;

        _xml_readAttrUsingFnWithDef(arrayCount,
                                    _s_dae_count,
                                    0,
                                    strtoul, NULL, 10);

        _xml_readText(source, content);

        /* 
         |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|
         
         the last one is pointer to all data
         */
        arraySize = sizeof(char *) * arrayCount + 1;
        arrayDataSize = strlen(content) + arrayCount /* NULL */;

        obj = ak_objAlloc(source,
                          sizeof(*stringAray) + arraySize,
                          found->val,
                          true);
        stringAray = ak_objGet(obj);

        _xml_readAttr(obj, stringAray->id, _s_dae_id);
        _xml_readAttr(obj, stringAray->name, _s_dae_name);

        pData = ak_malloc(stringAray,
                          arrayDataSize);

        stringAray->base.count = arrayCount;
        stringAray->base.items[arrayCount] = pData;

        idx = 0;
        for (tok = strtok(content, " ");
             tok;
             tok = strtok(NULL, " ")) {
          if (idx >= arrayCount)
            break;

          strcpy(pData, tok);
          stringAray->base.items[idx++] = pData;

          pData += strlen(tok);
          *pData++ = '\0';
        }

        source->data = obj;

        ak_free(content);
        break;
      }
      case k_s_dae_techniquec: {
        ak_technique_common *tc;
        int                  ret;

        tc = NULL;
        ret = ak_dae_techniquec(source, reader, &tc);
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
        ak_technique *tq;
        int           ret;

        tq = NULL;
        ret = ak_dae_technique(source, reader, &tq);
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
        _xml_skipElement;
        break;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = source;

  return AK_OK;
}
