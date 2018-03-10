/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_asset.h"
#include "../../ak_memory_common.h"

AkResult _assetkit_hide
ak_dae_assetInf(AkXmlState * __restrict xst,
                void       * __restrict memParent,
                AkAssetInf * __restrict ainf) {
  AkAssetInf   **extp;
  AkXmlElmState xest;

  if (!ainf) {
    ainf = ak_heap_calloc(xst->heap,
                          memParent,
                          sizeof(*ainf));
  }

  ak_xest_init(xest, _s_dae_asset)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_contributor)) {
      AkContributor *contrib;
      AkXmlElmState  xest2;

      contrib = ak_heap_calloc(xst->heap, ainf, sizeof(*contrib));

      ak_xest_init(xest2, _s_dae_contributor)

      /* contributor */
      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_author))
          contrib->author = ak_xml_val(xst, ainf);
        else if (ak_xml_eqelm(xst, _s_dae_author_email))
          contrib->authorEmail = ak_xml_val(xst, ainf);
        else if (ak_xml_eqelm(xst, _s_dae_author_website))
          contrib->authorWebsite = ak_xml_val(xst, ainf);
        else if (ak_xml_eqelm(xst, _s_dae_authoring_tool))
          contrib->authoringTool = ak_xml_val(xst, ainf);
        else if (ak_xml_eqelm(xst, _s_dae_comments))
          contrib->comments = ak_xml_val(xst, ainf);
        else if (ak_xml_eqelm(xst, _s_dae_copyright))
          contrib->copyright = ak_xml_val(xst, ainf);
        else if (ak_xml_eqelm(xst, _s_dae_source_data))
          contrib->sourceData = ak_xml_val(xst, ainf);
        else
          ak_xml_skipelm(xst);

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      ainf->contributor = contrib;
    } else if (ak_xml_eqelm(xst, _s_dae_created)) {
      const char * val;
      val = ak_xml_rawcval(xst);
      ainf->created = ak_parse_date(val, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_modified)) {
      const char * val;
      val = ak_xml_rawcval(xst);
      ainf->modified = ak_parse_date(val, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_keywords)) {
      ainf->keywords = ak_xml_val(xst, ainf);
    } else if (ak_xml_eqelm(xst, _s_dae_revision)) {
      ainf->revision = ak_xml_val(xst, ainf);
    } else if (ak_xml_eqelm(xst, _s_dae_subject)) {
      ainf->subject = ak_xml_val(xst, ainf);
    } else if (ak_xml_eqelm(xst, _s_dae_title)) {
      ainf->title = ak_xml_val(xst, ainf);
    } else if (ak_xml_eqelm(xst, _s_dae_unit)) {
      AkUnit * unit;
      unit = ak_heap_calloc(xst->heap, ainf, sizeof(*unit));

      unit->name = ak_xml_attr(xst, ainf, _s_dae_name);
      unit->dist = ak_xml_attrd(xst, _s_dae_meter);

      ainf->unit = unit;
    } else if (ak_xml_eqelm(xst, _s_dae_up_axis)) {
      const char *val;

      val = ak_xml_rawcval(xst);
      if (val) {
        if (strcasecmp(val, _s_dae_z_up) == 0)
          ainf->coordSys = AK_ZUP;
        else if (strcasecmp(val, _s_dae_x_up) == 0)
          ainf->coordSys = AK_XUP;
        else
          ainf->coordSys = AK_YUP;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree  * tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          ainf,
                          nodePtr,
                          &tree,
                          NULL);
      ainf->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }
    
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  extp = ak_heap_ext_add(xst->heap,
                         ak__alignof(memParent),
                         AK_HEAP_NODE_FLAGS_INF);
  *extp = ainf;

  return AK_OK;
}
