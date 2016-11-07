/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../../include/ak-path.h"
#include "ak_resource.h"

#include <string.h>
#include <stdlib.h>

void
ak_url_init(const char *urlstring,
            AkURL *dest) {
  dest->reserved = ak_resc_ins(urlstring);
  dest->url      = ak_path_fragment(urlstring);;
}

void
ak_url_ref(AkURL *url) {
  if (!url->reserved)
    return;

  ak_resc_ref(url->reserved);
}

void
ak_url_unref(AkURL *url) {
  if (!url->reserved)
    return;

  if (ak_resc_unref(url->reserved) < 0) {
    url->reserved = NULL;
    url->url      = NULL;
  }
}
