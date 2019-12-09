/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "xml.h"
#include "common.h"
#include "collada/dae_common.h"

void
ak_xml_url_add(AkXmlState * __restrict xst,
               AkURL      * url) {
  AkURLQueue *urlQueue;

  urlQueue       = xst->heap->allocator->malloc(sizeof(*urlQueue));
  urlQueue->next = xst->urlQueue;
  urlQueue->url  = url;
  xst->urlQueue  = urlQueue;
}

void
ak_xml_attr_url(AkXmlState * __restrict xst,
                const char * attrName,
                void       * memparent,
                AkURL      * url) {
  xmlChar    *attrVal;

  attrVal = xmlTextReaderGetAttribute(xst->reader, (const xmlChar *)attrName);
  if (attrVal) {
    ak_url_init(memparent, (char *)attrVal,  url);
    ak_xml_url_add(xst, url);
    xmlFree(attrVal);
    return;
  }

  url->reserved = NULL;
  url->url      = NULL;
}

AkURL*
ak_xmlAttrGetURL(AkXmlState * __restrict xst,
                 const char * __restrict attrName,
                 void       * __restrict memparent) {
  AkURL   *url;
  xmlChar *attrVal;

  attrVal = xmlTextReaderGetAttribute(xst->reader, (const xmlChar *)attrName);
  if (attrVal) {
    url = ak_heap_calloc(ak_heap_getheap(memparent), memparent, sizeof(*url));
    ak_url_init(url, (char *)attrVal, url);
    xmlFree(attrVal);
    return url;
  }
  
  return NULL;
}

void
ak_xml_readnext(AkXmlState * __restrict xst) {
again:
  xst->nodeRet   = xmlTextReaderRead(xst->reader);
  xst->nodeType  = xmlTextReaderNodeType(xst->reader);
  xst->nodeName  = xmlTextReaderConstName(xst->reader);
  xst->nodeDepth = xmlTextReaderDepth(xst->reader);

  if (xst->nodeType == XML_COMMENT_NODE)
    goto again;
}

bool
ak_xml_eqelm(AkXmlState * __restrict xst, const char * s) {
  return xst->nodeType == XML_ELEMENT_NODE
         && xmlStrcasecmp(xst->nodeName, (xmlChar *)s) == 0;
}

bool
ak_xml_eqdecl(AkXmlState * __restrict xst, const xmlChar * s) {
  return xst->nodeType == XML_ELEMENT_DECL
        && xmlStrcasecmp(xst->nodeName, s) == 0;
}

bool
ak_xml_begin(AkXmlElmState * __restrict xest) {
  if (_xml_eq(xest->xst->nodeName, xest->name)
      && xmlTextReaderIsEmptyElement(xest->xst->reader))
    return true;

  ak_xml_readnext(xest->xst);

  return (_xml_eq(xest->xst->nodeName, xest->name)
          && xest->xst->nodeType == XML_ELEMENT_DECL)
          || xest->xst->nodeType == XML_DTD_NODE /* Whitespace: 14 */
  ;
}

bool
ak_xml_end(AkXmlElmState * __restrict xest) {
  if (!xmlTextReaderIsEmptyElement(xest->xst->reader)) {
    do {
      if (xest->xst->nodeType == XML_ELEMENT_DECL
          || xest->xst->nodeType == XML_ELEMENT_NODE
          )
        break;

      ak_xml_readnext(xest->xst);
    } while(1);
  }

  /* TODO: there is no need to compare nodeType and nodeName?
           because we reached to end of node by depth info
   */
  return xest->depth == xest->xst->nodeDepth;
}

void
ak_xml_skipelm(AkXmlState * __restrict xst) {
  const xmlChar * nodeName;
  int nodeDepth;

  nodeName  = xst->nodeName;
  nodeDepth = xmlTextReaderDepth(xst->reader);

  while (!ak_xml_eqdecl(xst, nodeName)) {
    ak_xml_readnext(xst);

    if (xmlTextReaderDepth(xst->reader) <= nodeDepth)
      break;
  }
}

char *
ak_xml_rawcval(AkXmlState * __restrict xst) {
  if (xmlTextReaderIsEmptyElement(xst->reader))
    return NULL;

  /* read text element*/
  ak_xml_readnext(xst);

  if (xst->nodeType != XML_TEXT_NODE)
    return NULL;

  return (char *)xmlTextReaderConstValue(xst->reader);
}

char *
ak_xml_rawval(AkXmlState * __restrict xst) {
 if (xmlTextReaderIsEmptyElement(xst->reader))
   return NULL;

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

float
ak_xml_valf(AkXmlState * __restrict xst) {
  const char * val;
  val = ak_xml_rawcval(xst);

  if (val)
    return strtof(val, NULL);

  return 0.0f;
}

double
ak_xml_vald(AkXmlState * __restrict xst) {
  const char * val;
  val = ak_xml_rawcval(xst);

  if (val)
    return strtod(val, NULL);

  return 0.0;
}

long
ak_xml_vall(AkXmlState * __restrict xst) {
  const char * val;
  val = ak_xml_rawcval(xst);

  if (val)
    return strtol(val, NULL, 10);

  return 0l;
}

unsigned long
ak_xml_valul(AkXmlState * __restrict xst) {
  const char * val;
  val = ak_xml_rawcval(xst);

  if (val)
    return strtoul(val, NULL, 10);

  return 0ul;
}

unsigned long
ak_xml_valul_def(AkXmlState * __restrict xst,
                 unsigned long defval) {
  const char * val;
  char       * tmp;

  tmp = NULL;
  val = ak_xml_rawcval(xst);

  if (val) {
    unsigned long vall;
    vall = strtoul(val, &tmp, 10);

    if (tmp && *tmp == '\0')
      return defval;

    return vall;
  }

  return 0ul;
}

const char *
ak_xml_attr(AkXmlState * __restrict xst,
            void * __restrict parent,
            const char * name) {
  xmlChar *xmlAttrVal;
  char    *attrVal;

  xmlAttrVal = xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)name);
  if (xmlAttrVal) {
    attrVal = ak_heap_strdup(xst->heap,
                             parent,
                             (char *)xmlAttrVal);
    xmlFree(xmlAttrVal);
    return attrVal;
  }

  return NULL;
}

AkEnum
ak_xml_readenum(AkXmlState * __restrict xst,
                AkEnum (*fn)(const char * name)) {
  const char *text;

  text = ak_xml_rawcval(xst);

  if (text) {
    AkEnum enm;
    enm = fn(text);
    return enm != -1 ? enm: 0;
  }

  return 0;
}

AkEnum
ak_xml_readenum_from(const char *text,
                     AkEnum (*fn)(const char * name)) {
  AkEnum enm;

  if (!text)
    return 0;

  enm = fn(text);
  return (enm != -1) * enm;
}

AkEnum
ak_xml_attrenum(AkXmlState * __restrict xst,
                const char * name,
                AkEnum (*fn)(const char * name)) {
  xmlChar *xmlAttrVal;

  xmlAttrVal = xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)name);
  if (xmlAttrVal) {
    AkEnum enm;
    enm = fn((char *)xmlAttrVal);
    xmlFree(xmlAttrVal);

    return (enm != -1) * enm;
  }

  return 0;
}

AkEnum
ak_xml_attrenum_def(AkXmlState * __restrict xst,
                    const char * name,
                    AkEnum (*fn)(const char * name),
                    AkEnum defval) {
  AkEnum enm;

  enm = ak_xml_attrenum(xst, name, fn);

  return enm > 0 ? enm : defval;
}

float
ak_xml_attrf(AkXmlState * __restrict xst,
             const char * name) {
  xmlChar *xmlAttrVal;
  float    attr;

  xmlAttrVal = xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)name);
  if (xmlAttrVal) {
    attr = strtof((char *)xmlAttrVal, NULL);
    xmlFree(xmlAttrVal);
    return attr;
  }

  return 0.0f;
}

double
ak_xml_attrd(AkXmlState * __restrict xst,
             const char * name) {
  xmlChar *xmlAttrVal;
  double   attr;

  xmlAttrVal = xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)name);
  if (xmlAttrVal) {
    attr = strtod((char *)xmlAttrVal, NULL);
    xmlFree(xmlAttrVal);
    return attr;
  }

  return 0.0;
}

unsigned int
ak_xml_attrui(AkXmlState * __restrict xst,
              const char * name) {
  xmlChar     *xmlAttrVal;
  unsigned int attr;

  xmlAttrVal = xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)name);
  if (xmlAttrVal) {
    attr = (unsigned int)strtoul((char *)xmlAttrVal, NULL, 10);
    xmlFree(xmlAttrVal);
    return attr;
  }

  return 0ul;
}

uint64_t
ak_xml_attrui64(AkXmlState * __restrict xst,
                const char * name) {
  xmlChar *xmlAttrVal;
  uint64_t attr;

  xmlAttrVal = xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)name);
  if (xmlAttrVal) {
    attr = strtoull((char *)xmlAttrVal, NULL, 10);
    xmlFree(xmlAttrVal);
    return attr;
  }

  return 0ull;
}

unsigned int
ak_xml_attrui_def(AkXmlState * __restrict xst,
                  const char * name,
                  unsigned int defval) {
  xmlChar     *xmlAttrVal;
  char        *tmp;
  unsigned int attr;

  tmp        = NULL;
  xmlAttrVal = xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)name);
  if (xmlAttrVal) {
    attr = (unsigned int)strtoul((char *)xmlAttrVal, &tmp, 10);
    xmlFree(xmlAttrVal);
    return attr;
  }

  return defval;
}

void
ak_xml_readid(AkXmlState * __restrict xst,
              void * __restrict memptr) {
  xmlChar *xmlAttrVal;

  xmlAttrVal = xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)_s_dae_id);
  if (xmlAttrVal) {
    ak_setId(memptr,
             ak_heap_strdup(xst->heap,
                            memptr,
                            (char *)xmlAttrVal));
    xmlFree(xmlAttrVal);
  }
}

void
ak_xml_readsid(AkXmlState * __restrict xst,
               void * __restrict memptr) {
  xmlChar *xmlAttrVal;

  xmlAttrVal = xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)_s_dae_sid);
  if (xmlAttrVal) {
    ak_sid_set(memptr, ak_heap_strdup(xst->heap,
                                      memptr,
                                      (char *)xmlAttrVal));

    xmlFree(xmlAttrVal);
  }
}

void
ak_xml_sid_seta(AkXmlState * __restrict xst,
                void *memnode,
                void *memptr) {
  const char *sid;

  sid = ak_xml_attr(xst, memnode, _s_dae_sid);
  if (!sid)
    return;

  ak_sid_seta(memnode,
              memptr,
              sid);
}
