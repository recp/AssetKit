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
#include "../../string_fast.h"

#include "../../../include/ak/path.h"

#include <stdlib.h>

typedef struct DAEVersionPair {
  const char *key;
  size_t      keyLen;
  int         val;
} DAEVersionPair;

static DAEVersionPair daeVersions[] = {
  {"1.5.0", sizeof("1.5.0") - 1u, AK_COLLADA_VERSION_150},
  {"1.5",   sizeof("1.5")   - 1u, AK_COLLADA_VERSION_150},
  {"1.4.1", sizeof("1.4.1") - 1u, AK_COLLADA_VERSION_141},
  {"1.4.0", sizeof("1.4.0") - 1u, AK_COLLADA_VERSION_140},
  {"1.4",   sizeof("1.4")   - 1u, AK_COLLADA_VERSION_140},
  {NULL, 0u, 0}
};

typedef void*(*AkLoadLibraryItemFn)(DAEState * __restrict dst,
                                    xml_t    * __restrict xml,
                                    void     * __restrict memp);
static bool
dae_xml_utf16(const void * __restrict data,
              size_t                  size,
              bool      * __restrict bigEndian,
              size_t    * __restrict byteOffset);

static char*
dae_xml_utf16_to_utf8(const void * __restrict data,
                      size_t                  size,
                      bool                    bigEndian,
                      size_t                  byteOffset,
                      size_t    * __restrict utf8Size);

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
  xml_doc_t         *xdoc;
  xml_t             *xml, *assetEl;
  AkAssetInf        *inf;
  xml_attr_t        *versionAttr;
  void              *xmlString;
  char              *xmlUtf8;
  const char        *xmlInput;
  FListItem         *freeUsrData;
  DAEState           dstVal, *dst;
  size_t             xmlSize;
  AkResult           ret;
  bool               xmlBigEndian;
  size_t             xmlByteOffset;

  if ((ret = ak_readfile(filepath, NULL, &xmlString, &xmlSize)) != AK_OK)
    return ret;

  xmlUtf8  = NULL;
  xmlInput = xmlString;
  if (dae_xml_utf16(xmlString, xmlSize, &xmlBigEndian, &xmlByteOffset)) {
    xmlUtf8 = dae_xml_utf16_to_utf8(xmlString,
                                    xmlSize,
                                    xmlBigEndian,
                                    xmlByteOffset,
                                    NULL);
    if (!xmlUtf8) {
      ak_releasefile(xmlString, xmlSize);
      return AK_ERR;
    }
    ak_releasefile(xmlString, xmlSize);
    xmlString = NULL;
    xmlSize   = 0;
    xmlInput  = xmlUtf8;
  }

  xdoc = xml_parse(xmlInput, XML_PREFIXES | XML_READONLY);
  if (!xdoc || !(xml = xdoc->root)) {
    if (xdoc)
      xml_free(xdoc);
    if (xmlUtf8)
      free(xmlUtf8);
    ak_releasefile(xmlString, xmlSize);
    return AK_ERR;
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name      = ak_heap_strdup(heap, doc->inf, filepath);
  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage = true;
  doc->inf->ftype     = AK_FILE_TYPE_COLLADA;
  doc->coordSys       = AK_YUP; /* Default */

  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);

  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  memset(&dstVal, 0, sizeof(dstVal));

  dstVal.doc          = doc;
  dstVal.heap         = heap;
  dstVal.tempmem      = ak_heap_alloc(heap, doc, sizeof(void*));
  dstVal.meshInfo     = rb_newtree_ptr();
  dstVal.inputmap     = rb_newtree_ptr();
  dstVal.texmap       = rb_newtree_ptr();
  dstVal.instanceMap  = rb_newtree_ptr();
  dstVal.materialEffectMap = rb_newtree_ptr();

  dstVal.meshTargets  = rb_newtree_ptr();

  dst                 = &dstVal;

  dstVal.texmap->userData = dst;

  /* get version info */
  /* because it is current and most used version */
  dst->version = AK_COLLADA_VERSION_141;
  if ((versionAttr = DAE_XMLA8(xml, version))) {
    DAEVersionPair *v;

    for (v = daeVersions; v->key; v++) {
      if (ak_str_eq_fast(versionAttr->val,
                         versionAttr->valsize,
                         v->key,
                         v->keyLen)) {
        dst->version = v->val;
        break;
      }
    }
  }
  
  assetEl = NULL;
  xml     = xml->val;

  /* with default Asset Parameters */
  assetEl = DAE_XML_ELEM8(xml->parent, asset);
  if ((inf = dae_asset(dst, assetEl, doc, &doc->inf->base))) {
    doc->coordSys = inf->coordSys;
    doc->unit     = inf->unit;
  }
  
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, lib_cameras)) {
      dae_lib(dst, xml, _s_dae_camera, dae_cam,
              (void **)&doc->lib.cameras.first,
              (void **)&doc->lib.cameras.last,
              &doc->lib.cameras.count,
              offsetof(AkCamera, next),
              NULL);
    } else if (DAE_XML_TAG_EQ(xml, lib_lights)) {
      dae_lib(dst, xml, _s_dae_light, dae_light,
              (void **)&doc->lib.lights.first,
              (void **)&doc->lib.lights.last,
              &doc->lib.lights.count,
              offsetof(AkLight, next),
              NULL);
    } else if (DAE_XML_TAG_EQ(xml, lib_geometries)) {
      dae_lib(dst, xml, _s_dae_geometry, dae_geom,
              (void **)&doc->lib.geometries.first,
              (void **)&doc->lib.geometries.last,
              &doc->lib.geometries.count,
              offsetof(AkGeometry, next),
              NULL);
    } else if (DAE_XML_TAG_EQ(xml, lib_effects)) {
      dae_lib(dst, xml, _s_dae_effect, dae_effect,
              (void **)&dst->effects,
              NULL,
              NULL,
              offsetof(AkEffect, next),
              &dst->effectLibraries);
    } else if (DAE_XML_TAG_EQ(xml, lib_images)) {
      dae_lib(dst, xml, _s_dae_image, dae_image,
              (void **)&doc->lib.images.first,
              (void **)&doc->lib.images.last,
              &doc->lib.images.count,
              offsetof(AkImage, next),
              NULL);
    } else if (DAE_XML_TAG_EQ(xml, lib_materials)) {
      dae_lib(dst, xml, _s_dae_material, dae_material,
              (void **)&doc->lib.materials.first,
              (void **)&doc->lib.materials.last,
              &doc->lib.materials.count,
              offsetof(AkMaterial, next),
              NULL);
    } else if (DAE_XML_TAG_EQ(xml, lib_controllers)) {
      dae_lib(dst, xml, _s_dae_controller, dae_ctlr,
              (void **)&dst->controllers,
              NULL,
              NULL,
              offsetof(AkController, next),
              &dst->controllerLibraries);
    } else if (DAE_XML_TAG_EQ(xml, lib_visual_scenes)) {
      dae_lib(dst, xml, _s_dae_visual_scene, dae_vscene,
              (void **)&doc->lib.scenes.first,
              (void **)&doc->lib.scenes.last,
              &doc->lib.scenes.count,
              offsetof(AkScene, next),
              NULL);
    } else if (DAE_XML_TAG_EQ(xml, lib_nodes)) {
      dae_lib(dst, xml, _s_dae_node, dae_node2,
              (void **)&doc->lib.nodes.first,
              (void **)&doc->lib.nodes.last,
              &doc->lib.nodes.count,
              offsetof(AkNode, docNext),
              &dst->nodeLibraries);
    } else if (DAE_XML_TAG_EQ(xml, lib_animations)) {
      dae_lib(dst, xml, _s_dae_animation, dae_anim,
              (void **)&doc->lib.animations.first,
              (void **)&doc->lib.animations.last,
              &doc->lib.animations.count,
              offsetof(AkAnimation, next),
              NULL);
    } else if (DAE_XML_TAG_EQ8(xml, scene)) {
      dae_scene(dst, xml);
    }
    xml = xml->next;
  }

  *dest = doc;

  /* post-parse operations */
  dae_postscript(dst);

  /* cleanup up details */
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
  flist_sp_destroy(&dst->bindMaterials);

  rb_destroy(dstVal.meshInfo);
  rb_destroy(dstVal.inputmap);
  rb_destroy(dstVal.texmap);
  rb_destroy(dstVal.instanceMap);
  rb_destroy(dstVal.materialEffectMap);

  flist_sp_destroy(&dstVal.vertMap);

  rb_destroy(dstVal.meshTargets);

  if (xdoc)
    xml_free(xdoc);
  
  if (xmlString)
    ak_releasefile(xmlString, xmlSize);
  if (xmlUtf8)
    free(xmlUtf8);

  return AK_OK;
}

static bool
dae_xml_utf16(const void * __restrict data,
              size_t                  size,
              bool      * __restrict bigEndian,
              size_t    * __restrict byteOffset) {
  const unsigned char *bytes;

  if (!data || size < 4 || !bigEndian || !byteOffset)
    return false;

  bytes = data;
  if (bytes[0] == 0xff && bytes[1] == 0xfe) {
    *bigEndian  = false;
    *byteOffset = 2;
    return true;
  }

  if (bytes[0] == 0xfe && bytes[1] == 0xff) {
    *bigEndian  = true;
    *byteOffset = 2;
    return true;
  }

  if (bytes[0] == '<' && bytes[1] == 0
      && bytes[2] == '?' && bytes[3] == 0) {
    *bigEndian  = false;
    *byteOffset = 0;
    return true;
  }

  if (bytes[0] == 0 && bytes[1] == '<'
      && bytes[2] == 0 && bytes[3] == '?') {
    *bigEndian  = true;
    *byteOffset = 0;
    return true;
  }

  return false;
}

static uint16_t
dae_xml_read_u16(const unsigned char * __restrict it, bool bigEndian) {
  if (bigEndian)
    return ((uint16_t)it[0] << 8) | (uint16_t)it[1];
  return ((uint16_t)it[1] << 8) | (uint16_t)it[0];
}

static char*
dae_xml_utf16_to_utf8(const void * __restrict data,
                      size_t                  size,
                      bool                    bigEndian,
                      size_t                  byteOffset,
                      size_t    * __restrict utf8Size) {
  const unsigned char *bytes;
  char                *utf8;
  size_t               i, units, cap, out;

  if (!data || byteOffset > size || ((size - byteOffset) & 1u))
    return NULL;

  units = (size - byteOffset) / 2u;
  if (units > ((size_t)-1 - 1u) / 4u)
    return NULL;

  cap  = units * 4u + 1u;
  utf8 = malloc(cap);
  if (!utf8)
    return NULL;

  bytes = (const unsigned char *)data + byteOffset;
  out   = 0;
  for (i = 0; i < units; i++) {
    uint32_t codepoint;
    uint16_t u;

    u = dae_xml_read_u16(bytes + i * 2u, bigEndian);

    if (u >= 0xd800u && u <= 0xdbffu) {
      uint16_t lo;

      if (++i >= units)
        goto err;

      lo = dae_xml_read_u16(bytes + i * 2u, bigEndian);
      if (lo < 0xdc00u || lo > 0xdfffu)
        goto err;

      codepoint = 0x10000u
                  + (((uint32_t)u - 0xd800u) << 10)
                  + ((uint32_t)lo - 0xdc00u);
    } else if (u >= 0xdc00u && u <= 0xdfffu) {
      goto err;
    } else {
      codepoint = u;
    }

    if (codepoint <= 0x7fu) {
      utf8[out++] = (char)codepoint;
    } else if (codepoint <= 0x7ffu) {
      utf8[out++] = (char)(0xc0u | (codepoint >> 6));
      utf8[out++] = (char)(0x80u | (codepoint & 0x3fu));
    } else if (codepoint <= 0xffffu) {
      utf8[out++] = (char)(0xe0u | (codepoint >> 12));
      utf8[out++] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
      utf8[out++] = (char)(0x80u | (codepoint & 0x3fu));
    } else {
      utf8[out++] = (char)(0xf0u | (codepoint >> 18));
      utf8[out++] = (char)(0x80u | ((codepoint >> 12) & 0x3fu));
      utf8[out++] = (char)(0x80u | ((codepoint >> 6) & 0x3fu));
      utf8[out++] = (char)(0x80u | (codepoint & 0x3fu));
    }
  }

  utf8[out] = '\0';
  if (utf8Size)
    *utf8Size = out;
  return utf8;

err:
  free(utf8);
  return NULL;
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
