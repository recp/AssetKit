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

#ifndef dae_common_h
#define dae_common_h

#include "../../../include/ak/assetkit.h"
#include "../../../include/ak/path.h"
#include "../../../include/ak/url.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../xml.h"
#include "strpool.h"

#include <ds/forward-list-sep.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <xml/xml.h>
#include <xml/attrib.h>

#include "bugfix/url.h"

#ifdef _MSC_VER
#  ifndef PATH_MAX
#    define PATH_MAX 260
#  endif
#  define DAE_URL_HELPER static AK_NOINLINE
#else
#  define DAE_URL_HELPER AK_INLINE
#endif

#define DAE_XML_TAG_EQ8(XML, NAME)                                           \
  xml_tag_eq_packed(XML, _s_dae_##NAME##_u64_exact, _s_dae_##NAME##_len)

#define DAE_XML_TAG_EQ4(XML, NAME)                                           \
  xml_tag_eq_packed4(XML, _s_dae_##NAME##_u32_exact, _s_dae_##NAME##_len)

#define DAE_XML_VAL_EQ8(XML, NAME)                                           \
  xml_val_eq_packed(XML, _s_dae_##NAME##_u64_exact, _s_dae_##NAME##_len)

#define DAE_XML_VAL_EQ4(XML, NAME)                                           \
  xml_val_eq_packed4(XML, _s_dae_##NAME##_u32_exact, _s_dae_##NAME##_len)

#define DAE_XML_TAG_EQ(XML, NAME)                                            \
  xml_tag_eqsz(XML, _s_dae_##NAME, _s_dae_##NAME##_len)

#define DAE_XML_VAL_EQ(XML, NAME)                                            \
  xml_val_eqsz(XML, _s_dae_##NAME, _s_dae_##NAME##_len)

#define DAE_XMLA(XML, NAME)                                                  \
  xmla_sz(XML, _s_dae_##NAME, _s_dae_##NAME##_len)

#define DAE_XMLA8(XML, NAME)                                                 \
  xmla_packed(XML, _s_dae_##NAME##_u64_exact, _s_dae_##NAME##_len)

#define DAE_XMLA4(XML, NAME)                                                 \
  xmla_packed4(XML, _s_dae_##NAME##_u32_exact, _s_dae_##NAME##_len)

#define DAE_XML_ELEM(XML, NAME)                                              \
  xml_elem_sz(XML, _s_dae_##NAME, _s_dae_##NAME##_len)

#define DAE_XML_ELEM8(XML, NAME)                                             \
  xml_elem_packed(XML, _s_dae_##NAME##_u64_exact, _s_dae_##NAME##_len)

#define DAE_XML_ELEM4(XML, NAME)                                             \
  xml_elem_packed4(XML, _s_dae_##NAME##_u32_exact, _s_dae_##NAME##_len)

AK_INLINE
char *
dae_xmla_strdup_bysz(const xml_t * __restrict xobject,
                     AkHeap      * __restrict heap,
                     const char  * __restrict name,
                     size_t                   namesize,
                     void        * __restrict parent) {
  xml_attr_t *att;

  if ((att = xmla_sz(xobject, name, namesize)))
    return ak_heap_strndup(heap, parent, att->val, att->valsize);

  return NULL;
}

AK_INLINE
char *
dae_xmla_strdup_by4(const xml_t * __restrict xobject,
                    AkHeap      * __restrict heap,
                    uint32_t                 packed,
                    size_t                   namesize,
                    void        * __restrict parent) {
  xml_attr_t *att;

  if ((att = xmla_packed4(xobject, packed, namesize)))
    return ak_heap_strndup(heap, parent, att->val, att->valsize);

  return NULL;
}

AK_INLINE
char *
dae_xmla_strdup_by8(const xml_t * __restrict xobject,
                    AkHeap      * __restrict heap,
                    uint64_t                 packed,
                    size_t                   namesize,
                    void        * __restrict parent) {
  xml_attr_t *att;

  if ((att = xmla_packed(xobject, packed, namesize)))
    return ak_heap_strndup(heap, parent, att->val, att->valsize);

  return NULL;
}

#define DAE_XMLA_STRDUP(XML, HEAP, NAME, PARENT)                             \
  dae_xmla_strdup_bysz(XML, HEAP, _s_dae_##NAME, _s_dae_##NAME##_len, PARENT)

#define DAE_XMLA_STRDUP8(XML, HEAP, NAME, PARENT)                            \
  dae_xmla_strdup_by8(XML, HEAP, _s_dae_##NAME##_u64_exact,                  \
                      _s_dae_##NAME##_len, PARENT)

#define DAE_XMLA_STRDUP4(XML, HEAP, NAME, PARENT)                            \
  dae_xmla_strdup_by4(XML, HEAP, _s_dae_##NAME##_u32_exact,                  \
                      _s_dae_##NAME##_len, PARENT)

#ifndef AK_INPUT_SEMANTIC_VERTEX 
#  define AK_INPUT_SEMANTIC_VERTEX 100001
#endif

#define DAE_TYPE_SOURCE ((AkTypeId)0xfffeu)

AK_INLINE
AkInput*
dae_input_new(AkHeap * __restrict heap, void * __restrict parent) {
  return ak_heap_calloc(heap, parent, sizeof(AkInput));
}

typedef enum AkCOLLADAVersion {
  AK_COLLADA_VERSION_150 = 150,
  AK_COLLADA_VERSION_141 = 141,
  AK_COLLADA_VERSION_140 = 140
} AkCOLLADAVersion;

typedef enum AkControllerType {
  AK_CONTROLLER_MORPH = 1,
  AK_CONTROLLER_SKIN  = 2
} AkControllerType;

typedef struct AkURLQueue {
  AkURL *url;
  AkScene *scene;
  AkInstanceBase *instance;
  AkInstanceNode *nodeRef;
  struct AkURLQueue *next;
} AkURLQueue;

typedef struct AkDAEVerticesMapItem {
  AkInput         *inp;
  AkMeshPrimitive *prim;
  AkVertices      *fallbackVertices;
  uint32_t         indexOffset;
} AkDAEVerticesMapItem;

typedef struct AkDAEBindMaterialUse {
  AkInstanceGeometry *instance;
  AkBindMaterial     *bindMaterial;
} AkDAEBindMaterialUse;

typedef struct AkDAEMaterialEffectRecord {
  AkMaterial       *material;
  AkInstanceEffect *effects;
} AkDAEMaterialEffectRecord;

typedef struct DaeSource {
  const char       *name;
  AkBuffer         *buffer;
  AkAccessor       *accessor;
  void             *reserved;
  struct DaeSource *next;
  int32_t           target;
} DaeSource;

typedef struct DaeFloatParseJob {
  const xml_t *source;
  AkBuffer    *buffer;
  size_t       sourceByteCount;
  uint32_t     workerIndex;
} DaeFloatParseJob;

typedef struct DaeIndexParseJob {
  const xml_t     *source;
  AkMeshPrimitive *primitive;
  void            *parent;
  size_t           scratchOffset;
  size_t           sequence;
  size_t           sourceByteCount;
  unsigned long    count;
  unsigned long    remaining;
  AkUInt           maxValue;
  uint32_t         workerIndex;
} DaeIndexParseJob;

typedef struct DAELibrary {
  struct DAELibrary *next;
  const char        *name;
  AkTree            *extra;
  void              *first;
  uint64_t           count;
} DAELibrary;

typedef AK_ALIGN(16) struct DAEState {
  AkHeap          *heap;
  void            *tempmem;
  void            *jobs14;
  AkDoc           *doc;
  AkURLQueue      *urlQueue;
  FListItem       *accessors;
  FListItem       *instCtlrs;
  FListItem       *bindMaterials;
  FListItem       *inputs;
  FListItem       *toRadiansSampelers;
  FListItem       *linkedUserData;
  AkURL            activeScene;
  RBTree          *meshInfo;
  RBTree          *inputmap;
  RBTree          *texmap;
  RBTree          *instanceMap;
  RBTree          *materialEffectMap;
  FListItem       *vertMap;
  DaeFloatParseJob *floatParseJobs;
  DaeIndexParseJob *indexParseJobs;
  DAELibrary      *effectLibraries;
  DAELibrary      *controllerLibraries;
  DAELibrary      *nodeLibraries;
  AkEffect        *effects;
  struct AkController *controllers;
  /* maps base AkGeometry* → AkMorph*. Populated by dae_fixup_ctlr's
     MORPH case so that the postscript orphan-attach pass can wrap
     <instance_geometry> uses of the base mesh in an AkInstanceMorph
     (DAE exporters — especially glTF→DAE — frequently emit a morph
     controller without ever wrapping the geometry in
     <instance_controller>, leaving the morph dangling otherwise). */
  RBTree          *meshTargets;
  DaeSource       *sources;
  uint64_t         floatValueCount;
  uint64_t         indexValueCount;
  size_t           floatParseJobCount;
  size_t           floatParseJobCapacity;
  size_t           indexParseJobCount;
  size_t           indexParseJobCapacity;
  AkCOLLADAVersion version;
  bool             stop;
} DAEState;

AK_INLINE
void
dae_url_mark_scene_node_ref(DAEState       * __restrict dst,
                            AkURL          * __restrict url,
                            AkScene        * __restrict scene,
                            AkInstanceNode * __restrict nodeRef) {
  if (!dst || !dst->urlQueue || dst->urlQueue->url != url)
    return;

  dst->urlQueue->scene   = scene;
  dst->urlQueue->nodeRef = nodeRef;
}

AK_INLINE
void
dae_url_mark_scene_instance(DAEState       * __restrict dst,
                            AkURL          * __restrict url,
                            AkScene        * __restrict scene,
                            AkInstanceBase * __restrict instance) {
  if (!dst || !dst->urlQueue || dst->urlQueue->url != url)
    return;

  dst->urlQueue->scene    = scene;
  dst->urlQueue->instance = instance;
}

AK_INLINE
void
dae_bind_material_add(DAEState          * __restrict dst,
                      AkInstanceGeometry * __restrict instance,
                      AkBindMaterial     * __restrict bindMaterial) {
  AkDAEBindMaterialUse *item;

  if (!dst || !instance || !bindMaterial)
    return;

  item               = ak_heap_calloc(dst->heap, dst->tempmem, sizeof(*item));
  item->instance     = instance;
  item->bindMaterial = bindMaterial;
  flist_sp_insert(&dst->bindMaterials, item);
}

AK_INLINE
AkDAEMaterialEffectRecord*
dae_material_effect_record(DAEState  * __restrict dst,
                           AkMaterial * __restrict material) {
  if (!dst || !material)
    return NULL;

  return dst->materialEffectMap ? rb_find(dst->materialEffectMap, material) : NULL;
}

AK_INLINE
AkInstanceEffect*
dae_material_instance_effect(DAEState  * __restrict dst,
                             AkMaterial * __restrict material) {
  AkDAEMaterialEffectRecord *record;

  record = dae_material_effect_record(dst, material);
  return record ? record->effects : NULL;
}

AK_INLINE
AkEffect*
dae_material_effect(DAEState  * __restrict dst,
                    AkMaterial * __restrict material) {
  AkInstanceEffect *instEffect;

  instEffect = dae_material_instance_effect(dst, material);
  if (!instEffect)
    return NULL;

  return ak_instanceObject(&instEffect->base);
}

AK_INLINE
void
dae_material_effect_add(DAEState         * __restrict dst,
                        AkMaterial       * __restrict material,
                        AkInstanceEffect * __restrict instEffect) {
  AkDAEMaterialEffectRecord *record;
  AkInstanceEffect          *head;

  if (!dst || !dst->materialEffectMap || !material || !instEffect)
    return;

  record = dae_material_effect_record(dst, material);
  if (!record) {
    record           = ak_heap_calloc(dst->heap, dst->tempmem, sizeof(*record));
    record->material = material;
    rb_insert(dst->materialEffectMap, material, record);
  }

  head = record->effects;
  if (head) {
    head->base.prev       = &instEffect->base;
    instEffect->base.next = &head->base;
  }

  record->effects = instEffect;
}

typedef struct AkDaeMeshInfo {
  AkInput *pos;
  size_t   nVertex;
} AkDaeMeshInfo;

typedef struct AkDAETextureRef {
  const char          *texture;
  const char          *texcoord;
  AkTextureColorSpace  colorSpace;
  AkTextureChannels    channels;
} AkDAETextureRef;

typedef struct AkNewParam {
  /* const char * sid; */
  struct AkNewParam *prev;
  struct AkNewParam *next;
  const char        *semantic;
  AkValue           *val;
} AkNewParam;

typedef struct AkDataParam {
  /* const char * sid; */
  struct AkDataParam *next;
  const char         *name;
  const char         *semantic;
  AkTypeDesc          type;
} AkDataParam;

typedef struct AkController {
  /* const char * id; */
  struct AkController *next;
  const char          *name;
  void                *data;
  AkTree              *extra;
  AkControllerType     type;
} AkController;

typedef struct AkInstanceController {
  AkInstanceBase    base;
  AkURL             geometry;
  struct AkNode   **joints;
  AkBindMaterial   *bindMaterial;
  struct FListItem *reserved;
} AkInstanceController;

typedef struct AkAccessorDAE {
  struct AkDataParam *param;
  AkURL               source;
  size_t              offset;
  uint32_t            stride;
  uint32_t            bound;
} AkAccessorDAE;

typedef struct AkSkinJointsDAE {
  AkInput *joints;
  AkInput *invBindMatrix;
} AkSkinJointsDAE;

typedef struct AkSkinWeightsDAE {
  AkInput     *joints;
  AkInput     *weights;
  AkUIntArray *v;
} AkSkinWeightsDAE;

typedef struct AkSkinDAE {
  AkURL            baseGeom;
  DaeSource       *source;
  AkTree          *extra;

  AkSkinJointsDAE  joints;
  AkSkinWeightsDAE weights;
  uint32_t         inputCount;
} AkSkinDAE;

typedef struct AkMorphDAE {
  AkURL     baseGeom;
  DaeSource *source;
  AkTree   *extra;
  AkInput  *input;
} AkMorphDAE;

AK_INLINE
void
sid_seta(xml_t  * __restrict xml,
         AkHeap * __restrict heap,
         void   * __restrict memnode,
         void   * __restrict memptr) {
  const char *sid;

  if (!(sid = DAE_XMLA_STRDUP4(xml, heap, sid, memnode)))
    return;

  ak_sid_seta(memnode, memptr, sid);
}

AK_INLINE
void
sid_set(xml_t  * __restrict xml,
        AkHeap * __restrict heap,
        void   * __restrict memnode) {
  const char *sid;
  
  if (!(sid = DAE_XMLA_STRDUP4(xml, heap, sid, memnode)))
    return;
  
  ak_sid_set(memnode, sid);
}

AK_INLINE
void
dae_url_init_from_attr(DAEState         * __restrict dst,
                       void             * __restrict memp,
                       const xml_attr_t * __restrict att,
                       AkURL            * __restrict url) {
  AkHeap     *heap;
  const char *val;
  char       *urlstr;
  size_t      len;

  heap = dst->heap;

  if (!att || !(val = att->val)) {
    url->reserved = NULL;
    url->url      = NULL;
    return;
  }

  len = att->valsize;

  /* Internal DAE references are the common hot path. ak_url_init() duplicates
     its '#' input, so avoid first making a heap duplicate of the XML slice. */
  if (len > 0 && val[0] == '#') {
    urlstr = dae_alloca(len + 1);
    memcpy(urlstr, val, len);
    urlstr[len] = '\0';

    ak_url_init(memp, urlstr, url);
    return;
  }

  if (!memchr(val, '/', len)
      && !memchr(val, '\\', len)
      && !memchr(val, '#', len)) {
    const char *it, *end;

    if (len >= 3) {
      it  = val;
      end = val + len - 2;
      while (it < end) {
        if (it[0] == ':' && it[1] == '/' && it[2] == '/')
          goto external_url;
        it++;
      }
    }

    urlstr    = dae_alloca(len + 2);
    urlstr[0] = '#';
    memcpy(urlstr + 1, val, len);
    urlstr[len + 1] = '\0';

    ak_url_init(memp, urlstr, url);
    return;
  }

external_url:
  urlstr = NULL;
  if (dst->doc
      && dst->doc->inf
      && dst->doc->inf->dir
      && dst->doc->inf->dir[0]
      && len > 0
      && val[0] != '/'
      && val[0] != '\\'
      && !(len >= 3
           && ((val[0] >= 'A' && val[0] <= 'Z')
               || (val[0] >= 'a' && val[0] <= 'z'))
           && val[1] == ':')) {
    const char *it, *end;
    bool        hasScheme;

    hasScheme = false;
    if (len >= 3) {
      it  = val;
      end = val + len - 2;
      while (it < end) {
        if (it[0] == ':' && it[1] == '/' && it[2] == '/') {
          hasScheme = true;
          break;
        }
        it++;
      }
    }

    if (!hasScheme) {
      char       pathbuf[PATH_MAX];
#ifndef _WIN32
      char       realbuf[PATH_MAX];
#endif
      char      *rel;
      char      *dststr;
      const char *frag;
      const char *path;
      const char *resolved;
      size_t      pathLen;
      size_t      fragLen;
      size_t      resolvedLen;

      frag    = memchr(val, '#', len);
      pathLen = frag ? (size_t)(frag - val) : len;
      fragLen = len - pathLen;

      rel = dae_alloca(pathLen + 1u);
      memcpy(rel, val, pathLen);
      rel[pathLen] = '\0';

      path = ak_fullpathn(dst->doc, rel, pathbuf, sizeof(pathbuf));
      if (path) {
        resolved = path;
#ifndef _WIN32
        if (realpath(path, realbuf))
          resolved = realbuf;
        if (fragLen > 0 && dst->doc->inf->name) {
          char        docRealbuf[PATH_MAX];
          const char *docPath;
          char       *localRef;

          docPath = dst->doc->inf->name;
          if (realpath(docPath, docRealbuf))
            docPath = docRealbuf;

          if (strcmp(resolved, docPath) == 0) {
            localRef = dae_alloca(fragLen + 1u);
            memcpy(localRef, frag, fragLen);
            localRef[fragLen] = '\0';
            ak_url_init(memp, localRef, url);
            return;
          }
        }
#endif

        resolvedLen = strlen(resolved);
        dststr      = ak_heap_alloc(heap, memp, resolvedLen + fragLen + 1u);
        if (dststr) {
          memcpy(dststr, resolved, resolvedLen);
          if (fragLen > 0)
            memcpy(dststr + resolvedLen, frag, fragLen);
          dststr[resolvedLen + fragLen] = '\0';
          urlstr = dststr;
        }
      }
    }
  }

  if (!urlstr)
    urlstr = ak_heap_strndup(heap, memp, val, len);
  if (!urlstr) {
    url->reserved = NULL;
    url->url      = NULL;
    url->doc      = NULL;
    url->ptr      = NULL;
    return;
  }

  ak_url_init(memp, urlstr, url);
}

DAE_URL_HELPER
void
url_set_sz(DAEState   * __restrict dst,
           xml_t      * __restrict xml,
           const char * __restrict name,
           size_t                  namesize,
           void       * __restrict memp,
           AkURL      * __restrict url) {
  AkURLQueue *urlQueue;
  xml_attr_t *att;

  if (!(att = xmla_sz(xml, name, namesize)) || !att->val) {
    url->reserved = NULL;
    url->url      = NULL;
    return;
  }

  dae_url_init_from_attr(dst, memp, att, url);

  urlQueue       = dst->heap->allocator->malloc(sizeof(*urlQueue));
  urlQueue->next     = dst->urlQueue;
  urlQueue->url      = url;
  urlQueue->scene    = NULL;
  urlQueue->instance = NULL;
  urlQueue->nodeRef  = NULL;
  dst->urlQueue      = urlQueue;
}

AK_INLINE
void
url_set(DAEState   * __restrict dst,
        xml_t      * __restrict xml,
        const char * __restrict name,
        void       * __restrict memp,
        AkURL      * __restrict url) {
  if (!name) {
    url->reserved = NULL;
    url->url      = NULL;
    return;
  }

  url_set_sz(dst, xml, name, strlen(name), memp, url);
}

#define DAE_URL_SET(DST, XML, NAME, MEMP, URL)                              \
  url_set_sz(DST, XML, _s_dae_##NAME, _s_dae_##NAME##_len, MEMP, URL)


AK_INLINE
void
dae_vertmap_add(DAEState     * __restrict dst,
                AkInput         * __restrict inp,
                AkMeshPrimitive * __restrict prim,
                AkVertices      * __restrict fallbackVertices) {
  AkDAEVerticesMapItem *item;

  if (!inp || !prim) { return; }

  item                   = ak_heap_calloc(dst->heap,
                                          dst->tempmem,
                                          sizeof(*item));
  item->inp              = inp;
  item->prim             = prim;
  item->fallbackVertices = fallbackVertices;
  item->indexOffset      = inp->indexOffset;

  flist_sp_insert(&dst->vertMap, item);
}

DAE_URL_HELPER
AkURL*
url_from_sz(DAEState   * __restrict dst,
            xml_t      * __restrict xml,
            const char * __restrict name,
            size_t                  namesize,
            void       * __restrict memp) {
  AkHeap     *heap;
  AkURL      *url;
  xml_attr_t *att;

  if (!(att = xmla_sz(xml, name, namesize)) || !att->val)
    return NULL;

  heap = ak_heap_getheap(memp);
  url  = ak_heap_calloc(heap, memp, sizeof(*url));

  dae_url_init_from_attr(dst, memp, att, url);

  return url;
}

AK_INLINE
AkURL*
url_from(DAEState   * __restrict dst,
         xml_t      * __restrict xml,
         const char * __restrict name,
         void       * __restrict memp) {
  if (!name)
    return NULL;

  return url_from_sz(dst, xml, name, strlen(name), memp);
}

#define DAE_URL_FROM(DST, XML, NAME, MEMP)                                   \
  url_from_sz(DST, XML, _s_dae_##NAME, _s_dae_##NAME##_len, MEMP)

AK_EXPORT
AkGeometry*
ak_baseGeometry(AkURL * __restrict baseurl);

#endif /* dae_common_h */
