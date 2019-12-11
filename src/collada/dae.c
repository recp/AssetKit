/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae.h"
#include "dae_common.h"

#include "core/dae_asset.h"
#include "core/dae_camera.h"
#include "core/dae_light.h"
#include "core/dae_geometry.h"
#include "core/dae_controller.h"
#include "core/dae_visual_scene.h"
#include "core/dae_node.h"
#include "core/dae_scene.h"
#include "core/dae_anim.h"

#include "fx/dae_fx_effect.h"
#include "fx/dae_fx_image.h"
#include "fx/dae_fx_material.h"

#include "dae_lib.h"
#include "dae_postscript.h"
#include "../id.h"

#include "../../include/ak/path.h"

static ak_enumpair daeVersions[] = {
  {"1.5.0",             AK_COLLADA_VERSION_150},
  {"1.5",               AK_COLLADA_VERSION_150},
  {"1.4.1",             AK_COLLADA_VERSION_141},
  {"1.4.0",             AK_COLLADA_VERSION_140},
  {"1.4",               AK_COLLADA_VERSION_140},
  {NULL, 0}
};

static
void
ak_daeFreeDupl(RBTree *tree, RBNode *node);

AkResult
_assetkit_hide
dae_doc(AkDoc     ** __restrict dest,
        const char * __restrict filepath) {
  AkHeap            *heap;
  AkDoc             *doc;
  const xml_doc_t   *xdoc;
  xml_t             *xml;
  AkXmlState         dstVal, *dst;
  xml_attr_t        *versionAttr;
  void              *xmlString;
  size_t             xmlSize;
  AkResult           ret;

  if ((ret = ak_readfile(filepath, "rb", &xmlString, &xmlSize)) != AK_OK)
    return ret;

  xdoc = xml_parse(xmlString, true, false);
  if (!xdoc || !(xml = xdoc->root)) {
    if (xdoc)
      free((void *)xdoc);
    return AK_ERR;
  }

  ret  = AK_OK;
  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name      = filepath;
  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage = true;
  doc->inf->ftype     = AK_FILE_TYPE_COLLADA;

  /* for fixing skin and morph vertices */
  doc->reserved = rb_newtree_ptr();
  ((RBTree *)doc->reserved)->onFreeNode = ak_daeFreeDupl;

  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);

  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  memset(&dstVal, 0, sizeof(dstVal));

  dstVal.doc         = doc;
  dstVal.heap        = heap;
  dstVal.meshInfo    = rb_newtree_ptr();
  dstVal.inputmap    = rb_newtree_ptr();
  dstVal.texmap      = rb_newtree_ptr();
  dstVal.instanceMap = rb_newtree_ptr();
  dst                = &dstVal;

  dstVal.texmap->userData = dst;

  
  /* get version info */
  /* because it is current and most used version */
  dst->version = AK_COLLADA_VERSION_141;
  if ((versionAttr = xml_attr(xml, _s_dae_version))) {
    ak_enumpair *v;
    
    for (v = daeVersions; v->key; v++) {
      if (!strncmp(v->key, versionAttr->val, versionAttr->valsize)) {
        dst->version = v->val;
        break;
      }
    }
  }

  xml_objmap_t daemap[] = {
    XML_OBJMAP_FN(_s_dae_asset,       dae_asset,        dst),
    XML_OBJMAP_FN(_s_dae_lib_cameras,       dae_asset,        dst),
    XML_OBJMAP_FN(_s_dae_lib_lights,        dae_asset,     dst),
    XML_OBJMAP_FN(_s_dae_lib_effects,       dae_asset, dst),
    XML_OBJMAP_FN(_s_dae_lib_images,        dae_asset,   dst),
    XML_OBJMAP_FN(_s_dae_lib_materials,     dae_asset,      dst),
    XML_OBJMAP_FN(_s_dae_lib_geometries,    dae_asset, dst),
    XML_OBJMAP_FN(_s_dae_lib_controllers,   dae_asset,    dst),
    XML_OBJMAP_FN(_s_dae_lib_visual_scenes, dae_asset,   dst),
    XML_OBJMAP_FN(_s_dae_lib_nodes,         dae_asset,      dst),
    XML_OBJMAP_FN(_s_dae_lib_animations,    dae_asset,     dst),
    XML_OBJMAP_FN(_s_dae_scene,    dae_asset,     dst),
    XML_OBJMAP_FN(_s_dae_extra,    dae_asset,     dst)
  };
  
  *dest = doc;

  /* post-parse operations */
  dae_postscript(dst);

  /* TODO: memory leak, free this RBTree*/
  /* rb_destroy(doc->reserved); */

  return AK_OK;
}

static
void
ak_daeFreeDupl(RBTree *tree, RBNode *node) {
  if (node == tree->nullNode)
    return;
  ak_free(node->val);
}
