/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_common__h_
#define __libassetio__aio_collada_common__h_

#include "aio_collada_strpool.h"
#include "aio_libxml.h"
#include "aio_types.h"
#include "aio_memory.h"
#include "aio_utils.h"
#include "aio_tree.h"

#include <libxml/xmlreader.h>
#include <string.h>

typedef struct _xmlNode xmlNode;

extern
int
aio_xml_collada_read_attr(xmlNode * __restrict xml_node,
                          const char * __restrict attr_name,
                          char ** __restrict val);

extern
int
aio_xml_collada_read_id_name(xmlNode * __restrict xml_node,
                             const char ** __restrict id,
                             const char ** __restrict name);

#endif /* __libassetio__aio_collada_common__h_ */
