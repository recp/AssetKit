/*
* Copyright (c), Recep Aslantas.
*
* MIT License (MIT), http://opensource.org/licenses/MIT
* Full license can be found in the LICENSE file
*/

#ifndef ak_src_xml_h
#define ak_src_xml_h

AK_INLINE
char *
xml_strdup(const xml_t * __restrict xobject,
           AkHeap      * __restrict heap,
           void        * __restrict parent) {
  return ak_heap_strndup(heap, parent, xml_string(xobject), xobject->valsize);
}

AK_INLINE
char *
xmla_strdup(const xml_attr_t * __restrict attr,
            AkHeap           * __restrict heap,
            void             * __restrict parent) {
  return ak_heap_strndup(heap, parent, attr->val, attr->valsize);
}

AK_INLINE
char *
xmla_strdup_by(const xml_t * __restrict xobject,
               AkHeap      * __restrict heap,
               const char  * __restrict name,
               void        * __restrict parent) {
  xml_attr_t *attr;
  
  if ((attr = xml_attr(xobject, name))) {
    ak_heap_strndup(heap, parent, attr->val, attr->valsize);
  }

  return NULL;
}

AK_INLINE
char *
xmla_setid(void        * __restrict parent,
           AkHeap      * __restrict heap,
           const xml_t * __restrict xobject) {
  xml_attr_t *attr;
  
  if ((attr = xml_attr(xobject, name))) {
    ak_setId(memptr, ak_heap_strndup(heap, parent, attr->val, attr->valsize));
  }
  
  return NULL;
}

#endif /* ak_src_xml_h */
