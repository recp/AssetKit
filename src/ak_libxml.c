/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_libxml.h"
#include "ak_common.h"
#include "collada/ak_collada_common.h"

void
ak_url_from_attr(xmlTextReaderPtr reader,
                 const char * attrName,
                 void  *memparent,
                 AkURL *url) {
  xmlChar * attrVal;
  attrVal = xmlTextReaderGetAttribute(reader,
                                      (const xmlChar *)attrName);
  if (attrVal) {
    ak_url_init(memparent,
                (char *)attrVal,
                url);
    xmlFree(attrVal);
  } else {
    url->reserved = NULL;
    url->url      = NULL;
  }
}

void
ak_xml_readnext(AkXmlState * __restrict xst) {
  xst->nodeRet  = xmlTextReaderRead(xst->reader);
  xst->nodeType = xmlTextReaderNodeType(xst->reader);
  xst->nodeName = xmlTextReaderConstName(xst->reader);
}

bool
ak_xml_beginelm(AkXmlState * __restrict xst,
                const char *elmname) {
  xst->nodeName = xmlTextReaderConstName(xst->reader);
  if (_xml_eq(xst->nodeName, elmname)
      && xmlTextReaderIsEmptyElement(xst->reader))
    return true;

  ak_xml_readnext(xst);

  return (_xml_eq(xst->nodeName, elmname)
          && xst->nodeType == XML_ELEMENT_DECL)
         || xst->nodeType == XML_DTD_NODE /* Whitespace: 14 */
  ;
}

void
ak_xml_endelm(AkXmlState * __restrict xst) {
  do {
    if ((xst->nodeType == XML_ELEMENT_DECL
         && xst->nodeType != XML_TEXT_NODE)
        || xst->nodeType == XML_ELEMENT_NODE)
      break;

    ak_xml_readnext(xst);
  } while(1);
}

void
ak_xml_skipelm(AkXmlState * __restrict xst) {
  const xmlChar * nodeName;
  int nodeDepth;

  nodeName  = xst->nodeName;
  nodeDepth = xmlTextReaderDepth(xst->reader);

  while (!_xml_eqDecl(nodeName)) {
    ak_xml_readnext(xst);

    if (xmlTextReaderDepth(xst->reader) <= nodeDepth)
      break;
  }
}

char *
ak_xml_rawcval(AkXmlState * __restrict xst) {
  /* read text element*/
  ak_xml_readnext(xst);

  if (xst->nodeType != XML_TEXT_NODE)
    return NULL;

  return (char *)xmlTextReaderConstValue(xst->reader);
}

char *
ak_xml_rawval(AkXmlState * __restrict xst) {
  /* read text element*/
  ak_xml_readnext(xst);

  if (xst->nodeType != XML_TEXT_NODE)
    return NULL;

  return (char *)xmlTextReaderValue(xst->reader);
}

char *
ak_xml_val(AkXmlState * __restrict xst,
           void * __restrict parent) {
  return ak_heap_strdup(xst->heap,
                        parent,
                        ak_xml_rawcval(xst));
}
