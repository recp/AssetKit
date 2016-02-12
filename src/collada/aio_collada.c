/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada.h"
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "../aio_memory.h"
#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_tree.h"
#include "aio_collada_common.h"

#include "aio_collada_asset.h"
#include "aio_collada_camera.h"
#include "aio_collada_light.h"
#include "fx/aio_collada_fx_effect.h"
#include "fx/aio_collada_fx_image.h"

#include <unistd.h>
#include <libxml/xmlreader.h>

#include <stdio.h>
#include <sys/stat.h>

int
_assetio_hide
aio_dae_doc(aio_doc ** __restrict dest,
            const char * __restrict file) {

  aio_doc          *doc;
  xmlTextReaderPtr  reader;
  const xmlChar    *nodeName;
  int nodeType;
  int nodeRet;
  int xmlReaderFlags;

  /*
   * this initialize the library and check potential ABI mismatches
   * between the version it was compiled for and the actual shared
   * library used.
   */
  LIBXML_TEST_VERSION

  xmlReaderFlags = XML_PARSE_NOBLANKS
                   |XML_PARSE_NOCDATA
                   |XML_PARSE_NOXINCNODE
                   |XML_PARSE_NOBASEFIX
#ifndef _DEBUG
                   |XML_PARSE_NOERROR
                   |XML_PARSE_NOWARNING
#endif
  ;

  reader = xmlReaderForFile(file, NULL, xmlReaderFlags);
  if (!reader) {
    fprintf(stderr, "assetio: Unable to open %s\n", file);
    return -1;
  }

  doc = aio_malloc(sizeof(*doc));
  memset(doc, '\0', sizeof(*doc));

  _xml_readNext;

  do {
    _xml_beginElement(_s_dae_collada);

    /* COLLADA Core */
    if (_xml_eqElm(_s_dae_asset)) {
      aio_assetinf * assetInf;
      aio_docinf   * docInf;
      int          ret;

      docInf = aio_calloc(sizeof(*docInf), 1);
      assetInf = &docInf->base;

      ret = aio_dae_assetInf(reader, &assetInf);
      if (ret == 0) {
        docInf->ftype = AIO_FILE_TYPE_COLLADA;
        doc->docinf = *docInf;
      }
    } else if (_xml_eqElm(_s_dae_lib_cameras)) {
      aio_lib_camera *libcam;
      aio_camera     *lastcam;

      libcam = &doc->lib.cameras;

      _xml_readAttr(libcam->id, _s_dae_id);
      _xml_readAttr(libcam->name, _s_dae_name);

      lastcam = libcam->next;

      do {
        _xml_beginElement(_s_dae_lib_cameras);

        if (_xml_eqElm(_s_dae_asset)) {
          aio_assetinf *assetInf;
          int ret;

          assetInf = NULL;
          ret = aio_dae_assetInf(reader, &assetInf);
          if (ret == 0)
            doc->lib.cameras.inf = assetInf;

        } else if (_xml_eqElm(_s_dae_camera)) {
          aio_camera *acamera;
          int         ret;

          acamera = NULL;
          ret = aio_dae_camera(reader, &acamera);
          if (ret == 0) {
            if (lastcam)
              lastcam->next = acamera;
            else
              libcam->next = acamera;

            lastcam = acamera;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          aio_tree  *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          aio_tree_fromXmlNode(nodePtr, &tree, NULL);
          libcam->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_lights)) {
      aio_lib_light *liblight;
      aio_light     *lastlight;

      liblight = &doc->lib.lights;

      _xml_readAttr(liblight->id, _s_dae_id);
      _xml_readAttr(liblight->name, _s_dae_name);

      lastlight = liblight->next;

      do {
        _xml_beginElement(_s_dae_lib_lights);

        if (_xml_eqElm(_s_dae_asset)) {
          aio_assetinf *assetInf;
          int ret;

          assetInf = NULL;
          ret = aio_dae_assetInf(reader, &assetInf);
          if (ret == 0)
            doc->lib.cameras.inf = assetInf;

        } else if (_xml_eqElm(_s_dae_light)) {
          aio_light *alight;
          int        ret;

          alight = NULL;
          ret = aio_dae_light(reader, &alight);
          if (ret == 0) {
            if (lastlight)
              lastlight->next = alight;
            else
              liblight->next = alight;

            lastlight = alight;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          aio_tree  *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          aio_tree_fromXmlNode(nodePtr, &tree, NULL);
          liblight->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);

    /* COLLADA FX */
    } else if (_xml_eqElm(_s_dae_lib_effects)) {
      
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  xmlFreeTextReader(reader);

  if (nodeRet == -1) {
    fprintf(stderr, "%s : failed to parse\n", file);
    return -1;
  }


//  do {

//      /* COLLADA Core */
//      if (AIO_IS_EQ_CASE(node_name, "asset")) {
//      } else if (AIO_IS_EQ_CASE(node_name, "library_cameras")) {
//      } else if (AIO_IS_EQ_CASE(node_name, "library_lights")) {
//      /* COLLADA FX */
//      else if (AIO_IS_EQ_CASE(node_name, "library_effects")) {
//        xmlNode        * prev_node;
//        aio_lib_effect * effect_lib;
//
//        effect_lib = &asst_doc->lib.effects;
//
//        aio_xml_collada_read_id_name(curr_node,
//                                     &effect_lib->id,
//                                     &effect_lib->name);
//
//        prev_node = curr_node;
//        curr_node = curr_node->children;
//
//        while (curr_node) {
//          if (curr_node->type == XML_ELEMENT_NODE) {
//            node_name = (const char *)curr_node->name;
//
//            if (AIO_IS_EQ_CASE(node_name, "asset")) {
//              _AIO_ASSET_LOAD_TO(curr_node,
//                                 asst_doc->lib.effects.inf);
//
//            } else if (AIO_IS_EQ_CASE(node_name, "effect")) {
//              aio_effect * an_effect;
//              int          ret;
//
//              ret = aio_load_collada_effect(curr_node, &an_effect);
//              if (ret == 0) {
//                if (effect_lib->next) {
//                  an_effect->prev = effect_lib->next;
//                  effect_lib->next = an_effect;
//                } else {
//                  effect_lib->next = an_effect;
//                }
//              }
//            } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
//              _AIO_TREE_LOAD_TO(curr_node->children,
//                                asst_doc->lib.effects.extra,
//                                NULL);
//            }
//          }
//
//          curr_node = curr_node->next;
//        } /* while library_cameras */
//        
//        node_name = NULL;
//        curr_node = prev_node;
//
//      } else if (AIO_IS_EQ_CASE(node_name, "library_images")) {
//        xmlNode       * prev_node;
//        aio_lib_image * image_lib;
//
//        image_lib = &asst_doc->lib.images;
//
//        aio_xml_collada_read_id_name(curr_node,
//                                     &image_lib->id,
//                                     &image_lib->name);
//
//        prev_node = curr_node;
//        curr_node = curr_node->children;
//
//        while (curr_node) {
//          if (curr_node->type == XML_ELEMENT_NODE) {
//            node_name = (const char *)curr_node->name;
//
//            if (AIO_IS_EQ_CASE(node_name, "asset")) {
//              _AIO_ASSET_LOAD_TO(curr_node,
//                                 asst_doc->lib.images.inf);
//
//            } else if (AIO_IS_EQ_CASE(node_name, "image")) {
//              aio_image * an_image;
//              int         ret;
//
//              ret = aio_load_collada_image(curr_node, &an_image);
//              if (ret == 0) {
//                if (image_lib->next) {
//                  an_image->prev = image_lib->next;
//                  image_lib->next = an_image;
//                } else {
//                  image_lib->next = an_image;
//                }
//              }
//
//            } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
//              _AIO_TREE_LOAD_TO(curr_node->children,
//                                asst_doc->lib.images.extra,
//                                NULL);
//            }
//          }
//
//          curr_node = curr_node->next;
//        } /* while library_cameras */
//
//        node_name = NULL;
//        curr_node = prev_node;
//      }
//    }


  return 0;
err:
  return -1;
}
