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

AK_EXPORT
char *
xml_strdup(const xml_t * __restrict xobj,
           AkHeap      * __restrict heap,
           void        * __restrict parent);

AK_EXPORT
unsigned long
xml_strtof_fast(const xml_t * __restrict xobj,
                AkFloat     * __restrict dest,
                unsigned long            n);

AK_EXPORT
unsigned long
xml_strtoui_fast(const xml_t * __restrict xobj,
                 AkUInt      * __restrict dest,
                 unsigned long            n);

AK_EXPORT
unsigned long
xml_strtoi_fast(const xml_t * __restrict xobj,
                AkInt       * __restrict dest,
                unsigned long            n);

AK_EXPORT
unsigned long
xml_strtob_fast(const xml_t * __restrict xobj,
                AkBool      * __restrict dest,
                unsigned long            n);

AK_EXPORT
size_t
xml_strtok_count_fast(const xml_t * __restrict xobj,
                      size_t      * __restrict len);

AK_EXPORT
AkResult
xml_strtof_arrayL(AkHeap         * __restrict heap,
                  void           * __restrict memp,
                  const xml_t    * __restrict xobj,
                  AkFloatArrayL ** __restrict array);

AK_EXPORT
AkResult
xml_strtoui_array(AkHeap       * __restrict heap,
                  void         * __restrict memp,
                  const xml_t  * __restrict xobj,
                  AkUIntArray ** __restrict array);

AK_EXPORT
char *
xmla_strdup(const xml_attr_t * __restrict attr,
            AkHeap           * __restrict heap,
            void             * __restrict parent);

AK_EXPORT
char *
xmla_strdup_by(const xml_t * __restrict xobject,
               AkHeap      * __restrict heap,
               const char  * __restrict name,
               void        * __restrict parent);

AK_EXPORT
void
xmla_setid(const xml_t * __restrict xobject,
           AkHeap      * __restrict heap,
           void        * __restrict memptr);

#endif /* ak_src_xml_h */
