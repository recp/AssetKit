/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_asset.h"
#include "aio_collada_common.h"

int _assetio_hide
aio_dae_assetInf(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 aio_assetinf ** __restrict dest) {
  const xmlChar * nodeName;
  int             nodeType;
  int             nodeRet;

  if (!(*dest))
    *dest = aio_calloc(memParent, sizeof(**dest), 1);

  do {
    _xml_beginElement(_s_dae_asset);

    if (_xml_eqElm(_s_dae_contributor)) {
      aio_contributor * contrib;
      contrib = aio_calloc(*dest, sizeof(*contrib), 1);

      /* contributor */
      do {
        _xml_beginElement(_s_dae_contributor);

        if (_xml_eqElm(_s_dae_author))
          _xml_readText(*dest, contrib->author);
        else if (_xml_eqElm(_s_dae_author_email))
          _xml_readText(*dest, contrib->author_email);
        else if (_xml_eqElm(_s_dae_author_website))
          _xml_readText(*dest, contrib->author_website);
        else if (_xml_eqElm(_s_dae_authoring_tool))
          _xml_readText(*dest, contrib->authoring_tool);
        else if (_xml_eqElm(_s_dae_comments))
          _xml_readText(*dest, contrib->comments);
        else if (_xml_eqElm(_s_dae_copyright))
          _xml_readText(*dest, contrib->copyright);
        else if (_xml_eqElm(_s_dae_source_data))
          _xml_readText(*dest, contrib->source_data);
        else
          _xml_skipElement;

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      (*dest)->contributor = contrib;
    } else if (_xml_eqElm(_s_dae_created)) {
      const char * val;
      _xml_readConstText(val);
      (*dest)->created = aio_parse_date(val, NULL);
    } else if (_xml_eqElm(_s_dae_modified)) {
      const char * val;
      _xml_readConstText(val);
      (*dest)->modified = aio_parse_date(val, NULL);
    } else if (_xml_eqElm(_s_dae_keywords)) {
      _xml_readText(*dest, (*dest)->keywords);
    } else if (_xml_eqElm(_s_dae_revision)) {
      const char * val;
      _xml_readText(*dest, val);
      (*dest)->revision = strtoul(val, NULL, 10);
    } else if (_xml_eqElm(_s_dae_subject)) {
      _xml_readText(*dest, (*dest)->subject);
    } else if (_xml_eqElm(_s_dae_title)) {
      _xml_readText(*dest, (*dest)->title);
    } else if (_xml_eqElm(_s_dae_unit)) {
      aio_unit * unit;
      unit = aio_calloc(*dest, sizeof(*unit), 1);

      _xml_readAttr(*dest, unit->name, _s_dae_name);
      _xml_readAttrUsingFn(unit->dist, _s_dae_meter, strtod, NULL);

      (*dest)->unit = unit;
    } else if (_xml_eqElm(_s_dae_up_axis)) {
      const char * val;
      aio_upaxis   upaxis;

      _xml_readConstText(val);
      if (val) {
        if (strcasecmp(val, _s_dae_z_up))
          upaxis = AIO_UP_AXIS_Z;
        else if (strcasecmp(val, _s_dae_x_up))
          upaxis = AIO_UP_AXIS_X;
        else
          upaxis = AIO_UP_AXIS_Y;

        (*dest)->upaxis = upaxis;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      aio_tree * tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      aio_tree_fromXmlNode(*dest, nodePtr, &tree, NULL);
      (*dest)->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  return 0;
}
