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

bool
ak_xml_eqelm(AkXmlState * __restrict xst,
             const char * s);

bool
ak_xml_eqdecl(AkXmlState * __restrict xst,
              const xmlChar * s);

const char *
ak_xml_attr(AkXmlState * __restrict xst,
            void * __restrict parent,
            const char * name);

void
ak_xml_readid(AkXmlState * __restrict xst,
              void * __restrict memptr);

float
ak_xml_valf(AkXmlState * __restrict xst);

double
ak_xml_vald(AkXmlState * __restrict xst);

long
ak_xml_vall(AkXmlState * __restrict xst);

unsigned long
ak_xml_valul(AkXmlState * __restrict xst);

unsigned long
ak_xml_valul_def(AkXmlState * __restrict xst,
                 unsigned long defval);

AkEnum
ak_xml_readenum(AkXmlState * __restrict xst,
                AkEnum (*fn)(const char * name));

AkEnum
ak_xml_readenum_from(const char *text,
                     AkEnum (*fn)(const char * name));

AkEnum
ak_xml_attrenum(AkXmlState * __restrict xst,
                const char * name,
                AkEnum (*fn)(const char * name));

AkEnum
ak_xml_attrenum_def(AkXmlState * __restrict xst,
                    const char * name,
                    AkEnum (*fn)(const char * name),
                    AkEnum defval);

#define _xml_readAttrUsingFn(TARGET, ATTR, Fn, ...)                           \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(xst->reader,                  \
                                                (const xmlChar *)ATTR);       \
    if (attrVal) {                                                            \
      TARGET = Fn(attrVal, __VA_ARGS__);                                      \
      xmlFree(attrVal);                                                       \
    }                                                                         \
  } while (0);

#define _xml_readAttrUsingFnWithDef(TARGET, ATTR, DEF, Fn, ...)               \
  do {                                                                        \
    char * attrVal;                                                           \
    attrVal = (char *)xmlTextReaderGetAttribute(xst->reader,                  \
                                                (const xmlChar *)ATTR);       \
    if (attrVal) {                                                            \
      TARGET = Fn(attrVal, __VA_ARGS__);                                      \
      xmlFree(attrVal);                                                       \
    } else TARGET = DEF;                                                      \
  } while (0);

#include "../include/ak-url.h"

void
ak_url_from_attr(xmlTextReaderPtr reader,
                 const char * attrName,
                 void  *memparent,
                 AkURL *url);

#endif /* __libassetkit__libxml__h_ */
