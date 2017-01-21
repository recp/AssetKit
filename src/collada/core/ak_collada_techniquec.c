/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_technique.h"
#include "ak_collada_color.h"
#include "../ak_collada_common.h"
#include "../fx/ak_collada_fx_material.h"
#include <cglm.h>

AkResult _assetkit_hide
ak_dae_techniquec(AkXmlState * __restrict xst,
                  void * __restrict memParent,
                  AkTechniqueCommon ** __restrict dest) {
  AkTechniqueCommon  *techc;
  AkInstanceMaterial *last_instanceMaterial;
  AkXmlElmState       xest;

  techc = ak_heap_calloc(xst->heap, memParent, sizeof(*techc));

  last_instanceMaterial = NULL;

  ak_xest_init(xest, _s_dae_techniquec)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_instance_material)) {
      AkInstanceMaterial *instanceMaterial;
      AkResult ret;
      ret = ak_dae_fxInstanceMaterial(xst,
                                      memParent,
                                      &instanceMaterial);

      if (ret == AK_OK) {
        if (last_instanceMaterial)
          last_instanceMaterial->next = instanceMaterial;
        else {
          techc->technique = instanceMaterial;
          techc->techniqueType = AK_TECHNIQUE_COMMON_INSTANCE_MATERIAL;
        }

        last_instanceMaterial = instanceMaterial;
      }
    }
    
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = techc;

  return AK_OK;
}
