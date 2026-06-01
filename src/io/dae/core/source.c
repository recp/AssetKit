/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "source.h"
#include "../core/asset.h"
#include "../core/techn.h"
#include "../core/enum.h"
#include "../core/value.h"

AK_HIDE
DaeSource*
dae_source(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           AkEnum              (*asEnum)(const char *name, size_t nameLen),
           AkTypeId              enumType) {
  AkHeap        *heap;
  DaeSource     *source;
  AkBuffer      *buffer;
  AkTechnique   *tq;
  AkAccessor    *acc;
  AkAccessorDAE *accdae;
  const xml_t   *sval;
  void          *rootmemp, *tempmem;
  uint32_t       count;
  AkTypeId       t;
  bool           isName;
  double         profStart, profStep;

  profStart = DAE_PROF_START(dst);
  heap     = dst->heap;
  rootmemp = ak_heap_data(heap->data);
  tempmem  = dst->tempmem;
  isName   = false;
  buffer   = NULL;
  source   = ak_heap_calloc(heap, tempmem, sizeof(*source));
  ak_setypeid(source, DAE_TYPE_SOURCE);

  xmla_setid(xml, heap, source);
  source->name = DAE_XMLA_STRDUP8(xml, heap, name, source);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, source, NULL);
    } else if (DAE_XML_TAG_EQ(xml, techniquec)) {
      xml_t       *xacc;
      AkDataParam *dp_last;
      profStep = DAE_PROF_START(dst);

      if ((xacc = DAE_XML_ELEM8(xml, accessor))) {
        acc         = ak_heap_calloc(heap, rootmemp, sizeof(*acc));
        accdae      = ak_heap_calloc(heap, tempmem,  sizeof(*accdae));
        
        ak_heap_setUserData(heap, acc, accdae);
        
        acc->count     = xmla_u32(DAE_XMLA8(xacc, count),  0);
        accdae->offset = xmla_u32(DAE_XMLA8(xacc, offset), 0);
        accdae->stride = xmla_u32(DAE_XMLA8(xacc, stride), 1);

        ak_setypeid(acc, AKT_ACCESSOR);
        DAE_URL_SET(dst, xacc, source, accdae, &accdae->source);

        xacc    = xacc->val;
        dp_last = NULL;

        while (xacc) {
          AkDataParam *dp;
          
          dp = ak_heap_calloc(heap, accdae, sizeof(*dp));
          sid_set(xacc, heap, dp);

          dp->name = DAE_XMLA_STRDUP8(xacc, heap, name, dp);
          dae_dtype_attr(DAE_XMLA8(xacc, type),  &dp->type);
          
          AK_APPEND_FLINK(accdae->param, dp_last, dp);
          xacc = xacc->next;
        }

        source->accessor = acc;

        /* append accessor to global list */
        /* this will be prepared in postprocess */
        flist_sp_insert(&dst->accessors, acc);
      }
      DAE_PROF_ACC(dst, profGeomAccessor, profGeomAccessorCount, profStep);
    } else if (DAE_XML_TAG_EQ(xml, technique)) {
      tq       = dae_techn(xml, heap, source);
      tq->next = (AkTechnique *)source->reserved;
      source->reserved = tq;
    } else if (xml_valtype(xml) == XML_STRING && (sval = xmls(xml))) {
      profStep         = DAE_PROF_START(dst);
      count            = xmla_u32(DAE_XMLA8(xml, count), 0);
      buffer           = ak_heap_alloc(heap, rootmemp, sizeof(*buffer));
      buffer->name     = DAE_XMLA_STRDUP8(xml, heap, name, buffer);
      source->buffer   = buffer;
      
      xmla_setid(xml, heap, buffer);
      
      if (DAE_XML_TAG_EQ(xml, float_array)) {
        buffer->length = sizeof(float) * count;
        buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
        xml_strtof_fast(sval, buffer->data, count);
        
        ak_setUserData(buffer, (void *)(uintptr_t)AKT_FLOAT);
      } else if (DAE_XML_TAG_EQ(xml, int_array)) {
        buffer->length = sizeof(uint32_t) * count;
        buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
        xml_strtoi_fast(sval, buffer->data, count);
        
        ak_setUserData(buffer, (void *)(uintptr_t)AKT_INT);
      } else if (DAE_XML_TAG_EQ(xml, bool_array)) {
        buffer->length = sizeof(bool) * count;
        buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
        xml_strtob_fast(sval, buffer->data, count);
        
        ak_setUserData(buffer, (void *)(uintptr_t)AKT_BOOL);
      } else if ((DAE_XML_TAG_EQ(xml, Name_array)   & (1|(t = AKT_NAME)))
              || (DAE_XML_TAG_EQ(xml, IDREF_array)  & (1|(t = AKT_IDREF)))
              || (DAE_XML_TAG_EQ(xml, SIDREF_array) & (1|(t = AKT_SIDREF)))
              || (DAE_XML_TAG_EQ(xml, token_array)  & (1|(t = AKT_TOKEN)))) {
        char        *pData, **iter, *tok, *tok_begin, *end, c;
        const xml_t *v;
        size_t       srclen, toklen, enumLen;
        uint32_t     idx;
        AkEnum       enumValue;

        /*
         |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|
         
         the last one is pointer to all data
         */
        
        isName = true;
        idx    = 0;

        if (asEnum) {
          ak_setUserData(buffer, (void *)(uintptr_t)enumType);
          
          enumLen        = ak_typeDesc(enumType)->size;
          buffer->length = enumLen * count;
          buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
          pData          = buffer->data;

          if ((v = sval) && (tok = v->val)) {
            do {
              if (idx >= count)
                break;

              srclen = v->valsize;
              end    = tok + srclen;

              do {
                while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
                  tok++;
                
                tok_begin = tok;
                
                while (tok < end && !((void)(c = *tok), AK_ARRAY_SEP_CHECK))
                  tok++;
                
                toklen    = tok - tok_begin;
                enumValue = asEnum(tok_begin, toklen);
                memcpy(pData + idx * enumLen, &enumValue, enumLen);

                idx++;
              } while (idx < count && tok < end);
            } while ((v = xmls_next(v)) && (tok = v->val));
          }
        } else {
          ak_setUserData(buffer, (void *)(uintptr_t)t);

          buffer->length = sizeof(char *) * count * 2
                            + xmls_sumlen(sval) + 1 /* NULL */;
          iter  = buffer->data = ak_heap_alloc(heap, buffer, buffer->length);
          pData = (char *)buffer->data + sizeof(char *) * (count + 1);
          
          iter[count] = pData;

          if ((v = sval) && (tok = v->val)) {
            do {
              if (idx >= count)
                break;

              srclen = v->valsize;
              end    = tok + srclen;

              do {
                while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
                  tok++;
                
                tok_begin = tok;
                
                while (tok < end && !((void)(c = *tok), AK_ARRAY_SEP_CHECK))
                  tok++;

                toklen = tok - tok_begin;
                memcpy(pData, tok_begin, toklen);
                iter[idx++] = pData;
                
                pData += toklen;
                *pData++ = '\0';
              } while (idx < count && tok < end);
            } while ((v = xmls_next(v)) && (tok = v->val));
          }
        } /* if asEnum */
      }
      DAE_PROF_ACC(dst, profGeomArray, profGeomArrayCount, profStep);
    }
    
    xml = xml->next;
  }

  if (source->accessor
      && isName
      && asEnum
      && (accdae = ak_userData(source->accessor))) {

    accdae->bound  = 1;
    accdae->stride = 1;
  }

  DAE_PROF_ACC(dst, profGeomSource, profGeomSourceCount, profStart);

  return source;
}
