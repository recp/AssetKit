/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__libxml__h_
#define __libassetkit__libxml__h_

#include "ak_common.h"
#include <libxml/xmlreader.h>

typedef AK_ALIGN(16) struct AkXmlState {
  AkHeap          *heap;
  AkDoc           *doc;
  xmlTextReaderPtr reader;
  const xmlChar   *nodeName;
  int              nodeType;
  int              nodeRet;
} AkXmlState;

#define _xml_eq(a, b) (xmlStrcasecmp((xmlChar *)a, (xmlChar *)b) == 0)

void
ak_xml_readnext(AkXmlState * __restrict xst);

bool
ak_xml_beginelm(AkXmlState * __restrict xst,
                const char *elmname);

void
ak_xml_endelm(AkXmlState * __restrict xst);

void
ak_xml_skipelm(AkXmlState * __restrict xst);

char *
ak_xml_val(AkXmlState * __restrict xst,
           void * __restrict parent);

char *
ak_xml_rawcval(AkXmlState * __restrict xst);

char *
ak_xml_rawval(AkXmlState * __restrict xst);

#define _xml_eqElm(b)                                                         \
  (xst->nodeType == XML_ELEMENT_NODE                                               \
   && xmlStrcasecmp(xst->nodeName, (xmlChar *)b) == 0)

#define _xml_eqDecl(b)                                                        \
  (xst->nodeType == XML_ELEMENT_DECL                                               \
   && xmlStrcasecmp(xst->nodeName, (xmlChar *)b) == 0)

#define _xml_readTextUsingFn(TARGET, Fn, ...)                                 \
  do {                                                                        \
    const char * val;                                                         \
    val = ak_xml_rawcval(xst);\
    if (val)                                                                  \
      TARGET = Fn(val, __VA_ARGS__);                                          \
  } while (0)

#define _xml_readAttr(PARENT, TARGET, ATTR)                                   \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(xst->reader,             \
                                                (const xmlChar *)ATTR);       \
    if (attrVal) {                                                            \
      TARGET = ak_heap_strdup(xst->heap, PARENT, attrVal);               \
      xmlFree(attrVal);                                                       \
    } else TARGET = NULL;                                                     \
  } while (0);

#define _xml_readId(OBJ)                                                      \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(xst->reader,             \
                                                (const xmlChar *)_s_dae_id);  \
    if (attrVal) {                                                            \
      ak_setId(OBJ, ak_heap_strdup(xst->heap, OBJ, attrVal));                \
      xmlFree(attrVal);                                                       \
    }                                                                         \
  } while (0);

#define _xml_readAttrUsingFn(TARGET, ATTR, Fn, ...)                           \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(xst->reader,             \
                                                (const xmlChar *)ATTR);       \
    if (attrVal) {                                                            \
      TARGET = Fn(attrVal, __VA_ARGS__);                                      \
      xmlFree(attrVal);                                                       \
    }                                                                         \
  } while (0);

#define _xml_readAttrUsingFnWithDef(TARGET, ATTR, DEF, Fn, ...)               \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(xst->reader,             \
                                                (const xmlChar *)ATTR);       \
    if (attrVal) {                                                            \
      TARGET = Fn(attrVal, __VA_ARGS__);                                      \
      xmlFree(attrVal);                                                       \
    } else TARGET = DEF;                                                      \
  } while (0);

#define _xml_readTextAsEnum(D, X, FN)                                         \
  do {                                                                        \
    char *attrValStr;                                                         \
    attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,          \
                                                   (const xmlChar *)X);       \
    if (attrValStr) {                                                         \
      AkEnum attrVal;                                                         \
      attrVal = FN(attrValStr);                                               \
      D = attrVal != -1 ? attrVal: 0;                                         \
      xmlFree(attrValStr);                                                    \
    } else D = 0;                                                             \
  } while(0);

#define _xml_readAttrAsEnum(TARGET, ATTR, FN)                                 \
  do {                                                                        \
    char *attrValStr;                                                         \
    attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,          \
                                                   (const xmlChar *)ATTR);    \
    if (attrValStr) {                                                         \
      AkEnum attrVal;                                                         \
      attrVal = FN(attrValStr);                                               \
      TARGET = attrVal != -1 ? attrVal: 0;                                    \
      xmlFree(attrValStr);                                                    \
    } else TARGET = 0;                                                        \
  } while(0);

#define _xml_readAttrAsEnumWithDef(TARGET, ATTR, FN, DEF)                     \
  do {                                                                        \
    char *attrValStr;                                                         \
    attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,          \
                                                   (const xmlChar *)ATTR);    \
    if (attrValStr) {                                                         \
      AkEnum attrVal;                                                         \
      attrVal = FN(attrValStr);                                               \
      TARGET = attrVal != -1 ? attrVal: DEF;                                  \
      xmlFree(attrValStr);                                                    \
    } else TARGET = DEF;                                                      \
  } while(0);

#include "../include/ak-url.h"

void
ak_url_from_attr(xmlTextReaderPtr reader,
                 const char * attrName,
                 void  *memparent,
                 AkURL *url);

#endif /* __libassetkit__libxml__h_ */
