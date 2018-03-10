/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae14_surface.h"
#include "../core/dae_asset.h"
#include "../fx/dae_fx_enums.h"
#include <string.h>

AkResult _assetkit_hide
ak_dae14_fxSurface(AkXmlState      * __restrict xst,
                   void            * __restrict memParent,
                   AkDae14Surface ** __restrict dest) {
  AkDae14Surface *surface;
  AkXmlElmState   xest;

  surface = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*surface));

  ak_xest_init(xest, _s_dae_surface)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_init_from)) {
      AkDae14SurfaceFrom *initFrom;
      initFrom = ak_heap_calloc(xst->heap,
                                surface,
                                sizeof(*xst->heap));
      initFrom->mip   = ak_xml_attrui(xst, _s_dae_mip);
      initFrom->slice = ak_xml_attrui(xst, _s_dae_slice);
      initFrom->image = ak_xml_val(xst, surface);
      initFrom->face  = ak_xml_attrenum_def(xst,
                                            _s_dae_face,
                                            ak_dae_fxEnumFace,
                                            AK_FACE_POSITIVE_Y);
      surface->initFrom = initFrom;
    } else if (ak_xml_eqelm(xst, _s_dae_init_as_target)) {
      surface->initAsTarget = true; /* becuse the element exists */
    } else if (ak_xml_eqelm(xst, _s_dae_format)) {
      surface->format = ak_xml_val(xst, surface);
    } else if (ak_xml_eqelm(xst, _s_dae_format_hint)) {
      AkImageFormat *format;
      AkXmlElmState  xest2;

      format = ak_heap_calloc(xst->heap,
                              memParent,
                              sizeof(*format));

      ak_xest_init(xest2, _s_dae_format_hint)

      do {
        char *valStr;
        if (ak_xml_begin(&xest2))
          break;

        if (ak_xml_eqelm(xst, _s_dae_channels)) {
          valStr = ak_xml_rawcval(xst);
          if (valStr) {
            AkEnum attrVal;
            attrVal = ak_dae_fxEnumChannel(valStr);
            if (attrVal != -1)
              format->channel = attrVal;
          }
        } else if (ak_xml_eqelm(xst, _s_dae_range)) {
          valStr = ak_xml_rawcval(xst);
          if (valStr) {
            AkEnum attrVal;
            attrVal = ak_dae_fxEnumRange(valStr);
            if (attrVal != -1)
              format->range = attrVal;
          }
        } else if (ak_xml_eqelm(xst, _s_dae_precision)) {
          valStr = ak_xml_rawcval(xst);
          if (valStr) {
            AkEnum attrVal;
            attrVal = ak_dae_fxEnumPrecision(valStr);
            if (attrVal != -1)
              format->range = attrVal;
          }
        } else if (ak_xml_eqelm(xst, _s_dae_option)) {
          format->space = ak_xml_val(xst, format);
        } else if (ak_xml_eqelm(xst, _s_dae_exact)) {
          format->exact = ak_xml_val(xst, format);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        if (ak_xml_end(&xest2))
          break;
      } while (xst->nodeRet);
    } else if (ak_xml_eqelm(xst, _s_dae_size)) {
      char *content;
      content = ak_xml_rawcval(xst);
      if (content) {
        AkUInt size[3];
        ak_strtoui_fast(content, size, 3);

        surface->size.width  = size[0];
        surface->size.height = size[1];
        surface->size.depth  = size[2];
      }
    } else if (ak_xml_eqelm(xst, _s_dae_viewport_ratio)) {
      char *content;
      content = ak_xml_rawcval(xst);
      if (content)
        ak_strtof_fast(content, surface->viewportRatio, 2);
    } else if (ak_xml_eqelm(xst, _s_dae_mip_levels)) {
      char *content;
      content = ak_xml_rawcval(xst);
      if (content)
        surface->mipLevels = (int)strtol(content, NULL, 10);
    } else if (ak_xml_eqelm(xst, _s_dae_mipmap_generate)) {
      char *content;
      content = ak_xml_rawcval(xst);
      if (content)
        surface->mipmapGenerate = (bool)strtol(content, NULL, 10);
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree    *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          surface,
                          nodePtr,
                          &tree,
                          NULL);
      surface->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = surface;

  return AK_OK;
}
