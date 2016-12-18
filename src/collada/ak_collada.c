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

#include "ak_collada_lib.h"
#include "ak_collada_postscript.h"

#define k_s_dae_asset               1
#define k_s_dae_lib_cameras         2
#define k_s_dae_lib_lights          3
#define k_s_dae_lib_effects         4
#define k_s_dae_lib_images          5
#define k_s_dae_lib_materials       6
#define k_s_dae_lib_geometries      7
#define k_s_dae_lib_controllers     8
#define k_s_dae_lib_visual_scenes   9
#define k_s_dae_lib_nodes           10
#define k_s_dae_scene               11
#define k_s_dae_extra               12

static ak_enumpair daeMap[] = {
  {_s_dae_asset,             k_s_dae_asset},
  {_s_dae_lib_cameras,       k_s_dae_lib_cameras},
  {_s_dae_lib_lights,        k_s_dae_lib_lights},
  {_s_dae_lib_effects,       k_s_dae_lib_effects},
  {_s_dae_lib_images,        k_s_dae_lib_images},
  {_s_dae_lib_materials,     k_s_dae_lib_materials},
  {_s_dae_lib_geometries,    k_s_dae_lib_geometries},
  {_s_dae_lib_controllers,   k_s_dae_lib_controllers},
  {_s_dae_lib_visual_scenes, k_s_dae_lib_visual_scenes},
  {_s_dae_lib_nodes,         k_s_dae_lib_nodes},
  {_s_dae_scene,             k_s_dae_scene},
  {_s_dae_extra,             k_s_dae_extra},
};

static size_t daeMapLen = 0;

static AkLibChldDesc libchlds[] = {
  {
    NULL,
    _s_dae_lib_cameras,
    _s_dae_camera,
    ak_dae_camera,
    offsetof(AkLib, cameras),
    offsetof(AkCamera, next)
  },
  {
    NULL,
    _s_dae_lib_lights,
    _s_dae_light,
    ak_dae_light,
    offsetof(AkLib, lights),
    offsetof(AkLight, next)
  },
  {
    NULL,
    _s_dae_lib_effects,
    _s_dae_effect,
    ak_dae_effect,
    offsetof(AkLib, effects),
    offsetof(AkEffect, next)
  },
  {
    NULL,
    _s_dae_lib_images,
    _s_dae_image,
    ak_dae_fxImage,
    offsetof(AkLib, images),
    offsetof(AkImage, next)
  },
  {
    NULL,
    _s_dae_lib_materials,
    _s_dae_material,
    ak_dae_material,
    offsetof(AkLib, materials),
    offsetof(AkMaterial, next)
  },
  {
    NULL,
    _s_dae_lib_geometries,
    _s_dae_geometry,
    ak_dae_geometry,
    offsetof(AkLib, geometries),
    offsetof(AkGeometry, next)
  },
  {
    NULL,
    _s_dae_lib_controllers,
    _s_dae_controller,
    ak_dae_controller,
    offsetof(AkLib, controllers),
    offsetof(AkController, next)
  },
  {
    NULL,
    _s_dae_lib_visual_scenes,
    _s_dae_visual_scene,
    ak_dae_visualScene,
    offsetof(AkLib, visualScenes),
    offsetof(AkVisualScene, next)
  },
  {
    NULL,
    _s_dae_lib_nodes,
    _s_dae_node,
    ak_dae_node2,
    offsetof(AkLib, nodes),
    offsetof(AkNode, next)
  }
};

AkResult
_assetkit_hide
ak_dae_doc(AkDoc ** __restrict dest,
           const char * __restrict file) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkXmlState       xstVal, *xst;

  xmlTextReaderPtr reader;
  int              xmlReaderFlags;

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
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  ak_heap_setdata(heap, doc);

  xstVal.doc      = doc;
  xstVal.heap     = heap;
  xstVal.reader   = reader;
  xstVal.urlQueue = NULL;
  xst             = &xstVal;

  ak_xml_readnext(xst);

  if (daeMapLen == 0) {
    daeMapLen = AK_ARRAY_LEN(daeMap);
    qsort(daeMap,
          daeMapLen,
          sizeof(daeMap[0]),
          ak_enumpair_cmp);
  }

  do {
    const ak_enumpair *found;

    if (ak_xml_beginelm(xst, _s_dae_collada))
      break;

    found = bsearch(xst->nodeName,
                    daeMap,
                    daeMapLen,
                    sizeof(daeMap[0]),
                    ak_enumpair_cmp2);

    if (!found) {
      ak_xml_skipelm(xst);
      goto cont;
    }

    switch (found->val) {
      case k_s_dae_asset: {
        AkAssetInf *assetInf;
        AkDocInf   *docInf;
        AkResult    ret;

        docInf = ak_heap_calloc(heap, doc, sizeof(*docInf));
        assetInf = &docInf->base;

        ret = ak_dae_assetInf(xst, docInf, &assetInf);
        if (ret == AK_OK) {
          docInf->ftype = AK_FILE_TYPE_COLLADA;
          doc->docinf   = *docInf;

          doc->coordSys = assetInf->coordSys;
          doc->unit     = assetInf->unit;
        }

        break;
      }
      case k_s_dae_scene:
        ak_dae_scene(xst, doc, &doc->scene);
        break;
      case k_s_dae_extra: {
        xmlNodePtr nodePtr;
        AkTree   *tree;

        nodePtr = xmlTextReaderExpand(reader);
        tree = NULL;

        ak_tree_fromXmlNode(heap,
                            doc,
                            nodePtr,
                            &tree,
                            NULL);
        doc->extra = tree;

        ak_xml_skipelm(xst);
        break;
      }
      default:
        ak_dae_lib(xst, &libchlds[found->val - 2]);
        break;
    }
  cont:

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  xmlFreeTextReader(reader);

  if (xst->nodeRet == -1) {
    fprintf(stderr, "%s : failed to parse\n", file);
    return AK_ERR;
  }

  *dest = doc;

  /* post-parse operations */
  ak_dae_postscript(xst);

  return AK_OK;
}
