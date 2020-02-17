/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae.h"
#include "common.h"

#include "core/asset.h"
#include "core/cam.h"
#include "core/light.h"
#include "core/geom.h"
#include "core/ctlr.h"
#include "core/node.h"
#include "core/scene.h"
#include "core/anim.h"

#include "fx/effect.h"
#include "fx/img.h"
#include "fx/mat.h"

#include "postscript.h"
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

typedef void*(*AkLoadLibraryItemFn)(DAEState * __restrict dst,
                                    xml_t    * __restrict xml,
                                    void     * __restrict memp);
static void ak_daeFreeDupl(RBTree *, RBNode *);

static
_assetkit_hide
void
dae_lib(DAEState   * __restrict dst,
        xml_t      * __restrict xml,
        const char * __restrict name,
        AkLoadLibraryItemFn     loadfn,
        AkLibrary ** __restrict dest);

AkResult
_assetkit_hide
dae_doc(AkDoc     ** __restrict dest,
        const char * __restrict filepath) {
  AkHeap            *heap;
  AkDoc             *doc;
  const xml_doc_t   *xdoc;
  xml_t             *xml, *xlib;
  DAEState           dstVal, *dst;
  xml_attr_t        *versionAttr;
  void              *xmlString;
  AkLibraries       *libs;
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
  if ((versionAttr = xmla(xml, _s_dae_version))) {
    ak_enumpair *v;

    for (v = daeVersions; v->key; v++) {
      if (!strncmp(v->key, versionAttr->val, versionAttr->valsize)) {
        dst->version = v->val;
        break;
      }
    }
  }
  
  libs = &doc->lib;

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      AkAssetInf *inf;
      AkDocInf   *docInf;

      docInf = ak_heap_calloc(heap, doc, sizeof(*docInf));
      inf    = dae_asset(dst, xml, &docInf->base, &docInf->base);

      doc->coordSys = inf->coordSys;
      doc->unit     = inf->unit;

      doc->inf      = docInf;
    } else if (xml_tag_eq(xml, _s_dae_lib_cameras) && (xlib = xml->val)) {
      dae_lib(dst, xml, _s_dae_camera, dae_cam, &libs->cameras);
    } else if (xml_tag_eq(xml, _s_dae_lib_lights)) {
      dae_lib(dst, xml, _s_dae_light, dae_light, &libs->lights);
    } else if (xml_tag_eq(xml, _s_dae_lib_geometries)) {
      dae_lib(dst, xml, _s_dae_geometry, dae_geom, &libs->geometries);
    } else if (xml_tag_eq(xml, _s_dae_lib_effects)) {
      dae_lib(dst, xml, _s_dae_effect, dae_effect, &libs->effects);
    } else if (xml_tag_eq(xml, _s_dae_lib_images)) {
      dae_lib(dst, xml, _s_dae_image, dae_image, &libs->libimages);
    } else if (xml_tag_eq(xml, _s_dae_lib_materials)) {
      dae_lib(dst, xml, _s_dae_material, dae_material, &libs->materials);
    } else if (xml_tag_eq(xml, _s_dae_lib_controllers)) {
      dae_lib(dst, xml, _s_dae_controller, dae_ctlr, &libs->controllers);
    } else if (xml_tag_eq(xml, _s_dae_lib_visual_scenes)) {
      dae_lib(dst, xml, _s_dae_visual_scene, dae_vscene, &libs->visualScenes);
    } else if (xml_tag_eq(xml, _s_dae_lib_nodes)) {
      dae_lib(dst, xml, _s_dae_node, dae_node2, &libs->nodes);
    } else if (xml_tag_eq(xml, _s_dae_lib_animations)) {
      dae_lib(dst, xml, _s_dae_animation, dae_anim, &libs->animations);
    } else if (xml_tag_eq(xml, _s_dae_scene)) {
      dae_scene(dst, xml);
    }
    xml = xml->next;
  }

  *dest = doc;

  /* post-parse operations */
  dae_postscript(dst);

  if (xdoc)
    free((void *)xdoc);

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

static
_assetkit_hide
void
dae_lib(DAEState   * __restrict dst,
        xml_t      * __restrict xml,
        const char * __restrict name,
        AkLoadLibraryItemFn     loadfn,
        AkLibrary ** __restrict dest) {
  AkHeap           *heap;
  AkDoc            *doc;
  AkLibrary        *lib;
  AkOneWayIterBase *it;
  
  heap      = dst->heap;
  doc       = dst->doc;

  lib       = ak_heap_calloc(heap, doc, sizeof(*lib));
  lib->name = xmla_strdup_by(xml, heap, _s_dae_name, lib);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, name)) {
      if ((it = loadfn(dst, xml, lib))) {
        it->next  = lib->chld;
        lib->chld = it;
        lib->count++;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      lib->extra = tree_fromxml(heap, lib, xml);
    }
    xml = xml->next;
  }

  lib->next = *dest;
  *dest     = lib;
}
