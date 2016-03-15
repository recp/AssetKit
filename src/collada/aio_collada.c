/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada.h"
#include "aio_collada_common.h"
#include "aio_collada_asset.h"
#include "aio_collada_camera.h"
#include "aio_collada_light.h"

#include "fx/aio_collada_fx_effect.h"
#include "fx/aio_collada_fx_image.h"
#include "fx/aio_collada_fx_material.h"

int
_assetio_hide
aio_dae_doc(aio_doc ** __restrict dest,
            const char * __restrict file) {

  aio_doc          *doc;
  aio_lib_camera   *last_libCam;
  aio_lib_light    *last_libLight;
  aio_lib_effect   *last_libEffect;
  aio_lib_image    *last_libImage;
  aio_lib_material *last_libMaterial;

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

  doc = aio_calloc(NULL, sizeof(*doc), 1);

  _xml_readNext;

  last_libCam      = NULL;
  last_libLight    = NULL;
  last_libEffect   = NULL;
  last_libImage    = NULL;
  last_libMaterial = NULL;

  do {
    _xml_beginElement(_s_dae_collada);

    /* COLLADA Core */
    if (_xml_eqElm(_s_dae_asset)) {
      aio_assetinf * assetInf;
      aio_docinf   * docInf;
      int          ret;

      docInf = aio_calloc(doc, sizeof(*docInf), 1);
      assetInf = &docInf->base;

      ret = aio_dae_assetInf(docInf, reader, &assetInf);
      if (ret == 0) {
        docInf->ftype = AIO_FILE_TYPE_COLLADA;
        doc->docinf = *docInf;
      }
    } else if (_xml_eqElm(_s_dae_lib_cameras)) {
      aio_lib_camera *libcam;
      aio_camera     *lastcam;

      libcam = aio_calloc(doc, sizeof(*libcam), 1);
      if (last_libCam)
        last_libCam->next = libcam;
      else
        doc->lib.cameras = libcam;

      last_libCam = libcam;

      _xml_readAttr(libcam, libcam->id, _s_dae_id);
      _xml_readAttr(libcam, libcam->name, _s_dae_name);

      lastcam = NULL;

      do {
        _xml_beginElement(_s_dae_lib_cameras);

        if (_xml_eqElm(_s_dae_asset)) {
          aio_assetinf *assetInf;
          int ret;

          assetInf = NULL;
          ret = aio_dae_assetInf(doc, reader, &assetInf);
          if (ret == 0)
            libcam->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_camera)) {
          aio_camera *acamera;
          int         ret;

          acamera = NULL;
          ret = aio_dae_camera(doc, reader, &acamera);
          if (ret == 0) {
            if (lastcam)
              lastcam->next = acamera;
            else
              libcam->chld = acamera;

            lastcam = acamera;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          aio_tree  *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          aio_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
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

      liblight = aio_calloc(doc, sizeof(*liblight), 1);
      if (last_libLight)
        last_libLight->next = liblight;
      else
        doc->lib.lights = liblight;

      last_libLight = liblight;

      _xml_readAttr(liblight, liblight->id, _s_dae_id);
      _xml_readAttr(liblight, liblight->name, _s_dae_name);

      lastlight = NULL;

      do {
        _xml_beginElement(_s_dae_lib_lights);

        if (_xml_eqElm(_s_dae_asset)) {
          aio_assetinf *assetInf;
          int ret;

          assetInf = NULL;
          ret = aio_dae_assetInf(doc, reader, &assetInf);
          if (ret == 0)
            liblight->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_light)) {
          aio_light *alight;
          int        ret;

          alight = NULL;
          ret = aio_dae_light(doc, reader, &alight);
          if (ret == 0) {
            if (lastlight)
              lastlight->next = alight;
            else
              liblight->chld = alight;

            lastlight = alight;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          aio_tree  *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          aio_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
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
      aio_lib_effect *libeffect;
      aio_effect     *lasteffect;

      libeffect = aio_calloc(doc, sizeof(*libeffect), 1);
      if (last_libEffect)
        last_libEffect->next = libeffect;
      else
        doc->lib.effects = libeffect;

      last_libEffect = libeffect;

      _xml_readAttr(libeffect, libeffect->id, _s_dae_id);
      _xml_readAttr(libeffect, libeffect->name, _s_dae_name);

      lasteffect = NULL;

      do {
        _xml_beginElement(_s_dae_lib_effects);

        if (_xml_eqElm(_s_dae_asset)) {
          aio_assetinf *assetInf;
          int ret;

          assetInf = NULL;
          ret = aio_dae_assetInf(doc, reader, &assetInf);
          if (ret == 0)
            libeffect->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_effect)) {
          aio_effect *anEffect;
          int         ret;

          ret = aio_dae_effect(doc, reader, &anEffect);
          if (ret == 0) {
            if (lasteffect)
              lasteffect->next = anEffect;
            else
              libeffect->chld = anEffect;

            lasteffect = anEffect;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          aio_tree  *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          aio_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libeffect->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);

    } else if (_xml_eqElm(_s_dae_lib_images)) {
      aio_lib_image *libimg;
      aio_image     *lastimg;

      libimg = aio_calloc(doc, sizeof(*libimg), 1);
      if (last_libImage)
        last_libImage->next = libimg;
      else
        doc->lib.images = libimg;

      last_libImage = libimg;

      _xml_readAttr(libimg, libimg->id, _s_dae_id);
      _xml_readAttr(libimg, libimg->name, _s_dae_name);

      lastimg = NULL;

      do {
        _xml_beginElement(_s_dae_lib_images);

        if (_xml_eqElm(_s_dae_asset)) {
          aio_assetinf *assetInf;
          int ret;

          assetInf = NULL;
          ret = aio_dae_assetInf(doc, reader, &assetInf);
          if (ret == 0)
            libimg->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_image)) {
          aio_image *anImg;
          int        ret;
          
          ret = aio_dae_fxImage(doc, reader, &anImg);
          if (ret == 0) {
            if (lastimg)
              lastimg->next = anImg;
            else
              libimg->chld = anImg;

            lastimg = anImg;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          aio_tree  *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          aio_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libimg->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_materials)) {
      aio_lib_material *libmaterial;
      aio_material     *lastmaterial;

      libmaterial = aio_calloc(doc, sizeof(*libmaterial), 1);
      if (last_libMaterial)
        last_libMaterial->next = libmaterial;
      else
        doc->lib.materials = libmaterial;

      last_libMaterial = libmaterial;

      _xml_readAttr(libmaterial, libmaterial->id, _s_dae_id);
      _xml_readAttr(libmaterial, libmaterial->name, _s_dae_name);

      lastmaterial = NULL;

      do {
        _xml_beginElement(_s_dae_lib_materials);

        if (_xml_eqElm(_s_dae_asset)) {
          aio_assetinf *assetInf;
          int ret;

          assetInf = NULL;
          ret = aio_dae_assetInf(doc, reader, &assetInf);
          if (ret == 0)
            libmaterial->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_material)) {
          aio_material *material;
          int ret;

          material = NULL;
          ret = aio_dae_material(doc, reader, &material);
          if (ret == 0) {
            if (lastmaterial)
              lastmaterial->next = material;
            else
              libmaterial->chld = material;

            lastmaterial = material;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          aio_tree  *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          aio_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libmaterial->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  xmlFreeTextReader(reader);

  if (nodeRet == -1) {
    fprintf(stderr, "%s : failed to parse\n", file);
    return -1;
  }

  *dest = doc;

  return 0;
}
