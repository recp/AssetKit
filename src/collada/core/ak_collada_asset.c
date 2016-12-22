/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_asset.h"

AkResult _assetkit_hide
ak_dae_assetInf(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkAssetInf ** __restrict dest) {
  void         *memptr;
  AkXmlElmState xest;

  if (!(*dest)) {
    *dest = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(**dest));
    memptr = *dest;
  } else {
    memptr = memParent;
  }

  ak_xest_init(xest, _s_dae_asset)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_contributor)) {
      AkContributor *contrib;
      AkXmlElmState  xest2;

      contrib = ak_heap_calloc(xst->heap, memptr, sizeof(*contrib));

      ak_xest_init(xest2, _s_dae_contributor)

      /* contributor */
      do {
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_author))
          contrib->author = ak_xml_val(xst, memptr);
        else if (ak_xml_eqelm(xst, _s_dae_author_email))
          contrib->authorEmail = ak_xml_val(xst, memptr);
        else if (ak_xml_eqelm(xst, _s_dae_author_website))
          contrib->authorWebsite = ak_xml_val(xst, memptr);
        else if (ak_xml_eqelm(xst, _s_dae_authoring_tool))
          contrib->authoringTool = ak_xml_val(xst, memptr);
        else if (ak_xml_eqelm(xst, _s_dae_comments))
          contrib->comments = ak_xml_val(xst, memptr);
        else if (ak_xml_eqelm(xst, _s_dae_copyright))
          contrib->copyright = ak_xml_val(xst, memptr);
        else if (ak_xml_eqelm(xst, _s_dae_source_data))
          contrib->sourceData = ak_xml_val(xst, memptr);
        else
          ak_xml_skipelm(xst);

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);

      (*dest)->contributor = contrib;
    } else if (ak_xml_eqelm(xst, _s_dae_created)) {
      const char * val;
      val = ak_xml_rawcval(xst);
      (*dest)->created = ak_parse_date(val, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_modified)) {
      const char * val;
      val = ak_xml_rawcval(xst);
      (*dest)->modified = ak_parse_date(val, NULL);
    } else if (ak_xml_eqelm(xst, _s_dae_keywords)) {
      (*dest)->keywords = ak_xml_val(xst, memptr);
    } else if (ak_xml_eqelm(xst, _s_dae_revision)) {
      (*dest)->revision = ak_xml_vall(xst);
    } else if (ak_xml_eqelm(xst, _s_dae_subject)) {
      (*dest)->subject = ak_xml_val(xst, memptr);
    } else if (ak_xml_eqelm(xst, _s_dae_title)) {
      (*dest)->title = ak_xml_val(xst, memptr);
    } else if (ak_xml_eqelm(xst, _s_dae_unit)) {
      AkUnit * unit;
      unit = ak_heap_calloc(xst->heap, memptr, sizeof(*unit));

      unit->name = ak_xml_attr(xst, memptr, _s_dae_name);
      unit->dist = ak_xml_attrd(xst, _s_dae_meter);

      (*dest)->unit = unit;
    } else if (ak_xml_eqelm(xst, _s_dae_up_axis)) {
      const char *val;

      val = ak_xml_rawcval(xst);
      if (val) {
        if (strcasecmp(val, _s_dae_z_up) == 0)
          (*dest)->coordSys = AK_ZUP;
        else if (strcasecmp(val, _s_dae_x_up) == 0)
          (*dest)->coordSys = AK_XUP;
        else
          (*dest)->coordSys = AK_YUP;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree  * tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          memptr,
                          nodePtr,
                          &tree,
                          NULL);
      (*dest)->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }
    
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);
  
  return AK_OK;
}
