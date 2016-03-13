/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__libxml__h_
#define __libassetio__libxml__h_

#include <stdbool.h>
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
  (nodeType == XML_ELEMENT_NODE                                               \
   && xmlStrcasecmp(nodeName, (xmlChar *)b) == 0)

#define _xml_eqDecl(b)                                                        \
  (nodeType == XML_ELEMENT_DECL                                               \
   && xmlStrcasecmp(nodeName, (xmlChar *)b) == 0)

#define _xml_readNext                                                         \
  do {                                                                        \
    nodeRet  = xmlTextReaderRead(reader);                                     \
    nodeType = xmlTextReaderNodeType(reader);                                 \
    nodeName = xmlTextReaderConstName(reader);                                \
  } while (0)

#define _xml_beginElement(x)                                                  \
  _xml_readNext;                                                              \
  if (nodeType == XML_ELEMENT_DECL                                            \
      && _xml_eq(nodeName, x))                                                \
    break;                                                                    \

#define _xml_endElement                                                       \
  do {                                                                        \
    if ((nodeType == XML_ELEMENT_DECL                                         \
        && nodeType != XML_TEXT_NODE)                                         \
        || nodeType == XML_ELEMENT_NODE)                                      \
      break;                                                                  \
    _xml_readNext;                                                            \
  } while(1)

#define _xml_skipElement                                                      \
  do {                                                                        \
     const xmlChar * _nodeName;                                               \
     int _nodeDepth;                                                          \
     _nodeName = nodeName;                                                    \
     _nodeDepth = xmlTextReaderDepth(reader);                                 \
                                                                              \
     while (!_xml_eqDecl(_nodeName)) {                                        \
       _xml_readNext;                                                         \
       if (xmlTextReaderDepth(reader) <= _nodeDepth)                          \
         break;                                                               \
     }                                                                        \
  } while (0)

#define _xml_readText(PARENT, TARGET)                                         \
  do {                                                                        \
    /* read text element*/                                                    \
    _xml_readNext;                                                            \
    TARGET = nodeType == XML_TEXT_NODE ?                                      \
         aio_strdup(PARENT,                                                   \
                    (const char *)xmlTextReaderConstValue(reader)) : NULL;    \
  } while (0)

#define _xml_readMutText(x)                                                   \
  do {                                                                        \
    /* read text element*/                                                    \
    _xml_readNext;                                                            \
    x = nodeType == XML_TEXT_NODE ?                                           \
         (char *)xmlTextReaderValue(reader) : NULL;                           \
  } while (0)

#define _xml_readConstText(x)                                                 \
  do {                                                                        \
    /* read text element*/                                                    \
    _xml_readNext;                                                            \
    x = nodeType == XML_TEXT_NODE ?                                           \
        (const char *)xmlTextReaderConstValue(reader) : NULL;                 \
  } while (0)


#define _xml_readTextUsingFn(X, Fn, ...)                                      \
  do {                                                                        \
    const char * val;                                                         \
    /* read text element*/                                                    \
    _xml_readNext;                                                            \
    val = nodeType == XML_TEXT_NODE ?                                         \
        (const char *)xmlTextReaderConstValue(reader) : NULL;                 \
    if (val)                                                                  \
      X = Fn(val, __VA_ARGS__);                                               \
  } while (0)

#define _xml_readAttr(PARENT, TARGET, ATTR)                                   \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(reader,                       \
                                                (const xmlChar *)ATTR);       \
    if (attrVal) {                                                            \
      TARGET = aio_strdup(PARENT, attrVal);                                   \
      xmlFree(attrVal);                                                       \
    } else TARGET = NULL;                                                     \
  } while (0);

#define _xml_readAttrUsingFn(D, X, Fn, ...)                                   \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(reader, (const xmlChar *)X);  \
    if (attrVal) {                                                            \
      D = Fn(attrVal, __VA_ARGS__);                                           \
      xmlFree(attrVal);                                                       \
    }                                                                         \
  } while (0);

#define _xml_readTextAsEnum(D, X, FN)                                         \
  do {                                                                        \
    char *attrValStr;                                                         \
    attrValStr = (char *)xmlTextReaderGetAttribute(reader,                    \
                                                   (const xmlChar *)X);       \
    if (attrValStr) {                                                         \
      long attrVal;                                                           \
      attrVal = FN(attrValStr);                                               \
      D = attrVal != -1 ? attrVal: 0;                                         \
      xmlFree(attrValStr);                                                    \
    } else D = 0;                                                             \
  } while(0);

#define _xml_readAttrAsEnum(D, X, FN)                                         \
  do {                                                                        \
    char *attrValStr;                                                         \
    attrValStr = (char *)xmlTextReaderGetAttribute(reader,                    \
                                                   (const xmlChar *)X);       \
    if (attrValStr) {                                                         \
      long attrVal;                                                           \
      attrVal = FN(attrValStr);                                               \
      D = attrVal != -1 ? attrVal: 0;                                         \
      xmlFree(attrValStr);                                                    \
    } else D = 0;                                                             \
  } while(0);

#define _xml_readAttrAsEnumWithDef(D, X, FN, DEF)                             \
  do {                                                                        \
    char *attrValStr;                                                         \
    attrValStr = (char *)xmlTextReaderGetAttribute(reader,                    \
                                                   (const xmlChar *)X);       \
    if (attrValStr) {                                                         \
      long attrVal;                                                           \
      attrVal = FN(attrValStr);                                               \
      D = attrVal != -1 ? attrVal: DEF;                                       \
      xmlFree(attrValStr);                                                    \
    } else D = DEF;                                                           \
  } while(0);

#endif /* __libassetio__libxml__h_ */
