/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada.h"
#include "ak_collada_common.h"

#include "core/ak_collada_asset.h"
#include "core/ak_collada_camera.h"
#include "core/ak_collada_light.h"
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
  AkHeap           *heap;
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
  AkXmlState        xstVal, *xst;

  xmlTextReaderPtr  reader;
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

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc), false);
  
  ak_heap_setdata(heap, doc);

  xstVal.doc    = doc;
  xstVal.heap   = heap;
  xstVal.reader = reader;
  xst           = &xstVal;

//  _xml_readNext;
  ak_xml_readnext(xst);

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
    if (ak_xml_beginelm(xst, _s_dae_collada))
      break;

    /* COLLADA Core */
    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkDocInf   *docInf;
      AkResult    ret;

      docInf = ak_heap_calloc(heap, doc, sizeof(*docInf), false);
      assetInf = &docInf->base;

      ret = ak_dae_assetInf(xst, docInf, &assetInf);
      if (ret == AK_OK) {
        docInf->ftype = AK_FILE_TYPE_COLLADA;
        doc->docinf   = *docInf;

        doc->coordSys = assetInf->coordSys;
        doc->unit     = assetInf->unit;
      }
    } else if (_xml_eqElm(_s_dae_lib_cameras)) {
      AkLibCamera *libcam;
      AkCamera    *lastcam;

      libcam = ak_heap_calloc(heap, doc, sizeof(*libcam), true);
      if (last_libCam)
        last_libCam->next = libcam;
      else
        doc->lib.cameras = libcam;

      last_libCam = libcam;

      _xml_readId(libcam);
      _xml_readAttr(libcam, libcam->name, _s_dae_name);

      lastcam = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_lib_cameras))
          break;

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(xst, doc, &assetInf);
          if (ret == AK_OK)
            libcam->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_camera)) {
          AkCamera *acamera;
          AkResult   ret;

          acamera = NULL;
          ret = ak_dae_camera(xst, doc, &acamera);
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

                ak_tree_fromXmlNode(heap,
                          doc,
                          nodePtr,
                          &tree,
                          NULL);
          libcam->extra = tree;

          ak_xml_skipelm(xst);;
        } else {
          ak_xml_skipelm(xst);;
        }

        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_lights)) {
      AkLibLight *liblight;
      AkLight    *lastlight;

      liblight = ak_heap_calloc(heap, doc, sizeof(*liblight), true);
      if (last_libLight)
        last_libLight->next = liblight;
      else
        doc->lib.lights = liblight;

      last_libLight = liblight;

      _xml_readId(liblight);
      _xml_readAttr(liblight, liblight->name, _s_dae_name);

      lastlight = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_lib_lights))
          break;

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(xst, doc, &assetInf);
          if (ret == AK_OK)
            liblight->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_light)) {
          AkLight *alight;
          AkResult  ret;

          alight = NULL;
          ret = ak_dae_light(xst, doc, &alight);
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

          ak_tree_fromXmlNode(heap,
                              doc,
                              nodePtr,
                              &tree,
                              NULL);
          liblight->extra = tree;

          ak_xml_skipelm(xst);;
        } else {
          ak_xml_skipelm(xst);;
        }
        
        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);

    /* COLLADA FX */
    } else if (_xml_eqElm(_s_dae_lib_effects)) {
      AkLibEffect *libEffect;
      AkEffect    *lastEffect;

      libEffect = ak_heap_calloc(heap, doc, sizeof(*libEffect), true);
      if (last_libEffect)
        last_libEffect->next = libEffect;
      else
        doc->lib.effects = libEffect;

      last_libEffect = libEffect;

      _xml_readId(libEffect);
      _xml_readAttr(libEffect, libEffect->name, _s_dae_name);

      lastEffect = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_lib_effects))
          break;

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(xst, doc, &assetInf);
          if (ret == AK_OK)
            libEffect->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_effect)) {
          AkEffect *anEffect;
          AkResult   ret;

          ret = ak_dae_effect(xst, doc, &anEffect);
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

                ak_tree_fromXmlNode(heap,
                          doc,
                          nodePtr,
                          &tree,
                          NULL);
          libEffect->extra = tree;

          ak_xml_skipelm(xst);;
        } else {
          ak_xml_skipelm(xst);;
        }
        
        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);

    } else if (_xml_eqElm(_s_dae_lib_images)) {
      AkLibImage *libimg;
      AkImage      *lastimg;

      libimg = ak_heap_calloc(heap, doc, sizeof(*libimg), true);
      if (last_libImage)
        last_libImage->next = libimg;
      else
        doc->lib.images = libimg;

      last_libImage = libimg;

      _xml_readId(libimg);
      _xml_readAttr(libimg, libimg->name, _s_dae_name);

      lastimg = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_lib_images))
          break;

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(xst, doc, &assetInf);
          if (ret == AK_OK)
            libimg->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_image)) {
          AkImage *anImg;
          AkResult  ret;
          
          ret = ak_dae_fxImage(xst, doc, &anImg);
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

                ak_tree_fromXmlNode(heap,
                          doc,
                          nodePtr,
                          &tree,
                          NULL);
          libimg->extra = tree;

          ak_xml_skipelm(xst);;
        } else {
          ak_xml_skipelm(xst);;
        }
        
        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_materials)) {
      AkLibMaterial *libMaterial;
      AkMaterial     *lastMaterial;

      libMaterial = ak_heap_calloc(heap, doc, sizeof(*libMaterial), true);
      if (last_libMaterial)
        last_libMaterial->next = libMaterial;
      else
        doc->lib.materials = libMaterial;

      last_libMaterial = libMaterial;

      _xml_readId(libMaterial);
      _xml_readAttr(libMaterial, libMaterial->name, _s_dae_name);

      lastMaterial = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_lib_materials))
          break;

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(xst, doc, &assetInf);
          if (ret == AK_OK)
            libMaterial->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_material)) {
          AkMaterial *material;
          AkResult ret;

          material = NULL;
          ret = ak_dae_material(xst, doc, &material);
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

                ak_tree_fromXmlNode(heap,
                          doc,
                          nodePtr,
                          &tree,
                          NULL);
          libMaterial->extra = tree;

          ak_xml_skipelm(xst);;
        } else {
          ak_xml_skipelm(xst);;
        }
        
        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_geometries)) {
      AkLibGeometry *libGeometry;
      AkGeometry    *lastGeometry;

      libGeometry = ak_heap_calloc(heap, doc, sizeof(*libGeometry), true);
      if (last_libGeometry)
        last_libGeometry->next = libGeometry;
      else
        doc->lib.geometries = libGeometry;

      last_libGeometry = libGeometry;

      _xml_readId(libGeometry);
      _xml_readAttr(libGeometry, libGeometry->name, _s_dae_name);

      lastGeometry = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_lib_geometries))
          break;

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(xst, doc, &assetInf);
          if (ret == AK_OK)
            libGeometry->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_geometry)) {
          AkGeometry *geometry;
          AkResult ret;

          geometry = NULL;
          ret = ak_dae_geometry(xst, doc, &geometry);
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

                ak_tree_fromXmlNode(heap,
                          doc,
                          nodePtr,
                          &tree,
                          NULL);
          libGeometry->extra = tree;

          ak_xml_skipelm(xst);;
        } else {
          ak_xml_skipelm(xst);;
        }
        
        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_controllers)) {
      AkLibController *libController;
      AkController    *lastController;

      libController = ak_heap_calloc(heap, doc, sizeof(*libController), true);
      if (last_libController)
        last_libController->next = libController;
      else
        doc->lib.controllers = libController;

      last_libController = libController;

      _xml_readId(libController);
      _xml_readAttr(libController, libController->name, _s_dae_name);

      lastController = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_lib_controllers))
          break;

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(xst, doc, &assetInf);
          if (ret == AK_OK)
            libController->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_controller)) {
          AkController *controller;
          AkResult ret;

          controller = NULL;
          ret = ak_dae_controller(xst, doc, &controller);
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

                ak_tree_fromXmlNode(heap,
                          doc,
                          nodePtr,
                          &tree,
                          NULL);
          libController->extra = tree;

          ak_xml_skipelm(xst);;
        } else {
          ak_xml_skipelm(xst);;
        }
        
        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_visual_scenes)) {
      AkLibVisualScene *libVisualScene;
      AkVisualScene    *lastVisualScene;

      libVisualScene = ak_heap_calloc(heap,
                                      doc,
                                      sizeof(*libVisualScene),
                                      true);
      if (last_libVisualScene)
        last_libVisualScene->next = libVisualScene;
      else
        doc->lib.visualScenes = libVisualScene;

      last_libVisualScene = libVisualScene;

      _xml_readId(libVisualScene);
      _xml_readAttr(libVisualScene, libVisualScene->name, _s_dae_name);

      lastVisualScene = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_lib_visual_scenes))
          break;

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(xst, doc, &assetInf);
          if (ret == AK_OK)
            libVisualScene->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_visual_scene)) {
          AkVisualScene *visualScene;
          AkResult ret;

          visualScene = NULL;
          ret = ak_dae_visualScene(xst, doc, &visualScene);
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

                ak_tree_fromXmlNode(heap,
                          doc,
                          nodePtr,
                          &tree,
                          NULL);
          libVisualScene->extra = tree;

          ak_xml_skipelm(xst);;
        } else {
          ak_xml_skipelm(xst);;
        }
        
        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);
    } else if (_xml_eqElm(_s_dae_lib_nodes)) {
      AkLibNode *libNode;
      AkNode    *lastNode;

      libNode = ak_heap_calloc(heap, doc, sizeof(*libNode), true);
      if (last_libNode)
        last_libNode->next = libNode;
      else
        doc->lib.nodes = libNode;

      last_libNode = libNode;

      _xml_readId(libNode);
      _xml_readAttr(libNode, libNode->name, _s_dae_name);

      lastNode = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_lib_nodes))
          break;

        if (_xml_eqElm(_s_dae_asset)) {
          AkAssetInf *assetInf;
          AkResult ret;

          assetInf = NULL;
          ret = ak_dae_assetInf(xst, doc, &assetInf);
          if (ret == AK_OK)
            libNode->inf = assetInf;

        } else if (_xml_eqElm(_s_dae_node)) {
          AkNode  *node;
          AkResult ret;

          node = NULL;
          ret = ak_dae_node(xst, doc, NULL, &node);
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

                ak_tree_fromXmlNode(heap,
                          doc,
                          nodePtr,
                          &tree,
                          NULL);
          libNode->extra = tree;

          ak_xml_skipelm(xst);;
        } else {
          ak_xml_skipelm(xst);;
        }
        
        /* end element */
        ak_xml_endelm(xst);;
      } while (xst->nodeRet);
    } else if (_xml_eqElm(_s_dae_scene)) {
      ak_dae_scene(xst, doc, &doc->scene);
    } /* if */

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  xmlFreeTextReader(reader);

  if (xst->nodeRet == -1) {
    fprintf(stderr, "%s : failed to parse\n", file);
    return AK_ERR;
  }

  *dest = doc;

  /* now set used coordSys */
  doc->coordSys = ak_defaultCoordSys();

  return AK_OK;
}
