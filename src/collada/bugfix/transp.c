/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "transp.h"
#include <string.h>
#include <ctype.h>

void _assetkit_hide
dae_bugfix_transp(AkTransparent * __restrict transp) {
  AkContributor *contr;
  const char    *tool;

  if (!(contr = ak_getAssetInfo(transp, offsetof(AkAssetInf, contributor)))
      || !(tool = contr->authoringTool))
    return;

  /* fix old SketchUp transparency bug */
  if (strcasestr(tool, _s_dae_sketchup)) {
    int major, minor, patch;
    if(sscanf(tool, "%*[^0123456789]%d%*[. ]%d%*[. ]%d",
              &major, &minor, &patch)) {

      /* don't flip >= 7.1.1 */
      if (major <= 7 && minor < 2 && patch < 1) {
        if (transp->amount->val) {
          *transp->amount->val = 1.0f - *transp->amount->val;
        }
      }
    }
  } /* _s_dae_sketchup */
}
