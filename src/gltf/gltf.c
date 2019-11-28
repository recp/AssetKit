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

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#  define le_uint32(X, DATA) \
  do {                                                                        \
    memcpy(&X, DATA, sizeof(uint32_t));                                       \
    DATA = (char *)DATA + sizeof(uint32_t);                                   \
  } while (0)
#else
# include <arpa/inet.h>
AK_INLINE
uint32_t
bswapu32(uint32_t val) {
  
  /*
   val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
   return (val << 16) | (val >> 16);
   */

  #if defined(__llvm__)
    return __builtin_bswap32(val);
  #else
    __asm__ ("bswap   %0" : "+r" (val));
    return val;
  #endif
}
#  define le_uint32(X, DATA)                                                  \
  do {                                                                        \
    memcpy(&X, DATA, sizeof(uint32_t));                                       \
    X    = bswapu32(magic);                                                   \
    X    = ntohl(magic);                                                      \
    DATA = (char *)DATA + sizeof(uint32_t);                                   \
  } while (0)
#endif

static
AkResult _assetkit_hide
gltf_parse(AkDoc     ** __restrict dest,
           const char * __restrict filepath,
           const char * __restrict contents,
           void       * __restrict bindata);

static
void
ak_gltfFreeDupl(RBTree *tree, RBNode *node);

AkResult _assetkit_hide
gltf_glb(AkDoc     ** __restrict dest,
         const char * __restrict filepath) {
  void             *data, *bindata;
  char             *pdata;
  size_t            jsonSize;
  AkResult          ret;
  uint32_t          magic, version, length, chunkLength, chunkType;
  uint32_t          buffLen, buffType;

  if ((ret = ak_readfile(filepath, "rb", &data, &jsonSize)) != AK_OK)
    return ret;

  pdata = data;

  /* check if the is is glTF */
  le_uint32(magic, pdata);
  if (magic != 0x46546C67)
    return AK_ERR;

  le_uint32(version,     pdata);
  le_uint32(length,      pdata);
  le_uint32(chunkLength, pdata);
  le_uint32(chunkType ,  pdata);

  bindata = pdata + chunkLength;

  le_uint32(buffLen,  bindata);
  le_uint32(buffType, bindata);

  if (buffType != 0x004E4942)
    bindata = NULL;
  
  /* make the json NULL terminated */
  pdata[chunkLength] = '\0';

  ret = gltf_parse(dest, filepath, pdata, bindata);

  if (data)
    free(data);

  return ret;
}

AkResult _assetkit_hide
gltf_gltf(AkDoc     ** __restrict dest,
          const char * __restrict filepath) {
  void             *jsonString;
  size_t            jsonSize;
  AkResult          ret;

  if ((ret = ak_readfile(filepath, "rb", &jsonString, &jsonSize)) != AK_OK)
    return ret;

  ret = gltf_parse(dest, filepath, jsonString, NULL);

  if (jsonString)
    free(jsonString);

  return ret;
}

static
AkResult _assetkit_hide
gltf_parse(AkDoc     ** __restrict dest,
           const char * __restrict filepath,
           const char * __restrict contents,
           void       * __restrict bindata) {
  AkHeap           *heap;
  AkDoc            *doc;
  const json_doc_t *gltfRawDoc;
  json_t           *json;
  AkGLTFState       gstVal, *gst;
  AkResult          ret;
  
  if (!contents)
    return AK_ERR;

  ret  = AK_OK;
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
  
  gst            = &gstVal;
  gstVal.doc     = doc;
  gstVal.heap    = heap;
  gstVal.bindata = bindata;
  gst->bufferMap = rb_newtree_ptr();

  gltfRawDoc = json_parse(contents, true);
  if (!gltfRawDoc || !gltfRawDoc->root) {
    ak_free(doc);
    
    if (gltfRawDoc)
      free((void *)gltfRawDoc);

    return AK_ERR;
  }
  
  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);
  
  json = gltfRawDoc->root;
  
  /* json_print_human(stderr, gltfRawDoc->root); */
  
  json_objmap_t gltfMap[] = {
    JSON_OBJMAP_FN(_s_gltf_asset,       gltf_asset,       gst),
    JSON_OBJMAP_FN(_s_gltf_buffers,     gltf_buffers,     gst),
    JSON_OBJMAP_FN(_s_gltf_bufferViews, gltf_bufferViews, gst),
    JSON_OBJMAP_FN(_s_gltf_accessors,   gltf_accessors,   gst),
    JSON_OBJMAP_FN(_s_gltf_images,      gltf_images,      gst),
    JSON_OBJMAP_FN(_s_gltf_samplers,    gltf_samplers,    gst),
    JSON_OBJMAP_FN(_s_gltf_textures,    gltf_textures,    gst),
    JSON_OBJMAP_FN(_s_gltf_materials,   gltf_materials,   gst),
    JSON_OBJMAP_FN(_s_gltf_meshes,      gltf_meshes,      gst),
    JSON_OBJMAP_FN(_s_gltf_cameras,     gltf_cameras,     gst),
    JSON_OBJMAP_FN(_s_gltf_nodes,       gltf_nodes,       gst),
    JSON_OBJMAP_FN(_s_gltf_skins,       gltf_skin,        gst),
    JSON_OBJMAP_FN(_s_gltf_scenes,      gltf_scenes,      gst),
    JSON_OBJMAP_FN(_s_gltf_scene,       gltf_scene,       gst),
    JSON_OBJMAP_FN(_s_gltf_animations,  gltf_animations,  gst)
  };
  
  while (json) {
    json_objmap_call(json, gltfMap, JSON_ARR_LEN(gltfMap), &gstVal.stop);
    
    if (gstVal.stop) {
      ret = AK_EBADF;
      goto err;
    }
    
    json = json->next;
  }
  
err:
  
  if (gltfRawDoc)
    free((void *)gltfRawDoc);
  
  /* probably unsupportted version or verion is missing */
  if (ret == AK_EBADF) {
    ak_free(doc);
    return ret;
  }
  
  gst->meshTargets = rb_newtree(ds_allocator(), ds_cmp_ptr,  NULL);

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
  
  rb_destroy(gst->meshTargets);
  
  /* post-parse operations */
  gltf_postscript(gst);

  return ret;
}

static
void
ak_gltfFreeDupl(RBTree *tree, RBNode *node) {
  if (node == tree->nullNode)
    return;
  ak_free(node->val);
}
