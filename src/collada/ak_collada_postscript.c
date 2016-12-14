/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_postscript.h"
#include "ak_collada_geom_fixup.h"

void _assetkit_hide
ak_dae_postscript(AkDoc * doc) {
  /* fixup when finished,
     because we need to collect about source/array usages
     also we can run fixups as parallels here
  */
  if (!ak_opt_get(AK_OPT_INDICES_NONE))
    ak_dae_geom_fixup_all(doc);

  /* now set used coordSys */
  doc->coordSys = (void *)ak_opt_get(AK_OPT_COORD);
}
