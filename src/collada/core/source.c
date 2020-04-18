/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "source.h"
#include "../core/asset.h"
#include "../core/techn.h"
#include "../core/enum.h"
#include "../core/value.h"

AkSource* _assetkit_hide
dae_source(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           AkEnum              (*asEnum)(const char *name, size_t nameLen),
           AkTypeId              enumType) {
  AkHeap        *heap;
  AkSource      *source;
  AkBuffer      *buffer;
  AkTechnique   *tq;
  AkAccessor    *acc;
  AkAccessorDAE *accdae;
  const xml_t   *sval;
  uint32_t       count;
  bool           isName;

  heap   = dst->heap;
  isName = false;
  buffer = NULL;
  source = ak_heap_calloc(heap, dst->tempmem, sizeof(*source));
  ak_setypeid(source, AKT_SOURCE);

  xmla_setid(xml, heap, source);
  source->name = xmla_strdup_by(xml, heap, _s_dae_name, source);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, source, NULL);
    } else if (xml_tag_eq(xml, _s_dae_techniquec)) {
      xml_t       *xacc;
      AkDataParam *dp_last;

      if ((xacc = xml_elem(xml, _s_dae_accessor))) {
        acc         = ak_heap_calloc(heap, source, sizeof(*acc));
        accdae      = ak_heap_calloc(heap, acc, sizeof(*accdae));
        
        ak_heap_setUserData(heap, acc, accdae);
        
        acc->count     = xmla_u32(xmla(xacc, _s_dae_count),  0);
        accdae->offset = xmla_u32(xmla(xacc, _s_dae_offset), 0);
        accdae->stride = xmla_u32(xmla(xacc, _s_dae_stride), 1);

        ak_setypeid(acc, AKT_ACCESSOR);
        url_set(dst, xacc, _s_dae_source, accdae, &accdae->source);

        xacc    = xacc->val;
        dp_last = NULL;

        while (xacc) {
          AkDataParam *dp;
          
          dp = ak_heap_calloc(heap, acc, sizeof(*dp));
          sid_set(xacc, heap, dp);

          dp->name = xmla_strdup_by(xacc, heap, _s_dae_name, dp);
          dae_dtype(xmla_strdup_by(xacc, heap, _s_dae_type, dp),  &dp->type);
          
          if (dp_last)
            dp_last->next = dp;
          else
            accdae->param = dp;
          dp_last = dp;
        
          xacc = xacc->next;
        }

        source->tcommon = acc;

        /* append accessor to global list */
        /* this will be prepared in postprocess */
        flist_sp_insert(&dst->accessors, acc);
      }
    } else if (xml_tag_eq(xml, _s_dae_technique)) {
      tq                = dae_techn(xml, heap, source);
      tq->next          = source->technique;
      source->technique = tq;
    } else if (xml_valtype(xml) == XML_STRING && (sval = xmls(xml))) {
      count            = xmla_u32(xmla(xml, _s_dae_count), 0);
      buffer           = ak_heap_alloc(heap, source, sizeof(*buffer));
      buffer->name     = xmla_strdup_by(xml, heap, _s_dae_name, buffer);
      source->buffer   = buffer;
      
      xmla_setid(xml, heap, buffer);
      
      if (xml_tag_eq(xml, _s_dae_float_array)) {
        buffer->length = sizeof(float) * count;
        buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
        xml_strtof_fast(sval, buffer->data, count);
        
        ak_setUserData(buffer, (void *)(uintptr_t)AKT_FLOAT);
      } else if (xml_tag_eq(xml, _s_dae_int_array)) {
        buffer->length = sizeof(uint32_t) * count;
        buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
        xml_strtoi_fast(sval, buffer->data, count);
        
        ak_setUserData(buffer, (void *)(uintptr_t)AKT_INT);
      } else if (xml_tag_eq(xml, _s_dae_bool_array)) {
        buffer->length = sizeof(bool) * count;
        buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
        xml_strtob_fast(sval, buffer->data, count);
        
        ak_setUserData(buffer, (void *)(uintptr_t)AKT_BOOL);
      } else if (xml_tag_eq(xml, _s_dae_Name_array)
                 || xml_tag_eq(xml, _s_dae_IDREF_array)
                 || xml_tag_eq(xml, _s_dae_SIDREF_array)
                 || xml_tag_eq(xml, _s_dae_token_array)) {
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
                enumValue = asEnum(tok, toklen);
                memcpy(pData + idx * enumLen, &enumValue, enumLen);

                idx++;
              } while (idx < count && tok < end);
            } while ((v = xmls_next(v)) && (tok = v->val));
          }
        } else {
          ak_setUserData(buffer, (void *)(uintptr_t)AKT_STRING);

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
    }
    
    xml = xml->next;
  }

  if (source->tcommon
      && isName
      && asEnum
      && (accdae = ak_userData(source->tcommon))) {

    accdae->bound  = 1;
    accdae->stride = 1;
  }

  return source;
}
