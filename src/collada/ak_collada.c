/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada.h"
#include "ak_collada_common.h"
#include "ak_collada_asset.h"
#include "ak_collada_camera.h"
#include "ak_collada_light.h"

#include "core/ak_collada_geometry.h"
#include "core/ak_collada_controller.h"
#include "core/ak_collada_visual_scene.h"
#include "core/ak_collada_node.h"
#include "core/ak_collada_scene.h"

#include "fx/ak_collada_fx_effect.h"
#include "fx/ak_collada_fx_image.h"
#include "fx/ak_collada_fx_material.h"

AkResult
_assetkit_hide
ak_dae_doc(AkDoc ** __restrict dest,
           const char * __restrict file) {

  AkDoc            *doc;
  AkLibCamera      *last_libCam;
  AkLibLight       *last_libLight;
  AkLibEffect      *last_libEffect;
  AkLibImage       *last_libImage;
  AkLibMaterial    *last_libMaterial;
  AkLibGeometry    *last_libGeometry;
  AkLibController  *last_libController;
  AkLibVisualScene *last_libVisualScene;
  AkLibNode        *last_libNode;

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
    fprintf(stderr, "assetkit: Unable to open %s\n", file);
    return AK_ERR;
  }

  doc = ak_calloc(NULL, sizeof(*doc), false);

  _xml_readNext;

  last_libCam         = NULL;
  last_libLight       = NULL;
  last_libEffect      = NULL;
  last_libImage       = NULL;
  last_libMaterial    = NULL;
  last_libGeometry    = NULL;
  last_libController  = NULL;
  last_libVisualScene = NULL;
  last_libNode        = NULL;

  do {
    _xml_beginElement(_s_dae_collada);

    /* COLLADA Core */
    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf * assetInf;
      AkDocInf   * docInf;
      AkResult      ret;

      docInf = ak_calloc(doc, sizeof(*docInf), false);
      assetInf = &docInf->base;

      ret = ak_dae_assetInf(docInf, reader, &assetInf);
      if (ret == AK_OK) {
        docInf->ftype = AK_FILE_TYPE_COLLADA;
        doc->docinf = *docInf;
      }
    } else if (_xml_eqElm(_s_dae_lib_cameras)) {
      AkLibCamera *libcam;
      AkCamera    *lastcam;

      libcam = ak_calloc(doc, sizeof(*libcam), false);
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
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(doc, reader, &assetInf);
          if (ret == AK_OK)
            libcam->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_camera)) {
          AkCamera *acamera;
          AkResult   ret;

          acamera = NULL;
          ret = ak_dae_camera(doc, reader, &acamera);
          if (ret == AK_OK) {
            if (lastcam)
              lastcam->next = acamera;
            else
              libcam->chld = acamera;

            lastcam = acamera;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libcam->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_lights)) {
      AkLibLight *liblight;
      AkLight    *lastlight;

      liblight = ak_calloc(doc, sizeof(*liblight), false);
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
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(doc, reader, &assetInf);
          if (ret == AK_OK)
            liblight->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_light)) {
          AkLight *alight;
          AkResult  ret;

          alight = NULL;
          ret = ak_dae_light(doc, reader, &alight);
          if (ret == AK_OK) {
            if (lastlight)
              lastlight->next = alight;
            else
              liblight->chld = alight;

            lastlight = alight;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
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
      AkLibEffect *libEffect;
      AkEffect    *lastEffect;

      libEffect = ak_calloc(doc, sizeof(*libEffect), false);
      if (last_libEffect)
        last_libEffect->next = libEffect;
      else
        doc->lib.effects = libEffect;

      last_libEffect = libEffect;

      _xml_readAttr(libEffect, libEffect->id, _s_dae_id);
      _xml_readAttr(libEffect, libEffect->name, _s_dae_name);

      lastEffect = NULL;

      do {
        _xml_beginElement(_s_dae_lib_effects);

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(doc, reader, &assetInf);
          if (ret == AK_OK)
            libEffect->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_effect)) {
          AkEffect *anEffect;
          AkResult   ret;

          ret = ak_dae_effect(doc, reader, &anEffect);
          if (ret == AK_OK) {
            if (lastEffect)
              lastEffect->next = anEffect;
            else
              libEffect->chld = anEffect;

            lastEffect = anEffect;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libEffect->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);

    } else if (_xml_eqElm(_s_dae_lib_images)) {
      AkLibImage *libimg;
      AkImage      *lastimg;

      libimg = ak_calloc(doc, sizeof(*libimg), false);
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
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(doc, reader, &assetInf);
          if (ret == AK_OK)
            libimg->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_image)) {
          AkImage *anImg;
          AkResult  ret;
          
          ret = ak_dae_fxImage(doc, reader, &anImg);
          if (ret == AK_OK) {
            if (lastimg)
              lastimg->next = anImg;
            else
              libimg->chld = anImg;

            lastimg = anImg;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libimg->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_materials)) {
      AkLibMaterial *libMaterial;
      AkMaterial     *lastMaterial;

      libMaterial = ak_calloc(doc, sizeof(*libMaterial), false);
      if (last_libMaterial)
        last_libMaterial->next = libMaterial;
      else
        doc->lib.materials = libMaterial;

      last_libMaterial = libMaterial;

      _xml_readAttr(libMaterial, libMaterial->id, _s_dae_id);
      _xml_readAttr(libMaterial, libMaterial->name, _s_dae_name);

      lastMaterial = NULL;

      do {
        _xml_beginElement(_s_dae_lib_materials);

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(doc, reader, &assetInf);
          if (ret == AK_OK)
            libMaterial->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_material)) {
          AkMaterial *material;
          AkResult ret;

          material = NULL;
          ret = ak_dae_material(doc, reader, &material);
          if (ret == AK_OK) {
            if (lastMaterial)
              lastMaterial->next = material;
            else
              libMaterial->chld = material;

            lastMaterial = material;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libMaterial->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_geometries)) {
      AkLibGeometry *libGeometry;
      AkGeometry    *lastGeometry;

      libGeometry = ak_calloc(doc, sizeof(*libGeometry), false);
      if (last_libGeometry)
        last_libGeometry->next = libGeometry;
      else
        doc->lib.geometries = libGeometry;

      last_libGeometry = libGeometry;

      _xml_readAttr(libGeometry, libGeometry->id, _s_dae_id);
      _xml_readAttr(libGeometry, libGeometry->name, _s_dae_name);

      lastGeometry = NULL;

      do {
        _xml_beginElement(_s_dae_lib_geometries);

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(doc, reader, &assetInf);
          if (ret == AK_OK)
            libGeometry->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_geometry)) {
          AkGeometry *geometry;
          AkResult ret;

          geometry = NULL;
          ret = ak_dae_geometry(doc, reader, &geometry);
          if (ret == AK_OK) {
            if (lastGeometry)
              lastGeometry->next = geometry;
            else
              libGeometry->chld = geometry;

            lastGeometry = geometry;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libGeometry->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_controllers)) {
      AkLibController *libController;
      AkController    *lastController;

      libController = ak_calloc(doc, sizeof(*libController), false);
      if (last_libController)
        last_libController->next = libController;
      else
        doc->lib.controllers = libController;

      last_libController = libController;

      _xml_readAttr(libController, libController->id, _s_dae_id);
      _xml_readAttr(libController, libController->name, _s_dae_name);

      lastController = NULL;

      do {
        _xml_beginElement(_s_dae_lib_controllers);

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(doc, reader, &assetInf);
          if (ret == AK_OK)
            libController->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_controller)) {
          AkController *controller;
          AkResult ret;

          controller = NULL;
          ret = ak_dae_controller(doc, reader, &controller);
          if (ret == AK_OK) {
            if (lastController)
              lastController->next = controller;
            else
              libController->chld = controller;

            lastController = controller;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libController->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_visual_scenes)) {
      AkLibVisualScene *libVisualScene;
      AkVisualScene    *lastVisualScene;

      libVisualScene = ak_calloc(doc, sizeof(*libVisualScene), false);
      if (last_libVisualScene)
        last_libVisualScene->next = libVisualScene;
      else
        doc->lib.visualScenes = libVisualScene;

      last_libVisualScene = libVisualScene;

      _xml_readAttr(libVisualScene, libVisualScene->id, _s_dae_id);
      _xml_readAttr(libVisualScene, libVisualScene->name, _s_dae_name);

      lastVisualScene = NULL;

      do {
        _xml_beginElement(_s_dae_lib_visual_scenes);

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(doc, reader, &assetInf);
          if (ret == AK_OK)
            libVisualScene->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_controller)) {
          AkVisualScene *visualScene;
          AkResult ret;

          visualScene = NULL;
          ret = ak_dae_visualScene(doc, reader, &visualScene);
          if (ret == AK_OK) {
            if (lastVisualScene)
              lastVisualScene->next = visualScene;
            else
              libVisualScene->chld = visualScene;

            lastVisualScene = visualScene;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libVisualScene->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_nodes)) {
      AkLibNode *libNode;
      AkNode    *lastNode;

      libNode = ak_calloc(doc, sizeof(*libNode), false);
      if (last_libNode)
        last_libNode->next = libNode;
      else
        doc->lib.nodes = libNode;

      last_libNode = libNode;

      _xml_readAttr(libNode, libNode->id, _s_dae_id);
      _xml_readAttr(libNode, libNode->name, _s_dae_name);

      lastNode = NULL;

      do {
        _xml_beginElement(_s_dae_lib_nodes);

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(doc, reader, &assetInf);
          if (ret == AK_OK)
            libNode->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_controller)) {
          AkNode  *node;
          AkResult ret;

          node = NULL;
          ret = ak_dae_node(doc, reader, &node);
          if (ret == AK_OK) {
            if (lastNode)
              lastNode->next = node;
            else
              libNode->chld = node;

            lastNode = node;
          }
        } else if (_xml_eqElm(_s_dae_extra)) {
          xmlNodePtr nodePtr;
          AkTree   *tree;

          nodePtr = xmlTextReaderExpand(reader);
          tree = NULL;

          ak_tree_fromXmlNode(doc, nodePtr, &tree, NULL);
          libNode->extra = tree;

          _xml_skipElement;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);
    } else if (_xml_eqElm(_s_dae_scene)) {
      ak_dae_scene(doc, reader, &doc->scene);
    } /* if */

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  xmlFreeTextReader(reader);

  if (nodeRet == -1) {
    fprintf(stderr, "%s : failed to parse\n", file);
    return AK_ERR;
  }

  *dest = doc;

  return AK_OK;
}
