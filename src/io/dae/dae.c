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
#include "../../id.h"

#include "../../../include/ak/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
bool
dae_profile_enabled(void) {
  const char *value;

  value = getenv("ASSETKIT_DAE_PROFILE");
  if (!value)
    value = getenv("ASSETKIT_BLENDER_PROFILE");

  return value && value[0] && value[0] != '0';
}

static
double
dae_profile_now_ms(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static
void
dae_profile_log(bool enabled, const char *name, double start) {
  if (!enabled)
    return;

  fprintf(stderr,
          "[AssetKit DAE] %s=%.3fms\n",
          name,
          dae_profile_now_ms() - start);
}

static
void
dae_profile_log_geometry(DAEState * __restrict dst) {
  if (!dst || !dst->profile)
    return;

  fprintf(stderr,
          "[AssetKit DAE geom] geometry=%.3fms/%u mesh=%.3fms/%u "
          "source=%.3fms/%u accessor=%.3fms/%u array=%.3fms/%u "
          "input=%.3fms/%u index_array=%.3fms/%u vertices=%.3fms/%u "
          "tri=%.3fms/%u poly=%.3fms/%u line=%.3fms/%u\n",
          dst->profGeom,
          dst->profGeomCount,
          dst->profGeomMesh,
          dst->profGeomMeshCount,
          dst->profGeomSource,
          dst->profGeomSourceCount,
          dst->profGeomAccessor,
          dst->profGeomAccessorCount,
          dst->profGeomArray,
          dst->profGeomArrayCount,
          dst->profGeomInput,
          dst->profGeomInputCount,
          dst->profGeomIndexArray,
          dst->profGeomIndexArrayCount,
          dst->profGeomVertices,
          dst->profGeomVerticesCount,
          dst->profGeomTriangles,
          dst->profGeomTrianglesCount,
          dst->profGeomPolygons,
          dst->profGeomPolygonsCount,
          dst->profGeomLines,
          dst->profGeomLinesCount);
}

#define DAE_PROFILE_CALL(PROFILE, NAME, CALL) do {                         \
    double _dae_profile_call_start = 0.0;                                   \
    if (PROFILE)                                                            \
      _dae_profile_call_start = dae_profile_now_ms();                       \
    CALL;                                                                   \
    dae_profile_log(PROFILE, NAME, _dae_profile_call_start);                \
  } while (0)

static
void
dae_lib(DAEState   * __restrict dst,
        xml_t      * __restrict xml,
        const char * __restrict name,
        AkLoadLibraryItemFn     loadfn,
        void      ** __restrict dest,
        void      ** __restrict lastDest,
        uint32_t   * __restrict countDest,
        size_t                  nextOffset,
        DAELibrary ** __restrict librecDest);

AK_HIDE
AkResult
dae_doc(AkDoc     ** __restrict dest,
        const char * __restrict filepath) {
  AkHeap            *heap;
  AkDoc             *doc;
  const xml_doc_t   *xdoc;
  xml_t             *xml, *assetEl;
  AkAssetInf        *inf;
  xml_attr_t        *versionAttr;
  void              *xmlString;
  FListItem         *freeUsrData;
  DAEState           dstVal, *dst;
  size_t             xmlSize;
  AkResult           ret;
  double             totalStart, stepStart;
  bool               profile;

  profile    = dae_profile_enabled();
  totalStart = dae_profile_now_ms();
  stepStart  = totalStart;
  if ((ret = ak_readfile(filepath, NULL, &xmlString, &xmlSize)) != AK_OK)
    return ret;
  dae_profile_log(profile, "readfile", stepStart);

  stepStart = dae_profile_now_ms();
  xdoc = xml_parse(xmlString, XML_PREFIXES | XML_READONLY);
  dae_profile_log(profile, "xml_parse", stepStart);
  if (!xdoc || !(xml = xdoc->root)) {
    if (xdoc)
      free((void *)xdoc);
    ak_releasefile(xmlString, xmlSize);
    return AK_ERR;
  }

  stepStart = dae_profile_now_ms();
  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name      = filepath;
  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage = true;
  doc->inf->ftype     = AK_FILE_TYPE_COLLADA;
  doc->coordSys       = AK_YUP; /* Default */

  /* for fixing skin and morph vertices */
  doc->reserved = rb_newtree_ptr();
  ((RBTree *)doc->reserved)->onFreeNode = ak_daeFreeDupl;

  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);

  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  memset(&dstVal, 0, sizeof(dstVal));

  dstVal.doc          = doc;
  dstVal.heap         = heap;
  dstVal.profile      = profile;
  dstVal.tempmem      = ak_heap_alloc(heap, doc, sizeof(void*));
  dstVal.meshInfo     = rb_newtree_ptr();
  dstVal.inputmap     = rb_newtree_ptr();
  dstVal.texmap       = rb_newtree_ptr();
  dstVal.instanceMap  = rb_newtree_ptr();

  dstVal.meshTargets  = rb_newtree_ptr();

  dst                 = &dstVal;

  dstVal.texmap->userData = dst;
  dae_profile_log(profile, "setup", stepStart);

  /* get version info */
  /* because it is current and most used version */
  dst->version = AK_COLLADA_VERSION_141;
  if ((versionAttr = DAE_XMLA8(xml, version))) {
    ak_enumpair *v;

    for (v = daeVersions; v->key; v++) {
      if (!strncmp(v->key, versionAttr->val, versionAttr->valsize)) {
        dst->version = v->val;
        break;
      }
    }
  }
  
  assetEl = NULL;
  xml     = xml->val;

  /* with default Asset Parameters */
  stepStart = dae_profile_now_ms();
  assetEl = DAE_XML_ELEM8(xml->parent, asset);
  if ((inf = dae_asset(dst, assetEl, doc, &doc->inf->base))) {
    doc->coordSys = inf->coordSys;
    doc->unit     = inf->unit;
  }
  dae_profile_log(profile, "asset", stepStart);
  
  stepStart = dae_profile_now_ms();
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, lib_cameras)) {
      DAE_PROFILE_CALL(profile,
                       "library_cameras",
                       dae_lib(dst, xml, _s_dae_camera, dae_cam,
                               (void **)&doc->lib.cameras.first,
                               (void **)&doc->lib.cameras.last,
                               &doc->lib.cameras.count,
                               offsetof(AkCamera, next),
                               NULL));
    } else if (DAE_XML_TAG_EQ(xml, lib_lights)) {
      DAE_PROFILE_CALL(profile,
                       "library_lights",
                       dae_lib(dst, xml, _s_dae_light, dae_light,
                               (void **)&doc->lib.lights.first,
                               (void **)&doc->lib.lights.last,
                               &doc->lib.lights.count,
                               offsetof(AkLight, next),
                               NULL));
    } else if (DAE_XML_TAG_EQ(xml, lib_geometries)) {
      DAE_PROFILE_CALL(profile,
                       "library_geometries",
                       dae_lib(dst, xml, _s_dae_geometry, dae_geom,
                               (void **)&doc->lib.geometries.first,
                               (void **)&doc->lib.geometries.last,
                               &doc->lib.geometries.count,
                               offsetof(AkGeometry, next),
                               NULL));
      dae_profile_log_geometry(dst);
    } else if (DAE_XML_TAG_EQ(xml, lib_effects)) {
      DAE_PROFILE_CALL(profile,
                       "library_effects",
                       dae_lib(dst, xml, _s_dae_effect, dae_effect,
                               (void **)&dst->effects,
                               NULL,
                               NULL,
                               offsetof(AkEffect, next),
                               &dst->effectLibraries));
    } else if (DAE_XML_TAG_EQ(xml, lib_images)) {
      DAE_PROFILE_CALL(profile,
                       "library_images",
                       dae_lib(dst, xml, _s_dae_image, dae_image,
                               (void **)&doc->lib.images.first,
                               (void **)&doc->lib.images.last,
                               &doc->lib.images.count,
                               offsetof(AkImage, next),
                               NULL));
    } else if (DAE_XML_TAG_EQ(xml, lib_materials)) {
      DAE_PROFILE_CALL(profile,
                       "library_materials",
                       dae_lib(dst, xml, _s_dae_material, dae_material,
                               (void **)&doc->lib.materials.first,
                               (void **)&doc->lib.materials.last,
                               &doc->lib.materials.count,
                               offsetof(AkMaterial, next),
                               NULL));
    } else if (DAE_XML_TAG_EQ(xml, lib_controllers)) {
      DAE_PROFILE_CALL(profile,
                       "library_controllers",
                       dae_lib(dst, xml, _s_dae_controller, dae_ctlr,
                               (void **)&dst->controllers,
                               NULL,
                               NULL,
                               offsetof(AkController, next),
                               &dst->controllerLibraries));
    } else if (DAE_XML_TAG_EQ(xml, lib_visual_scenes)) {
      DAE_PROFILE_CALL(profile,
                       "library_visual_scenes",
                       dae_lib(dst, xml, _s_dae_visual_scene, dae_vscene,
                               (void **)&doc->lib.scenes.first,
                               (void **)&doc->lib.scenes.last,
                               &doc->lib.scenes.count,
                               offsetof(AkScene, next),
                               NULL));
    } else if (DAE_XML_TAG_EQ(xml, lib_nodes)) {
      DAE_PROFILE_CALL(profile,
                       "library_nodes",
                       dae_lib(dst, xml, _s_dae_node, dae_node2,
                               (void **)&doc->lib.nodes.first,
                               (void **)&doc->lib.nodes.last,
                               &doc->lib.nodes.count,
                               offsetof(AkNode, docNext),
                               &dst->nodeLibraries));
    } else if (DAE_XML_TAG_EQ(xml, lib_animations)) {
      DAE_PROFILE_CALL(profile,
                       "library_animations",
                       dae_lib(dst, xml, _s_dae_animation, dae_anim,
                               (void **)&doc->lib.animations.first,
                               (void **)&doc->lib.animations.last,
                               &doc->lib.animations.count,
                               offsetof(AkAnimation, next),
                               NULL));
    } else if (DAE_XML_TAG_EQ8(xml, scene)) {
      DAE_PROFILE_CALL(profile, "scene", dae_scene(dst, xml));
    }
    xml = xml->next;
  }
  dae_profile_log(profile, "libraries", stepStart);

  *dest = doc;

  /* post-parse operations */
  stepStart = dae_profile_now_ms();
  dae_postscript(dst);
  dae_profile_log(profile, "postscript", stepStart);

  /* cleanup up details */
  stepStart = dae_profile_now_ms();
  freeUsrData = dst->linkedUserData;
  while (freeUsrData) {
    void *tofree;

    if ((tofree = ak_userData(freeUsrData->data)))
      ak_free(tofree);

    ak_heap_ext_rm(heap, ak__alignof(freeUsrData->data), AK_HEAP_NODE_FLAGS_USR);
    freeUsrData = freeUsrData->next;
  }

  ak_free(dstVal.tempmem);

  flist_sp_destroy(&dst->linkedUserData);

  rb_destroy(dstVal.meshInfo);
  rb_destroy(dstVal.inputmap);
  rb_destroy(dstVal.texmap);
  rb_destroy(dstVal.instanceMap);

  flist_sp_destroy(&dstVal.vertMap);

  rb_destroy(dstVal.meshTargets);

  if (xdoc)
    free((void *)xdoc);
  
  if (xmlString)
    ak_releasefile(xmlString, xmlSize);

  /* TODO: memory leak, free this RBTree*/
  /* rb_destroy(doc->reserved); */
  dae_profile_log(profile, "cleanup", stepStart);
  dae_profile_log(profile, "total", totalStart);

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
void
dae_lib(DAEState   * __restrict dst,
        xml_t      * __restrict xml,
        const char * __restrict name,
        AkLoadLibraryItemFn     loadfn,
        void      ** __restrict dest,
        void      ** __restrict lastDest,
        uint32_t   * __restrict countDest,
        size_t                  nextOffset,
        DAELibrary ** __restrict librecDest) {
  AkHeap     *heap;
  DAELibrary *lib;
  char       *item, *tail;
  size_t      namesize;
  
  heap      = dst->heap;
  namesize  = strlen(name);

  lib       = ak_heap_calloc(heap, dst->doc, sizeof(*lib));
  lib->name = DAE_XMLA_STRDUP8(xml, heap, name, lib);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eqsz(xml, name, namesize)) {
      if ((item = loadfn(dst, xml, lib))) {
        *(void **)(item + nextOffset) = lib->first;
        lib->first = item;
        lib->count++;
      }
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      lib->extra = tree_fromxml(heap, lib, xml);
    }
    xml = xml->next;
  }

  if (lib->first) {
    tail = lib->first;
    while (*(void **)(tail + nextOffset))
      tail = *(void **)(tail + nextOffset);
    *(void **)(tail + nextOffset) = *dest;
    *dest = lib->first;
    if (lastDest && !*lastDest)
      *lastDest = tail;
    if (countDest)
      *countDest += (uint32_t)lib->count;
  }

  if (librecDest) {
    lib->next   = *librecDest;
    *librecDest = lib;
  }
}
