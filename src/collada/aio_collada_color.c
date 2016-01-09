/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_color.h"

#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_memory.h"
#include "../aio_utils.h"
#include "../aio_tree.h"

#include "aio_collada_common.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_dae_color(xmlNode * __restrict xml_node,
              aio_color * __restrict dest) {

  char  * sid;
  char  * color_comp;
  float * color_vec;
  int     color_comp_idx;

  sid = NULL;
  aio_xml_collada_read_attr(xml_node, "sid", &sid);

  if (sid)
    dest->sid = sid;
  else
    dest->sid = NULL;

  color_vec = dest->vec;
  color_comp_idx = 0;

  color_comp = strtok(aio_xml_content(xml_node), " ");
  color_vec[color_comp_idx] = strtof(color_comp, NULL);

  while (color_comp && ++color_comp_idx < 4) {
    color_comp = strtok(NULL, " ");

    if (!color_comp) {
      --color_comp_idx;
      continue;
    }

    color_vec[color_comp_idx] = strtof(color_comp, NULL);
  }

  /* make alpha channel to 1.0 as default */
  if (color_comp_idx < 3)
    color_vec[3] = 1.0;

  return 0;
}
