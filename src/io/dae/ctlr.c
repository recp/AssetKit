/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common.h"

AK_EXPORT
AkGeometry*
ak_baseGeometry(AkURL * __restrict baseurl) {
  void         *found;
  AkController *ctlr;
  AkSkinDAE    *skindae;
  AkMorphDAE   *morphdae;
  AkTypeId      foundType;

  if ((found = ak_getObjectByUrl(baseurl))) {
    foundType = ak_typeid(found);
    switch (foundType) {
      case AKT_GEOMETRY:
        return found;
      case AKT_CONTROLLER: {
        /* `ctlr->data` is the cross-format AkSkin/AkMorph (no baseGeom).
           DAE-specific URL lives on the AkSkinDAE/AkMorphDAE attached as
           userData by the parser. */
        ctlr = found;
        if (ctlr->type == AK_CONTROLLER_SKIN
            && (skindae = ak_userData(ctlr->data))) {
          return ak_baseGeometry(&skindae->baseGeom);
        } else if (ctlr->type == AK_CONTROLLER_MORPH
                   && (morphdae = ak_userData(ctlr->data))) {
          return ak_baseGeometry(&morphdae->baseGeom);
        }
      }
      default: break;
    }
  }

  return NULL;
}
