/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "3mf.h"
#include "../../../miniz/miniz.h"

AK_HIDE
AkResult
imp_3mf(AkDoc ** __restrict dest, const char * __restrict filepath) {
//  AkHeap          *heap;
//  AkDoc           *doc;
//  const xml_doc_t *xdoc;
//  xml_t           *xml, *assetEl;
//  AkAssetInf      *inf;
//  xml_attr_t      *versionAttr;
//  void            *xmlString, *zipped;
//  AkLibraries     *libs;
//  FListItem       *freeUsrData;
//  _3MFState        dstVal, *dst;
//  size_t           xmlSize, zipSize;
//  AkResult         ret;
//
//  if ((ret = ak_readfile(filepath, NULL, &zipped, &zipSize)) != AK_OK)
//    return ret;
//
//  /* if we know uncompressed fixed size, otherwise use chunked uncompress and merge it */
//  xmlSize   = zipSize * 4;
//  xmlString = malloc(xmlSize); /* TODO: enough size? */
//
//  /* TODO: optimize memory */
//  /* unzip */
//  if (mz_uncompress(xmlString, &xmlSize, zipped, zipSize) != MZ_OK) {
//    goto err;
//  }
//
//  if (mz_zip_reader_validate_mem(xmlString, xmlSize)) {
//    mz_zip_archive zip;
//    mz_zip_zero_struct(&zip);
//    if (!mz_zip_reader_init_mem(&zip, xmlString, xmlSize, 0)) {
//      free(xmlString);
//      return AK_ERR_IO;
//    }
//
//    if (mz_zip_reader_is_file_a_directory(&zip, "3D/3dmodel.model", 0)) {
//      mz_zip_reader_end(&zip);
//      free(xmlString);
//      return AK_ERR_IO;
//    }
//
//    xmlSize = mz_zip_reader_get_file_stat(&zip, "3D/3dmodel.model")->m_uncomp_size;
//    xmlString = realloc(xmlString, xmlSize);
//    if (!mz_zip_reader_extract_file_to_mem(&zip,
//                                           "3D/3dmodel.model",
//                                           xmlString,
//                                           xmlSize,
//                                           0)) {
//      mz_zip_reader_end(&zip);
//      free(xmlString);
//      return AK_ERR_IO;
//    }
//
//    mz_zip_reader_end(&zip);
//  }
//
//  xdoc = xml_parse(xmlString, XML_PREFIXES | XML_READONLY);
//  if (!xdoc || !(xml = xdoc->root)) {
//    if (xdoc) { free((void *)xdoc); }
//    ak_releasefile(xmlString, xmlSize);
//    return AK_ERR;
//  }
//
//  heap = ak_heap_new(NULL, NULL, NULL);
//  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
//
//  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
//  doc->inf->name      = filepath;
//  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
//  doc->inf->flipImage = true;
//  doc->inf->ftype     = AK_FILE_TYPE_COLLADA;
//  doc->coordSys       = AK_YUP; /* Default */
//
//  /* for fixing skin and morph vertices */
//  doc->reserved = rb_newtree_ptr();
//  ((RBTree *)doc->reserved)->onFreeNode = ak_daeFreeDupl;
//
//  if (doc->inf->dir)
//    doc->inf->dirlen = strlen(doc->inf->dir);
//
//  ak_heap_setdata(heap, doc);
//  ak_id_newheap(heap);
//
//  memset(&dstVal, 0, sizeof(dstVal));
//
//  dstVal.doc          = doc;
//  dstVal.heap         = heap;
//  dstVal.tempmem      = ak_heap_alloc(heap, doc, sizeof(void*));
//  dstVal.meshInfo     = rb_newtree_ptr();
//  dstVal.inputmap     = rb_newtree_ptr();
//  dstVal.texmap       = rb_newtree_ptr();
//  dstVal.instanceMap  = rb_newtree_ptr();
//
//  dstVal.ctlrSkinMap  = rb_newtree_ptr();
//  dstVal.ctlrMorphMap = rb_newtree_ptr();
//
//  dst                 = &dstVal;
//
//  dstVal.texmap->userData = dst;
//
//  /* get version info */
//  /* because it is current and most used version */
//  dst->version = AK_COLLADA_VERSION_141;
//  if ((versionAttr = xmla(xml, _s_dae_version))) {
//    ak_enumpair *v;
//
//    for (v = daeVersions; v->key; v++) {
//      if (!strncmp(v->key, versionAttr->val, versionAttr->valsize)) {
//        dst->version = v->val;
//        break;
//      }
//    }
//  }
//
//  libs    = &doc->lib;
//  assetEl = NULL;
//  xml     = xml->val;
//
//  /* with default Asset Parameters */
//  assetEl = xml_elem(xml->parent, _s_dae_asset);
//  if ((inf = dae_asset(dst, assetEl, doc, &doc->inf->base))) {
//    doc->coordSys = inf->coordSys;
//    doc->unit     = inf->unit;
//  }
//
//  while (xml) {
//    if (xml_tag_eq(xml, _s_dae_lib_cameras)) {
//      dae_lib(dst, xml, _s_dae_camera, dae_cam, &libs->cameras);
//    } else if (xml_tag_eq(xml, _s_dae_lib_lights)) {
//      dae_lib(dst, xml, _s_dae_light, dae_light, &libs->lights);
//    } else if (xml_tag_eq(xml, _s_dae_lib_geometries)) {
//      dae_lib(dst, xml, _s_dae_geometry, dae_geom, &libs->geometries);
//    } else if (xml_tag_eq(xml, _s_dae_lib_effects)) {
//      dae_lib(dst, xml, _s_dae_effect, dae_effect, &libs->effects);
//    } else if (xml_tag_eq(xml, _s_dae_lib_images)) {
//      dae_lib(dst, xml, _s_dae_image, dae_image, &libs->libimages);
//    } else if (xml_tag_eq(xml, _s_dae_lib_materials)) {
//      dae_lib(dst, xml, _s_dae_material, dae_material, &libs->materials);
//    } else if (xml_tag_eq(xml, _s_dae_lib_controllers)) {
//      dae_lib(dst, xml, _s_dae_controller, dae_ctlr, &libs->controllers);
//    } else if (xml_tag_eq(xml, _s_dae_lib_visual_scenes)) {
//      dae_lib(dst, xml, _s_dae_visual_scene, dae_vscene, &libs->visualScenes);
//    } else if (xml_tag_eq(xml, _s_dae_lib_nodes)) {
//      dae_lib(dst, xml, _s_dae_node, dae_node2, &libs->nodes);
//    } else if (xml_tag_eq(xml, _s_dae_lib_animations)) {
//      dae_lib(dst, xml, _s_dae_animation, dae_anim, &libs->animations);
//    } else if (xml_tag_eq(xml, _s_dae_scene)) {
//      dae_scene(dst, xml);
//    }
//    xml = xml->next;
//  }
//
//  *dest = doc;
//
//  /* post-parse operations */
//  dae_postscript(dst);
//
//  /* cleanup up details */
//  freeUsrData = dst->linkedUserData;
//  while (freeUsrData) {
//    void *tofree;
//
//    if ((tofree = ak_userData(freeUsrData->data)))
//      ak_free(tofree);
//
//    ak_heap_ext_rm(heap, ak__alignof(freeUsrData->data), AK_HEAP_NODE_FLAGS_USR);
//    freeUsrData = freeUsrData->next;
//  }
//
//  ak_free(dstVal.tempmem);
//
//  flist_sp_destroy(&dst->linkedUserData);
//
//  rb_destroy(dstVal.meshInfo);
//  rb_destroy(dstVal.inputmap);
//  rb_destroy(dstVal.texmap);
//  rb_destroy(dstVal.instanceMap);
//
//  flist_sp_destroy(&dstVal.vertMap);
//
//  rb_destroy(dstVal.ctlrSkinMap);
//  rb_destroy(dstVal.ctlrMorphMap);
//
//  if (xdoc)
//    free((void *)xdoc);
//
//  if (xmlString)
//    ak_releasefile(xmlString, xmlSize);
//
//  /* TODO: memory leak, free this RBTree*/
//  /* rb_destroy(doc->reserved); */
//
//  return AK_OK;
//
//err:
  return AK_EBADF;
}
