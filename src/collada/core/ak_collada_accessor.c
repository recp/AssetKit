/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_accessor.h"
#include "ak_collada_param.h"

AkResult _assetkit_hide
ak_dae_accessor(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkAccessor ** __restrict dest) {
  AkAccessor  *accessor;
  AkDataParam *last_param;

  accessor = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*accessor));

  accessor->count  = ak_xml_attrui(xst, _s_dae_count);
  accessor->offset = ak_xml_attrui(xst, _s_dae_offset);
  accessor->stride = ak_xml_attrui(xst, _s_dae_stride);

  ak_xml_attr_url(xst->reader,
                  _s_dae_source,
                  accessor,
                  &accessor->source);

  if (xmlTextReaderIsEmptyElement(xst->reader))
    goto done;

  last_param = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_accessor))
      break;

    if (ak_xml_eqelm(xst, _s_dae_param)) {
      AkDataParam *dataParam;
      AkResult     ret;

      ret = ak_dae_dataparam(xst, accessor, &dataParam);
      if (ret == AK_OK) {
        if (last_param)
          last_param->next = dataParam;
        else
          accessor->param = dataParam;

        last_param = dataParam;
      }
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

done:
  *dest = accessor;
  return AK_OK;
}
