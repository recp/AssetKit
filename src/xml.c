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

#include "common.h"
#include "xml.h"
#include "string_fast.h"

AK_INLINE
char*
xml_str_skip_array_sep(char * __restrict tok,
                       char * __restrict end) {
  char c;

  while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
    tok++;

  return tok;
}

AK_INLINE
unsigned long
xml_strtof_node(char          * __restrict src,
                size_t                     srclen,
                unsigned long              n,
                AkFloat       * __restrict dest) {
  AkFloat      *out;
  char         *tok, *end;
  unsigned long rem;

  if (n == 0)
    return 0;

  out = dest;
  tok = src;
  rem = n;

  if (srclen != 0) {
    end = src + srclen;

    do {
      tok = xml_str_skip_array_sep(tok, end);
      if (tok >= end)
        break;

      tok = ak_str_parse_float_end_fast(tok, end, out++);
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep_fast(tok, NULL, false);
      if (*tok == '\0')
        break;

      tok = ak_str_parse_float_fast(tok, NULL, out++);
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }

  return rem;
}

AK_INLINE
unsigned long
xml_strtoui_node_max(char          * __restrict src,
                     size_t                     srclen,
                     unsigned long              n,
                     AkUInt        * __restrict dest,
                     AkUInt        * __restrict maxValue) {
  char          *tok, *end;
  AkUInt        *out;
  AkUInt         value, maxv;
  unsigned long rem;

  if (n == 0)
    return 0;

  out  = dest;
  tok  = src;
  rem  = n;
  maxv = maxValue ? *maxValue : 0;

  if (srclen != 0) {
    end = src + srclen;

    do {
      tok = xml_str_skip_array_sep(tok, end);
      if (tok >= end)
        break;

      tok = ak_str_parse_uint_end_fast(tok, end, &value);
      *out++ = value;
      if (value > maxv)
        maxv = value;
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    do {
      tok = ak_str_skip_sep_fast(tok, NULL, false);
      if (*tok == '\0')
        break;

      tok = ak_str_parse_uint_fast(tok, NULL, &value);
      *out++ = value;
      if (value > maxv)
        maxv = value;
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }

  if (maxValue)
    *maxValue = maxv;

  return rem;
}

static
AkIndexArray*
xml_index_promote(AkHeap       * __restrict heap,
                  void         * __restrict parent,
                  AkIndexArray * __restrict indices,
                  AkTypeId                  componentType,
                  size_t                    written) {
  AkIndexArray *promoted;
  size_t        i;

  promoted = ak_indexArrayAlloc(heap, parent, indices->count, componentType);
  if (!promoted)
    return NULL;

  switch (indices->componentType) {
    case AKT_UBYTE: {
      uint8_t *src;

      src = (uint8_t *)indices->items;
      if (componentType == AKT_USHORT) {
        uint16_t *dst;

        dst = (uint16_t *)promoted->items;
        for (i = 0; i < written; i++)
          dst[i] = src[i];
      } else {
        uint32_t *dst;

        dst = (uint32_t *)promoted->items;
        for (i = 0; i < written; i++)
          dst[i] = src[i];
      }
      break;
    }
    case AKT_USHORT: {
      uint16_t *src;
      uint32_t *dst;

      src = (uint16_t *)indices->items;
      dst = (uint32_t *)promoted->items;
      for (i = 0; i < written; i++)
        dst[i] = src[i];
      break;
    }
    default:
      break;
  }

  promoted->max = indices->max;
  ak_free(indices);

  return promoted;
}

static
AkTypeId
xml_index_initial_component_type(void) {
  return AKT_UBYTE;
}

static
unsigned long
xml_strtoindex_node(AkHeap        * __restrict heap,
                    void          * __restrict parent,
                    char          * __restrict src,
                    size_t                     srclen,
                    unsigned long              n,
                    AkIndexArray ** __restrict array,
                    AkUInt       * __restrict maxValue,
                    size_t        * __restrict writtenCount) {
  AkIndexArray *indices;
  char         *tok, *end;
  AkUInt        value, maxv;
  unsigned long rem;
  size_t        written;

  indices = *array;
  if (n == 0 || !indices)
    return 0;

  tok     = src;
  rem     = n;
  written = writtenCount ? *writtenCount : 0;
  maxv    = maxValue ? *maxValue : indices->max;

  if (indices->componentType == AKT_UINT) {
    if (srclen != 0) {
      end = src + srclen;
      goto parse_uint_end;
    }

    goto parse_uint;
  }

  if (indices->componentType == AKT_USHORT) {
    if (srclen != 0) {
      end = src + srclen;
      goto parse_ushort_end;
    }

    goto parse_ushort;
  }

  if (indices->componentType != AKT_UBYTE)
    goto done;

  if (srclen != 0) {
    uint8_t *out;

    end = src + srclen;
    out = (uint8_t *)(void *)indices->items + written;

    do {
      tok = xml_str_skip_array_sep(tok, end);
      if (tok >= end)
        break;

      tok = ak_str_parse_uint_index_end_fast(tok, end, &value);
      if (value > UINT8_MAX) {
        if (value > UINT16_MAX) {
          indices = xml_index_promote(heap, parent, indices, AKT_UINT, written);
          if (!indices)
            return rem;
          *array = indices;
          ((uint32_t *)(void *)indices->items)[written++] = value;
          if (value > maxv)
            maxv = value;
          rem--;
          goto parse_uint_end;
        }

        indices = xml_index_promote(heap, parent, indices, AKT_USHORT, written);
        if (!indices)
          return rem;
        *array = indices;
        ((uint16_t *)(void *)indices->items)[written++] = (uint16_t)value;
        if (value > maxv)
          maxv = value;
        rem--;
        goto parse_ushort_end;
      }

      *out++ = (uint8_t)value;
      if (value > maxv)
        maxv = value;
      written++;
      rem--;
    } while (rem > 0ul && tok < end);
  } else {
    uint8_t *out;

    out = (uint8_t *)(void *)indices->items + written;

    do {
      tok = ak_str_skip_sep_fast(tok, NULL, false);
      if (*tok == '\0')
        break;

      tok = ak_str_parse_uint_index_fast(tok, NULL, &value);
      if (value > UINT8_MAX) {
        if (value > UINT16_MAX) {
          indices = xml_index_promote(heap, parent, indices, AKT_UINT, written);
          if (!indices)
            return rem;
          *array = indices;
          ((uint32_t *)(void *)indices->items)[written++] = value;
          if (value > maxv)
            maxv = value;
          rem--;
          goto parse_uint;
        }

        indices = xml_index_promote(heap, parent, indices, AKT_USHORT, written);
        if (!indices)
          return rem;
        *array = indices;
        ((uint16_t *)(void *)indices->items)[written++] = (uint16_t)value;
        if (value > maxv)
          maxv = value;
        rem--;
        goto parse_ushort;
      }

      *out++ = (uint8_t)value;
      if (value > maxv)
        maxv = value;
      written++;
      rem--;
    } while (rem > 0ul && *tok != '\0');
  }

  goto done;

parse_ushort_end:
  do {
    uint16_t *out;

    out = (uint16_t *)(void *)indices->items + written;
    do {
      tok = xml_str_skip_array_sep(tok, end);
      if (tok >= end)
        break;

      tok = ak_str_parse_uint_index_end_fast(tok, end, &value);
      if (value > UINT16_MAX) {
        indices = xml_index_promote(heap, parent, indices, AKT_UINT, written);
        if (!indices)
          return rem;
        *array = indices;
        ((uint32_t *)(void *)indices->items)[written++] = value;
        if (value > maxv)
          maxv = value;
        rem--;
        goto parse_uint_end;
      }

      *out++ = (uint16_t)value;
      if (value > maxv)
        maxv = value;
      written++;
      rem--;
    } while (rem > 0ul && tok < end);
  } while (0);
  goto done;

parse_ushort:
  do {
    uint16_t *out;

    out = (uint16_t *)(void *)indices->items + written;
    do {
      tok = ak_str_skip_sep_fast(tok, NULL, false);
      if (*tok == '\0')
        break;

      tok = ak_str_parse_uint_index_fast(tok, NULL, &value);
      if (value > UINT16_MAX) {
        indices = xml_index_promote(heap, parent, indices, AKT_UINT, written);
        if (!indices)
          return rem;
        *array = indices;
        ((uint32_t *)(void *)indices->items)[written++] = value;
        if (value > maxv)
          maxv = value;
        rem--;
        goto parse_uint;
      }

      *out++ = (uint16_t)value;
      if (value > maxv)
        maxv = value;
      written++;
      rem--;
    } while (rem > 0ul && *tok != '\0');
  } while (0);
  goto done;

parse_uint_end:
  do {
    uint32_t *out;

    out = (uint32_t *)(void *)indices->items + written;
    do {
      tok = xml_str_skip_array_sep(tok, end);
      if (tok >= end)
        break;

      tok = ak_str_parse_uint_index_end_fast(tok, end, &value);
      *out++ = value;
      if (value > maxv)
        maxv = value;
      written++;
      rem--;
    } while (rem > 0ul && tok < end);
  } while (0);
  goto done;

parse_uint:
  do {
    uint32_t *out;

    out = (uint32_t *)(void *)indices->items + written;
    do {
      tok = ak_str_skip_sep_fast(tok, NULL, false);
      if (*tok == '\0')
        break;

      tok = ak_str_parse_uint_index_fast(tok, NULL, &value);
      *out++ = value;
      if (value > maxv)
        maxv = value;
      written++;
      rem--;
    } while (rem > 0ul && *tok != '\0');
  } while (0);

done:
  indices->max = maxv;
  if (maxValue)
    *maxValue = maxv;
  if (writtenCount)
    *writtenCount = written;

  return rem;
}

AK_EXPORT
char *
xml_strdup(const xml_t * __restrict xobj,
           AkHeap      * __restrict heap,
           void        * __restrict parent) {
  char        *s, *p;
  const xml_t *v;
  size_t       len;

  if ((len = xmls_sumlen(xobj)) < 1)
    return NULL;

  s = p = ak_heap_alloc(heap, parent, len);
  if (xobj->type!= XML_STRING) {
    v = xmls(xobj); /* because len > 0 */
    
    do {
      memcpy(p, v->val, v->valsize);
      p += v->valsize;
    } while ((v = xmls_next(v)));
  } else {
    memcpy(p, xobj->val, xobj->valsize);
  }

  s[len - 1] = '\0';
  
  return s;
}

AK_EXPORT
unsigned long
xml_strtof_fast(const xml_t * __restrict xobj,
                AkFloat     * __restrict dest,
                unsigned long            n) {
  const xml_t  *v;
  unsigned long rem;
  
  /* this step must be done before calling this func. */
  /*
  if (!(v = xmls(xobj)))
    return 0;
  */
  
  if (!(v = xobj)
      || !v->val
      || (rem = n) < 1)
    return 0;
  
  while ((rem = xml_strtof_node(v->val,
                                v->valsize,
                                rem,
                                dest + n - rem))
         && (v = xmls_next(v)));
  
  return rem;
}

AK_EXPORT
unsigned long
xml_strtoui_fast(const xml_t  * __restrict xobj,
                 AkUInt * __restrict dest,
                 unsigned long       n) {
  const xml_t  *v;
  unsigned long rem;
  
  /* this step must be done before calling this func. */
  /*
  if (!(v = xmls(xobj)))
    return 0;
  */
  
  if (!(v = xobj)
      || !v->val
      || (rem = n) < 1)
    return 0;
  
  while ((rem = ak_strtoui(v->val,
                           v->valsize,
                           rem,
                           dest + n - rem))
         && (v = xmls_next(v)));
  


  return rem;
}

AK_EXPORT
unsigned long
xml_strtoui_fast_max(const xml_t  * __restrict xobj,
                     AkUInt       * __restrict dest,
                     unsigned long              n,
                     AkUInt       * __restrict maxValue) {
  const xml_t  *v;
  unsigned long rem;

  if (maxValue)
    *maxValue = 0;

  if (!(v = xobj)
      || !v->val
      || (rem = n) < 1)
    return 0;

  while ((rem = xml_strtoui_node_max(v->val,
                                     v->valsize,
                                     rem,
                                     dest + n - rem,
                                     maxValue))
         && (v = xmls_next(v)));

  return rem;
}

AK_EXPORT
unsigned long
xml_strtoi_fast(const xml_t * __restrict xobj,
                AkInt * __restrict dest,
                unsigned long      n) {
  const xml_t  *v;
  unsigned long rem;
  
  /* this step must be done before calling this func. */
  /*
  if (!(v = xmls(xobj)))
    return 0;
  */
  
  if (!(v = xobj)
      || !v->val
      || (rem = n) < 1)
    return 0;

  while ((rem = ak_strtoi(v->val,
                          v->valsize,
                          rem,
                          dest + n - rem))
         && (v = xmls_next(v)));

  return rem;
}

AK_EXPORT
unsigned long
xml_strtob_fast(const xml_t  * __restrict xobj,
                AkBool * __restrict dest,
                unsigned long       n) {
  const xml_t  *v;
  unsigned long rem;
  
  /* this step must be done before calling this func. */
  /*
  if (!(v = xmls(xobj)))
    return 0;
  */
  
  if (!(v = xobj)
      || !v->val
      || (rem = n) < 1)
    return 0;

  
  while ((rem = ak_strtob(v->val,
                          v->valsize,
                          rem,
                          dest + n - rem))
         && (v = xmls_next(v)));

  return rem;
}

AK_EXPORT
size_t
xml_strtok_count_fast(const xml_t  * __restrict xobj,
                      size_t       * __restrict len) {
  const xml_t *v, *p;
  size_t       count, len_total, l;
  
//  if (!(v = xmls(xobj)))
//    return 0;
  
  v = xobj;

  len_total = 0;
  count     = 0;
  p         = v;

  do {
    count     += ak_strtok_count_fast(p->val, p->valsize, &l);
    len_total += l;
  } while ((p = xmls_next(p)));

  if (len)
    *len = len_total;

  return count;
}

AK_EXPORT
AkResult
xml_strtof_arrayL(AkHeap         * __restrict heap,
                  void           * __restrict memp,
                  const xml_t    * __restrict xobj,
                  AkFloatArrayL ** __restrict array) {
  AkFloatArrayL *arr;
  unsigned long  count;

  if ((count = (unsigned long)xml_strtok_count_fast(xobj, NULL)) == 0)
    return AK_ERR;

  arr = ak_heap_alloc(heap, memp, sizeof(*arr) + sizeof(AkFloat) * count);
  xml_strtof_fast(xobj, arr->items, count);

  arr->count = count;
  arr->next  = NULL;

  *array = arr;

  return AK_OK;
}

AK_EXPORT
AkResult
xml_strtoui_array(AkHeap       * __restrict heap,
                  void         * __restrict memp,
                  const xml_t  * __restrict xobj,
                  AkUIntArray ** __restrict array) {
  return xml_strtoui_array_max(heap, memp, xobj, array, NULL);
}

AK_EXPORT
AkResult
xml_strtoui_array_max(AkHeap       * __restrict heap,
                      void         * __restrict memp,
                      const xml_t  * __restrict xobj,
                      AkUIntArray ** __restrict array,
                      AkUInt      * __restrict maxValue) {
  AkUIntArray  *arr;
  unsigned long count;

  if (maxValue)
    *maxValue = 0;

  if ((count = (unsigned long)xml_strtok_count_fast(xobj, NULL)) == 0)
    return AK_ERR;

  arr = ak_heap_alloc(heap, memp, sizeof(*arr) + sizeof(AkUInt) * count);
  xml_strtoui_fast_max(xobj, arr->items, count, maxValue);

  arr->count = count;

  *array = arr;

  return AK_OK;
}

AK_EXPORT
AkResult
xml_strtoui_arrayN(AkHeap       * __restrict heap,
                   void         * __restrict memp,
                   const xml_t  * __restrict xobj,
                   unsigned long             count,
                   AkUIntArray ** __restrict array) {
  return xml_strtoui_arrayN_max(heap, memp, xobj, count, array, NULL);
}

AK_EXPORT
AkResult
xml_strtoui_arrayN_max(AkHeap       * __restrict heap,
                       void         * __restrict memp,
                       const xml_t  * __restrict xobj,
                       unsigned long             count,
                       AkUIntArray ** __restrict array,
                       AkUInt      * __restrict maxValue) {
  AkUIntArray *arr;
  unsigned long rem;

  if (maxValue)
    *maxValue = 0;

  if (count == 0)
    return AK_ERR;

  arr = ak_heap_alloc(heap, memp, sizeof(*arr) + sizeof(AkUInt) * count);
  rem = xml_strtoui_fast_max(xobj, arr->items, count, maxValue);
  if (rem != 0) {
    ak_free(arr);
    return xml_strtoui_array_max(heap, memp, xobj, array, maxValue);
  }

  arr->count = count;

  *array = arr;

  return AK_OK;
}

AK_EXPORT
AkResult
xml_strtoindex_array(AkHeap        * __restrict heap,
                     void          * __restrict memp,
                     const xml_t   * __restrict xobj,
                     AkIndexArray ** __restrict array) {
  return xml_strtoindex_array_max(heap, memp, xobj, array, NULL);
}

AK_EXPORT
AkResult
xml_strtoindex_array_max(AkHeap        * __restrict heap,
                         void          * __restrict memp,
                         const xml_t   * __restrict xobj,
                         AkIndexArray ** __restrict array,
                         AkUInt       * __restrict maxValue) {
  return xml_strtoindex_arrayN_max(heap,
                                   memp,
                                   xobj,
                                   (unsigned long)xml_strtok_count_fast(xobj, NULL),
                                   array,
                                   maxValue);
}

AK_EXPORT
AkResult
xml_strtoindex_arrayN(AkHeap        * __restrict heap,
                      void          * __restrict memp,
                      const xml_t   * __restrict xobj,
                      unsigned long              count,
                      AkIndexArray ** __restrict array) {
  return xml_strtoindex_arrayN_max(heap, memp, xobj, count, array, NULL);
}

AK_EXPORT
AkResult
xml_strtoindex_arrayN_max(AkHeap        * __restrict heap,
                          void          * __restrict memp,
                          const xml_t   * __restrict xobj,
                          unsigned long              count,
                          AkIndexArray ** __restrict array,
                          AkUInt       * __restrict maxValue) {
  AkIndexArray *arr;
  const xml_t  *v;
  unsigned long rem;
  size_t        written;

  if (array)
    *array = NULL;
  if (maxValue)
    *maxValue = 0;

  if (!array || count == 0)
    return AK_ERR;

  arr = ak_indexArrayAlloc(heap,
                           memp,
                           count,
                           xml_index_initial_component_type());
  if (!arr)
    return AK_ERR;

  rem = count;
  written = 0;
  v = xobj;
  while (v && rem) {
    rem = xml_strtoindex_node(heap,
                              memp,
                              v->val,
                              v->valsize,
                              rem,
                              &arr,
                              maxValue,
                              &written);
    v = xmls_next(v);
  }

  if (rem != 0) {
    ak_free(arr);
    if (count == (unsigned long)xml_strtok_count_fast(xobj, NULL))
      return AK_ERR;
    return xml_strtoindex_array_max(heap, memp, xobj, array, maxValue);
  }

  arr->count = count;
  *array = arr;

  return AK_OK;
}

AK_EXPORT
char *
xmla_strdup(const xml_attr_t * __restrict attr,
            AkHeap           * __restrict heap,
            void             * __restrict parent) {
  const char *s;
  
  if (!attr || !(s = attr->val))
    return NULL;

  return ak_heap_strndup(heap, parent, s, attr->valsize);
}

AK_EXPORT
char *
xmla_strdup_by(const xml_t * __restrict xobject,
               AkHeap      * __restrict heap,
               const char  * __restrict name,
               void        * __restrict parent) {
  xml_attr_t *att;

  if (!name)
    return NULL;
  
  if ((att = xmla_sz(xobject, name, strlen(name))))
    return ak_heap_strndup(heap, parent, att->val, att->valsize);

  return NULL;
}

AK_EXPORT
void
xmla_setid(const xml_t * __restrict xobject,
           AkHeap      * __restrict heap,
           void        * __restrict memptr) {
  xml_attr_t *att;
  
  if ((att = xmla_sz(xobject, "id", 2)))
    ak_setId(memptr, ak_heap_strndup(heap, memptr, att->val, att->valsize));
}
