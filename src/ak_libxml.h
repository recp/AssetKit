/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__libxml__h_
#define __libassetkit__libxml__h_

#include "../include/ak-memory.h"
#include <libxml/xmlreader.h>

#define _xml_eq(a, b) (xmlStrcasecmp((xmlChar *)a, (xmlChar *)b) == 0)

static
inline
bool
_xml_eqElm2(xmlTextReaderPtr reader,
            const char * __restrict b) {
  return xmlTextReaderNodeType(reader) == XML_ELEMENT_NODE
         && xmlStrcasecmp(xmlTextReaderConstName(reader), (xmlChar *)b) == 0;
}

static
inline
bool
_xml_eqDecl2(xmlTextReaderPtr reader,
             const char * __restrict b) {
  return xmlTextReaderNodeType(reader) == XML_ELEMENT_DECL
         && xmlStrcasecmp(xmlTextReaderConstName(reader), (xmlChar *)b) == 0;
}

/*
 * These <<convinient>> macros are not generic,
 * they are depends on ^^local/scope^^ variables. Be careful!
 *
 * These variables must be exists correctly:
 * nodeType
 * nodeName
 */

#define _xml_eqElm(b)                                                         \
  (daestate->nodeType == XML_ELEMENT_NODE                                               \
   && xmlStrcasecmp(daestate->nodeName, (xmlChar *)b) == 0)

#define _xml_eqDecl(b)                                                        \
  (daestate->nodeType == XML_ELEMENT_DECL                                               \
   && xmlStrcasecmp(daestate->nodeName, (xmlChar *)b) == 0)

#define _xml_readNext                                                         \
  do {                                                                        \
    daestate->nodeRet  = xmlTextReaderRead(daestate->reader);                           \
    daestate->nodeType = xmlTextReaderNodeType(daestate->reader);                       \
    daestate->nodeName = xmlTextReaderConstName(daestate->reader);                      \
  } while (0)

#define _xml_beginElement(x)                                                  \
  daestate->nodeName = xmlTextReaderConstName(daestate->reader);                        \
  if (_xml_eq(daestate->nodeName, x)                                                    \
      && xmlTextReaderIsEmptyElement(daestate->reader))                       \
    break;                                                                    \
  _xml_readNext;                                                              \
  if ((_xml_eq(daestate->nodeName, x) && daestate->nodeType == XML_ELEMENT_DECL)                  \
      || daestate->nodeType == XML_DTD_NODE /* Whitespace: 14 */                        \
      )                                                                       \
    break;                                                                    \

#define _xml_endElement                                                       \
  do {                                                                        \
    if ((daestate->nodeType == XML_ELEMENT_DECL                                         \
        && daestate->nodeType != XML_TEXT_NODE)                                         \
        || daestate->nodeType == XML_ELEMENT_NODE)                                      \
      break;                                                                  \
    _xml_readNext;                                                            \
  } while(1)

#define _xml_skipElement                                                      \
  do {                                                                        \
     const xmlChar * _nodeName;                                               \
     int _nodeDepth;                                                          \
     _nodeName = daestate->nodeName;                                                    \
     _nodeDepth = xmlTextReaderDepth(daestate->reader);                       \
                                                                              \
     while (!_xml_eqDecl(_nodeName)) {                                        \
       _xml_readNext;                                                         \
       if (xmlTextReaderDepth(daestate->reader) <= _nodeDepth)                \
         break;                                                               \
     }                                                                        \
  } while (0)

#define _xml_readText(PARENT, TARGET)                                         \
  do {                                                                        \
    /* read text element*/                                                    \
    _xml_readNext;                                                            \
    TARGET = daestate->nodeType == XML_TEXT_NODE ?                                      \
         ak_heap_strdup(daestate->heap,                                       \
                        PARENT,                                               \
              (const char *)xmlTextReaderConstValue(daestate->reader)):NULL;  \
  } while (0)

#define _xml_readMutText(x)                                                   \
  do {                                                                        \
    /* read text element*/                                                    \
    _xml_readNext;                                                            \
    x = daestate->nodeType == XML_TEXT_NODE ?                                           \
         (char *)xmlTextReaderValue(daestate->reader) : NULL;                 \
  } while (0)

#define _xml_readConstText(x)                                                 \
  do {                                                                        \
    /* read text element*/                                                    \
    _xml_readNext;                                                            \
    x = daestate->nodeType == XML_TEXT_NODE ?                                           \
        (const char *)xmlTextReaderConstValue(daestate->reader) : NULL;       \
  } while (0)


#define _xml_readTextUsingFn(TARGET, Fn, ...)                                 \
  do {                                                                        \
    const char * val;                                                         \
    /* read text element*/                                                    \
    _xml_readNext;                                                            \
    val = daestate->nodeType == XML_TEXT_NODE ?                                         \
        (const char *)xmlTextReaderConstValue(daestate->reader) : NULL;       \
    if (val)                                                                  \
      TARGET = Fn(val, __VA_ARGS__);                                          \
  } while (0)

#define _xml_readAttr(PARENT, TARGET, ATTR)                                   \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(daestate->reader,             \
                                                (const xmlChar *)ATTR);       \
    if (attrVal) {                                                            \
      TARGET = ak_heap_strdup(daestate->heap, PARENT, attrVal);               \
      xmlFree(attrVal);                                                       \
    } else TARGET = NULL;                                                     \
  } while (0);

#define _xml_readId(OBJ)                                                      \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(daestate->reader,             \
                                                (const xmlChar *)_s_dae_id);  \
    if (attrVal) {                                                            \
      ak_setId(OBJ, ak_heap_strdup(daestate->heap, OBJ, attrVal));                \
      xmlFree(attrVal);                                                       \
    }                                                                         \
  } while (0);

#define _xml_readAttrUsingFn(TARGET, ATTR, Fn, ...)                           \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(daestate->reader,             \
                                                (const xmlChar *)ATTR);       \
    if (attrVal) {                                                            \
      TARGET = Fn(attrVal, __VA_ARGS__);                                      \
      xmlFree(attrVal);                                                       \
    }                                                                         \
  } while (0);

#define _xml_readAttrUsingFnWithDef(TARGET, ATTR, DEF, Fn, ...)               \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(daestate->reader,             \
                                                (const xmlChar *)ATTR);       \
    if (attrVal) {                                                            \
      TARGET = Fn(attrVal, __VA_ARGS__);                                      \
      xmlFree(attrVal);                                                       \
    } else TARGET = DEF;                                                      \
  } while (0);

#define _xml_readTextAsEnum(D, X, FN)                                         \
  do {                                                                        \
    char *attrValStr;                                                         \
    attrValStr = (char *)xmlTextReaderGetAttribute(daestate->reader,          \
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
    attrValStr = (char *)xmlTextReaderGetAttribute(daestate->reader,          \
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
    attrValStr = (char *)xmlTextReaderGetAttribute(daestate->reader,          \
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
