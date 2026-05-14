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

#include "../../../../include/ak/path.h"
#include "../../../id.h"
#include "../../../endian.h"
#include "gltf.h"
#include "core/asset.h"
#include "core/buffer.h"
#include "core/accessor.h"
#include "core/mesh.h"
#include "core/node.h"
#include "core/scene.h"
#include "core/camera.h"
#include "core/image.h"
#include "core/profile.h"
#include "core/sampler.h"
#include "core/texture.h"
#include "core/material.h"
#include "core/anim.h"
#include "core/skin.h"
#include "core/ext.h"
#include "extra.h"
#include "postscript.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static
AkResult
gltf_parse(AkDoc     ** __restrict dest,
           const char * __restrict filepath,
           const char * __restrict contents,
           size_t                  contentsLen,
           void       * __restrict bindata,
           size_t                  bindataLen);

static
void
ak_gltfFreeDupl(RBTree *tree, RBNode *node);

static
bool
gltf_profile_enabled(void);

static
double
gltf_profile_now_ms(void);

static
void
gltf_profile_log(const char *name, double elapsed);

AK_HIDE
AkResult
gltf_glb(AkDoc     ** __restrict dest,
         const char * __restrict filepath) {
  void             *data, *bindata;
  char             *pdata, *jsonData, *binData;
  size_t            fileSize, bindataLen;
  AkResult          ret;
  uint32_t          magic, version, length, chunkLength, chunkType;
  uint32_t          buffLen, buffType;

  if ((ret = ak_readfile(filepath, NULL, &data, &fileSize)) != AK_OK)
    return ret;

  pdata = data;
  bindata = NULL;
  bindataLen = 0;

  /* check if the is is glTF */
  le_32(magic, pdata);
  if (magic != 0x46546C67) {
    ret = AK_ERR;
    goto done;
  }

  le_32(version,     pdata);
  le_32(length,      pdata);
  le_32(chunkLength, pdata);
  le_32(chunkType ,  pdata);

  if (chunkType != 0x4E4F534A
      || length > fileSize
      || chunkLength > fileSize
      || (size_t)(pdata - (char *)data) > fileSize - chunkLength) {
    ret = AK_ERR;
    goto done;
  }

  jsonData = pdata;
  binData  = jsonData + chunkLength;

  if ((size_t)(binData - (char *)data) <= fileSize - 8) {
    le_32(buffLen,  binData);
    le_32(buffType, binData);

    if (buffType == 0x004E4942
        && buffLen <= fileSize
        && (size_t)(binData - (char *)data) <= fileSize - buffLen) {
      bindata    = binData;
      bindataLen = buffLen;
    }
  }

  /* make the json NULL terminated */
  /*
   pdata[chunkLength] = '\0';
   */

  ret = gltf_parse(dest, filepath, jsonData, chunkLength, bindata, bindataLen);

done:
  if (data) {
    if (ret == AK_OK && dest && *dest && bindata && ak_opt_get(AK_OPT_USE_MMAP))
      ak_mmap_attach(*dest, data, fileSize);
    else
      ak_releasefile(data, fileSize);
  }

  return ret;
}

AK_HIDE
AkResult
gltf_gltf(AkDoc     ** __restrict dest,
          const char * __restrict filepath) {
  void             *jsonString;
  size_t            jsonSize;
  AkResult          ret;

  if ((ret = ak_readfile(filepath, NULL, &jsonString, &jsonSize)) != AK_OK)
    return ret;

  ret = gltf_parse(dest, filepath, jsonString, jsonSize, NULL, 0);

  if (jsonString)
    ak_releasefile(jsonString, jsonSize);

  return ret;
}

static
AkResult
gltf_parse(AkDoc     ** __restrict dest,
           const char * __restrict filepath,
           const char * __restrict contents,
           size_t                  contentsLen,
           void       * __restrict bindata,
           size_t                  bindataLen) {
  AkHeap           *heap;
  AkDoc            *doc;
  json_doc_t       *gltfRawDoc;
  json_t           *json;
  AkGLTFState       gstVal, *gst;
  AkResult          ret;
  double            profStart, profSection;
  bool              prof;
  
  if (!contents || contentsLen == 0)
    return AK_ERR;

  ret  = AK_OK;
  prof = gltf_profile_enabled();
  profStart = profSection = prof ? gltf_profile_now_ms() : 0.0;
  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  
  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
  doc->inf->name      = filepath;
  doc->inf->flipImage = false;
  doc->inf->ftype     = AK_FILE_TYPE_GLTF;
  
  /* for fixing skin and morph vertices */
  doc->reserved = rb_newtree_ptr();
  ((RBTree *)doc->reserved)->onFreeNode = ak_gltfFreeDupl;
  
  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);
  
  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);
  
  memset(&gstVal, 0, sizeof(gstVal));
  
  gst              = &gstVal;
  gstVal.doc       = doc;
  gstVal.heap      = heap;
  gstVal.bindata   = bindata;
  gstVal.bindataLen = bindataLen;
  gstVal.borrowBufferViews = (bindata ? ak_opt_get(AK_OPT_USE_MMAP) : true)
                              && (AkCoordCvtType)ak_opt_get(AK_OPT_COORD_CONVERT_TYPE)
                                 != AK_COORD_CVT_ALL;
  gstVal.tmpParent = ak_heap_alloc(heap, doc, sizeof(void*));
  gst->bufferMap   = rb_newtree_ptr();
  gst->meshTargets = rb_newtree_ptr();
  gst->skinBound   = rb_newtree_ptr();
  gst->skinBound->userData = gst;
  
  profSection = prof ? gltf_profile_now_ms() : 0.0;
  gltfRawDoc = json_parse_len(contents, contentsLen, true);
  if (prof)
    gltf_profile_log("json_parse", gltf_profile_now_ms() - profSection);
  if (!gltfRawDoc || !gltfRawDoc->root) {
    ak_free(doc);

    if (gltfRawDoc)
      json_free(gltfRawDoc);
    return AK_ERR;
  }

  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);

  json = gltfRawDoc->root;
  profSection = prof ? gltf_profile_now_ms() : 0.0;
  gltf_extra(gst,
             doc,
             GLTF_JSON_GET8(json, extras),
             GLTF_JSON_GET(json, extensions));
  gltf_extra_node(gst,
                  doc,
                  _s_gltf_extensionsUsed,
                  GLTF_JSON_GET(json, extensionsUsed));
  gltf_extra_node(gst,
                  doc,
                  _s_gltf_extensionsRequired,
                  GLTF_JSON_GET(json, extensionsRequired));
  gltf_ext_root(GLTF_JSON_GET(json, extensions), gst);
  if (prof)
    gltf_profile_log("root_extra", gltf_profile_now_ms() - profSection);

  /* json_print_human(stderr, gltfRawDoc->root); */

  json_objmap_t gltfMap[] = {
    GLTF_JSON_OBJMAP_FN8(asset,         gltf_asset,       gst),
    GLTF_JSON_OBJMAP_FN8(buffers,       gltf_buffers,     gst),
    JSON_OBJMAP_FN(_s_gltf_bufferViews, gltf_bufferViews, gst),
    JSON_OBJMAP_FN(_s_gltf_accessors,   gltf_accessors,   gst),
    GLTF_JSON_OBJMAP_FN8(images,        gltf_images,      gst),
    GLTF_JSON_OBJMAP_FN8(samplers,      gltf_samplers,    gst),
    GLTF_JSON_OBJMAP_FN8(textures,      gltf_textures,    gst),
    JSON_OBJMAP_FN(_s_gltf_extensionsRequired, gltf_exts,  gst),
    JSON_OBJMAP_FN(_s_gltf_materials,   gltf_materials,   gst),
    GLTF_JSON_OBJMAP_FN8(meshes,        gltf_meshes,      gst),
    GLTF_JSON_OBJMAP_FN8(cameras,       gltf_cameras,     gst),
    GLTF_JSON_OBJMAP_FN8(nodes,         gltf_nodes,       gst),
    GLTF_JSON_OBJMAP_FN8(skins,         gltf_skin,        gst),
    GLTF_JSON_OBJMAP_FN8(scenes,        gltf_scenes,      gst),
    GLTF_JSON_OBJMAP_FN8(scene,         gltf_scene,       gst),
    JSON_OBJMAP_FN(_s_gltf_animations,  gltf_animations,  gst)
  };

  while (json) {
    size_t i;

    profSection = prof ? gltf_profile_now_ms() : 0.0;
    json_objmap(json, gltfMap, JSON_ARR_LEN(gltfMap));
    if (prof)
      gltf_profile_log("root_objmap", gltf_profile_now_ms() - profSection);

    for (i = 0; i < JSON_ARR_LEN(gltfMap); i++) {
      json_objmap_t *item;

      if (gstVal.stop)
        break;

      item = &gltfMap[i];
      if (item->object) {
        if (item->foundFunc.func) {
          profSection = prof ? gltf_profile_now_ms() : 0.0;
          item->foundFunc.func(item->object, item->foundFunc.param);
          if (prof)
            gltf_profile_log(item->key, gltf_profile_now_ms() - profSection);
        }
      } else if (item->notFoundFunc.func) {
        item->notFoundFunc.func(item->object, item->notFoundFunc.param);
      }
    }
    
    if (gstVal.stop) {
      ret = AK_EBADF;
      goto err;
    }
    
    json = json->next;
  }

err:

  if (gltfRawDoc) {
    profSection = prof ? gltf_profile_now_ms() : 0.0;
    json_free(gltfRawDoc);
    if (prof)
      gltf_profile_log("json_free", gltf_profile_now_ms() - profSection);
  }

  gltf_ext_close(gst);

  /* probably unsupportted version or verion is missing */
  if (ret == AK_EBADF) {
    ak_free(doc);
    return ret;
  }


  /* TODO: release resources in GLTFState */
  
  /* set first scene as default scene if not specified  */
  if (!doc->scene.visualScene) {
    if (doc->lib.visualScenes->chld) {
      AkInstanceBase *instScene;
      instScene = ak_heap_calloc(heap, doc, sizeof(*instScene));
      
      instScene->url.ptr     = doc->lib.visualScenes->chld;
      doc->scene.visualScene = instScene;
    }
  }

  *dest = doc;

  /* post-parse operations */
  profSection = prof ? gltf_profile_now_ms() : 0.0;
  gltf_postscript(gst);
  if (prof) {
    gltf_profile_log("postscript", gltf_profile_now_ms() - profSection);
    gltf_profile_log("total", gltf_profile_now_ms() - profStart);
  }
  
  ak_free(gstVal.tmpParent);

  rb_destroy(gst->bufferMap);
  rb_destroy(gst->meshTargets);
  rb_destroy(gst->skinBound);

  return ret;
}

static
void
ak_gltfFreeDupl(RBTree *tree, RBNode *node) {
  if (node == tree->nullNode)
    return;
  ak_free(node->val);
}

static
bool
gltf_profile_enabled(void) {
  const char *value;

  value = getenv("ASSETKIT_GLTF_PROFILE");
  if (!value || !value[0])
    value = getenv("ASSETKIT_BLENDER_PROFILE");

  return value && value[0] && value[0] != '0';
}

static
double
gltf_profile_now_ms(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static
void
gltf_profile_log(const char *name, double elapsed) {
  fprintf(stderr, "[AssetKit glTF] %s=%.3fms\n", name, elapsed);
}
