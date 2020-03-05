/*
* Copyright (c), Recep Aslantas.
*
* MIT License (MIT), http://opensource.org/licenses/MIT
* Full license can be found in the LICENSE file
*/

#ifndef ak_src_xml_h
#define ak_src_xml_h

#include "collada/strpool.h"
#include <xml/xml.h>

AK_INLINE
char *
xml_strdup(const xml_t * __restrict xobj,
           AkHeap      * __restrict heap,
           void        * __restrict parent) {
  const char *s;

  if (!(xobj = xobj->val) || !(s = xmls(xobj)))
    return NULL;

  return ak_heap_strndup(heap, parent, s, xobj->valsize);
}

AK_INLINE
char *
xmla_strdup(const xml_attr_t * __restrict attr,
            AkHeap           * __restrict heap,
            void             * __restrict parent) {
  const char *s;
  
  if (!(s = attr->val))
    return NULL;

  return ak_heap_strndup(heap, parent, s, attr->valsize);
}

AK_INLINE
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

AK_INLINE
void
xmla_setid(const xml_t * __restrict xobject,
           AkHeap      * __restrict heap,
           void        * __restrict memptr) {
  xml_attr_t *att;
  
  if ((att = xmla(xobject, _s_dae_id)))
    ak_setId(memptr, ak_heap_strndup(heap, memptr, att->val, att->valsize));
}

#endif /* ak_src_xml_h */
