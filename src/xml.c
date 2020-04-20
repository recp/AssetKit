/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "common.h"
#include "xml.h"

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
  v = xmls(xobj); /* because len > 0 */

  do {
    memcpy(p, v->val, v->valsize);
    p += v->valsize;
  } while ((v = xmls_next(v)));

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
  
  while ((rem = ak_strtof_fast(v->val,
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
  
  while ((rem = ak_strtoui_fast(v->val,
                                v->valsize,
                                rem,
                                dest + n - rem))
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

  while ((rem = ak_strtoi_fast(v->val,
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

  
  while ((rem = ak_strtob_fast(v->val,
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
    count     += ak_strtok_count_fast(p->val, &l);
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
  AkUIntArray  *arr;
  unsigned long count;

  if ((count = (unsigned long)xml_strtok_count_fast(xobj, NULL)) == 0)
    return AK_ERR;

  arr = ak_heap_alloc(heap, memp, sizeof(*arr) + sizeof(AkUInt) * count);
  xml_strtoui_fast(xobj, arr->items, count);

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
  
  if (!(s = attr->val))
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
  
  if ((att = xmla(xobject, name)))
    return ak_heap_strndup(heap, parent, att->val, att->valsize);

  return NULL;
}

AK_EXPORT
void
xmla_setid(const xml_t * __restrict xobject,
           AkHeap      * __restrict heap,
           void        * __restrict memptr) {
  xml_attr_t *att;
  
  if ((att = xmla(xobject, _s_dae_id)))
    ak_setId(memptr, ak_heap_strndup(heap, memptr, att->val, att->valsize));
}
