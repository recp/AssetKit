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
  char * attrVal;
  attrVal = (char *)xmlTextReaderGetAttribute(reader,
                                              (const xmlChar *)attrName);
  if (attrVal) {
    ak_url_init(memparent,
                attrVal,
                url);
    xmlFree(attrVal);
  } else {
    url->reserved = NULL;
    url->url      = NULL;
  }
}
