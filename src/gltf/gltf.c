/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "gltf.h"
#include "../id.h"
#include "../../include/ak/path.h"
#include "core/gltf_asset.h"
#include "core/gltf_buffer.h"
#include "core/gltf_accessor.h"
#include "core/gltf_mesh.h"
#include "core/gltf_node.h"
#include "core/gltf_scene.h"
#include "gltf_postscript.h"
#include "core/gltf_camera.h"
#include "core/gltf_image.h"
#include "core/gltf_profile.h"
#include "core/gltf_sampler.h"
#include "core/gltf_texture.h"
#include "core/gltf_material.h"
#include "core/gltf_anim.h"
#include "core/gltf_skin.h"

#include <json/json.h>
#include <json/print.h>

#define k_s_gltf_asset       1
#define k_s_gltf_buffers     2
#define k_s_gltf_bufferviews 3
#define k_s_gltf_accessors   4
#define k_s_gltf_images      5
#define k_s_gltf_materials   6
#define k_s_gltf_meshes      7
#define k_s_gltf_cameras     8
#define k_s_gltf_nodes       9
#define k_s_gltf_scenes      10
#define k_s_gltf_animations  11
#define k_s_gltf_skins       12

static ak_enumpair gltfMap[] = {
  {_s_gltf_asset,       k_s_gltf_asset},
  {_s_gltf_buffers,     k_s_gltf_buffers},
  {_s_gltf_bufferViews, k_s_gltf_bufferviews},
  {_s_gltf_accessors,   k_s_gltf_accessors},
  {_s_gltf_images,      k_s_gltf_images},
  {_s_gltf_materials,   k_s_gltf_materials},
  {_s_gltf_meshes,      k_s_gltf_meshes},
  {_s_gltf_cameras,     k_s_gltf_cameras},
  {_s_gltf_nodes,       k_s_gltf_nodes},
  {_s_gltf_scenes,      k_s_gltf_scenes},
  {_s_gltf_animations,  k_s_gltf_animations},
  {_s_gltf_skins,       k_s_gltf_skins}
};

static size_t gltfMapLen = 0;

static
void
ak_gltfFreeDupl(RBTree *tree, RBNode *node) {
  if (node == tree->nullNode)
    return;
  ak_free(node->val);
}

AkResult _assetkit_hide
gltf_doc(AkDoc     ** __restrict dest,
         const char * __restrict filepath) {
  AkHeap           *heap;
  AkDoc            *doc;
  AkAssetInf       *assetInf;
  json_t           *jscene;
  AkVisualScene    *scene;
  const json_doc_t *gltfRawDoc;
  const json_t     *json;
  void             *jsonString;
  size_t            jsonSize;
  AkGLTFState       gstVal, *gst;
  AkResult          ret;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf        = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name  = filepath;
  doc->inf->dir   = ak_path_dir(heap, doc, filepath);
  doc->inf->ftype = AK_FILE_TYPE_GLTF;

  /* for fixing skin and morph vertices */
  doc->reserved = rb_newtree_ptr();
  ((RBTree *)doc->reserved)->onFreeNode = ak_gltfFreeDupl;

  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);

  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  memset(&gstVal, 0, sizeof(gstVal));

  gst         = &gstVal;
  gstVal.doc  = doc;
  gstVal.heap = heap;

  if ((ret = ak_readfile(filepath, "rb", &jsonString, &jsonSize)) != AK_OK)
    return ret;

  gltfRawDoc = json_parse(jsonString);
  if (!gltfRawDoc || !gltfRawDoc->root)
    return AK_ERR;

  if (gltfMapLen == 0) {
    gltfMapLen = AK_ARRAY_LEN(gltfMap);
    qsort(gltfMap, gltfMapLen, sizeof(gltfMap[0]), ak_enumpair_cmp);
  }

  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
  doc->inf->name      = filepath;
  doc->inf->flipImage = true;
  doc->inf->ftype     = AK_FILE_TYPE_GLTF;

  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);
  
  json = gltfRawDoc->root->value;

  json_print(json);

  while (json) {
    const ak_enumpair *found;

    if (!(found = bsearch(json,
                          gltfMap,
                          gltfMapLen,
                          sizeof(gltfMap[0]),
                          ak_enumpair_json_key_cmp)))
      goto cont;

    switch (found->val) {
      case k_s_gltf_asset: {
        assetInf = &doc->inf->base;
        ret = gltf_asset(gst, doc, json->value, assetInf);

        if (ret == AK_OK) {
          doc->coordSys = assetInf->coordSys;
          doc->unit     = assetInf->unit;
        }

        break;
      }

      case k_s_gltf_buffers:
        gltf_buffers(gst, json);
        break;

      case k_s_gltf_bufferviews:
        gltf_bufferViews(gst, json);
        break;

      case k_s_gltf_accessors:
        gltf_accessors(gst, json);
        break;
    }
  cont:
    json = json->next;
  }
  
 // gstVal.root = json_load_file(filepath, 0, &error);

//  ainf = NULL;
//  ret  = gltf_asset(gst, doc, json, ainf);

  /* probably unsupportted version or verion is missing */
  if (ret == AK_EBADF) {
    ak_free(doc);
    return ret;
  }

  gst->bufferViews = rb_newtree(ds_allocator(), ds_cmp_i32p, NULL);
  gst->meshTargets = rb_newtree(ds_allocator(), ds_cmp_ptr,  NULL);

  // gltf_buffers(gst);
//  gltf_images(gst);
//  gltf_materials(gst);
//  gltf_meshes(gst);
//  gltf_cameras(gst);
//  gltf_nodes(gst);
//  gltf_scenes(gst);
//  gltf_animations(gst);
//  gltf_skin(gst);

  /* TODO: release resources in GLTFState */

  /* set first scene as default scene if not specified  */
//  scene = doc->lib.visualScenes->chld;

//  /* set default scene */
//  if ((jscene = json_object_get(gst->root, _s_gltf_scene))) {
//    int32_t sceneIndex;
//    sceneIndex = (int32_t)json_integer_value(jscene);
//    while (sceneIndex > 0 && scene) {
//      scene = scene->next;
//      sceneIndex--;
//    }
//  }

//  if (scene) {
//    AkInstanceBase *instScene;
//    instScene = ak_heap_calloc(heap, doc, sizeof(*instScene));
//
//    instScene->url.ptr = scene;
//    doc->scene.visualScene = instScene;
//  }

  *dest = doc;

  rb_destroy(gst->bufferViews);
  rb_destroy(gst->meshTargets);

  /* post-parse operations */
  gltf_postscript(gst);

  return AK_OK;
}
