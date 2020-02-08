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
           AkEnum              (*asEnum)(const char *name),
           uint32_t              enumLen) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkSource    *source;
  AkBuffer    *buffer;
  AkTechnique *tq;
  char        *content;
  uint32_t     count;
  bool         isName;

  heap = dst->heap;
  doc  = dst->doc;
  xml  = xml->val;

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
      xml_t      *xacc;
      AkAccessor *acc;

      if ((xacc = xml_elem(xml, _s_dae_accessor))) {
        acc         = ak_heap_calloc(heap, source, sizeof(*acc));
        acc->count  = xmla_uint32(xml_attr(xacc, _s_dae_count),  0);
        acc->offset = xmla_uint32(xml_attr(xacc, _s_dae_offset), 0);
        acc->stride = xmla_uint32(xml_attr(xacc, _s_dae_stride), 1);

        ak_setypeid(acc, AKT_ACCESSOR);
        url_set(dst, xacc, _s_dae_source, acc, &acc->source);
        
        xacc = xacc->val;
        while (xacc) {
          AkDataParam *dp;
          
          dp = ak_heap_calloc(heap, acc, sizeof(*dp));
          sid_set(xacc, heap, dp);
          
          dp->name = xmla_strdup_by(xacc, heap, _s_dae_name, dp);
          dae_dtype(xmla_strdup_by(xacc, heap, _s_dae_type, dp),  &dp->type);
          
          dp->next   = acc->param;
          acc->param = dp;
          
          xacc = xacc->next;
        }

        if (asEnum) {
          acc->byteStride = enumLen;
          acc->byteLength = acc->count * enumLen;
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
    } else if (xml_valtype(xml) == XML_STRING
               && (content = (char *)xml_string(xml))) {
      count            = xmla_uint32(xml_attr(xml, _s_dae_count), 0);
      buffer           = ak_heap_alloc(heap, source, sizeof(*buffer));
      buffer->name     = xmla_strdup_by(xml, heap, _s_dae_name, buffer);
      source->buffer   = buffer;
      
      xmla_setid(xml, heap, buffer);
      
      if (xml_tag_eq(xml, _s_dae_float_array)) {
        buffer->length = sizeof(float) * count;
        buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
        ak_strtomf(&content, buffer->data, 1, count);
      } else if (xml_tag_eq(xml, _s_dae_int_array)) {
        buffer->length   = sizeof(uint32_t) * count;
        buffer->reserved = AKT_INT;
        buffer->data     = ak_heap_alloc(heap, buffer, buffer->length);
        ak_strtomi(&content, buffer->data, 1, count);
      } else if (xml_tag_eq(xml, _s_dae_bool_array)) {
        buffer->length = sizeof(bool) * count;
        buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
        ak_strtomb(&content, buffer->data, 1, count);
      } else if (xml_tag_eq(xml, _s_dae_IDREF_array)
                 || xml_tag_eq(xml, _s_dae_Name_array)
                 || xml_tag_eq(xml, _s_dae_SIDREF_array)
                 || xml_tag_eq(xml, _s_dae_token_array)) {
        char    *pData;
        char    *tok;
        char   **iter;
        uint32_t idx;
        
        /*
         |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|
         
         the last one is pointer to all data
         */
        
        isName = true;
        if (asEnum) {
          AkEnum enumValue;
          
          buffer->length = enumLen * count;
          buffer->data   = ak_heap_alloc(heap, buffer, buffer->length);
          pData          = buffer->data;
          
          idx = 0;
          for (tok = strtok(content, " \t\r\n");
               tok;
               tok = strtok(NULL, " \t\r\n")) {
            if (idx >= count)
              break;
            
            enumValue = asEnum(tok);
            memcpy(pData + idx * enumLen, &enumValue, enumLen);
            
            idx++;
          }
        } else {
          buffer->length = sizeof(char *) * (count + 1)
                            + strlen(content) + count /* NULL */;
          iter  = buffer->data = ak_heap_alloc(heap, buffer, buffer->length);
          pData = (char *)buffer->data + sizeof(char *) * (count + 1);
          
          iter[count] = pData;
          
          idx = 0;
          for (tok = strtok(content, " \t\r\n");
               tok;
               tok = strtok(NULL, " \t\r\n")) {
            if (idx >= count)
              break;

            strcpy(pData, tok);
            iter[idx++] = pData;

            pData += strlen(tok);
            *pData++ = '\0';
          }
        } /* if asEnum */
      }
    }
    
    xml = xml->next;
  }

  if (source->tcommon && isName && asEnum) {
    source->tcommon->bound  = 1;
    source->tcommon->stride = 1;
  }

  return source;
}
