/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__libxml__h_
#define __libassetkit__libxml__h_

#include "ak_common.h"
#include "../include/ak-url.h"
#include <libxml/xmlreader.h>

typedef enum AkCOLLADAVersion {
  AK_COLLADA_VERSION_150 = 150,
  AK_COLLADA_VERSION_141 = 141,
  AK_COLLADA_VERSION_140 = 140
} AkCOLLADAVersion;

typedef struct AkURLQueue {
  AkURL *url;
  struct AkURLQueue *next;
} AkURLQueue;

typedef AK_ALIGN(16) struct AkXmlState {
  AkHeap          *heap;
  void            *jobs14;
  AkDoc           *doc;
  xmlTextReaderPtr reader;
  const xmlChar   *nodeName;
  AkURLQueue      *urlQueue;
  AkCOLLADAVersion version;
  int              nodeType;
  int              nodeRet;
  int              nodeDepth;
} AkXmlState;

typedef struct AkXmlElmState {
  AkXmlState * __restrict xst;
  const char * __restrict name;
  int                     depth;
} AkXmlElmState;

#define ak_xest_init(XEST, NAME)                                              \
  XEST.xst   = xst;                                                           \
  XEST.name  = NAME;                                                          \
  XEST.depth = xmlTextReaderDepth(xst->reader);

#define _xml_eq(a, b) (xmlStrcasecmp((xmlChar *)a, (xmlChar *)b) == 0)

void
ak_xml_readnext(AkXmlState * __restrict xst);

bool
ak_xml_begin(AkXmlElmState * __restrict xest);

bool
ak_xml_end(AkXmlElmState * __restrict);

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

float
ak_xml_attrf(AkXmlState * __restrict xst,
             const char * name);

double
ak_xml_attrd(AkXmlState * __restrict xst,
             const char * name);

int
ak_xml_attri(AkXmlState * __restrict xst,
             const char * name);

unsigned int
ak_xml_attrui(AkXmlState * __restrict xst,
              const char * name);

uint64_t
ak_xml_attrui64(AkXmlState * __restrict xst,
                const char * name);

unsigned int
ak_xml_attrui_def(AkXmlState * __restrict xst,
                  const char * name,
                  unsigned int defval);

void
ak_xml_attr_url(AkXmlState * __restrict xst,
                const char * attrName,
                void  *memparent,
                AkURL *url);

void
ak_xml_readid(AkXmlState * __restrict xst,
              void * __restrict memptr);

void
ak_xml_readsid(AkXmlState * __restrict xst,
               void * __restrict memptr);

void
ak_xml_sid_seta(AkXmlState * __restrict xst,
                void *memnode,
                void *memptr);

#endif /* __libassetkit__libxml__h_ */
