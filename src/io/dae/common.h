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
#include "../../../include/ak/url.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../xml.h"
#include "strpool.h"

#include <ds/forward-list-sep.h>
#include <string.h>
#include <xml/xml.h>
#include <xml/attrib.h>

#include "bugfix/url.h"

#include <time.h>

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
  struct AkURLQueue *next;
} AkURLQueue;

typedef struct AkDAEVerticesMapItem {
  AkInput         *inp;
  AkMeshPrimitive *prim;
} AkDAEVerticesMapItem;

typedef struct DaeSource {
  const char       *name;
  AkBuffer         *buffer;
  AkAccessor       *accessor;
  void             *reserved;
  struct DaeSource *next;
  int32_t           target;
} DaeSource;

typedef AK_ALIGN(16) struct DAEState {
  AkHeap          *heap;
  void            *tempmem;
  void            *jobs14;
  AkDoc           *doc;
  AkURLQueue      *urlQueue;
  FListItem       *accessors;
  FListItem       *instCtlrs;
  FListItem       *inputs;
  FListItem       *toRadiansSampelers;
  FListItem       *linkedUserData;
  RBTree          *meshInfo;
  RBTree          *inputmap;
  RBTree          *texmap;
  RBTree          *instanceMap;
  FListItem       *vertMap;
  /* maps base AkGeometry* → AkMorph*. Populated by dae_fixup_ctlr's
     MORPH case so that the postscript orphan-attach pass can wrap
     <instance_geometry> uses of the base mesh in an AkInstanceMorph
     (DAE exporters — especially glTF→DAE — frequently emit a morph
     controller without ever wrapping the geometry in
     <instance_controller>, leaving the morph dangling otherwise). */
  RBTree          *meshTargets;
  DaeSource       *sources;
  AkCOLLADAVersion version;
  double           profGeom;
  double           profGeomMesh;
  double           profGeomSource;
  double           profGeomAccessor;
  double           profGeomArray;
  double           profGeomInput;
  double           profGeomIndexArray;
  double           profGeomVertices;
  double           profGeomTriangles;
  double           profGeomPolygons;
  double           profGeomLines;
  uint32_t         profGeomCount;
  uint32_t         profGeomMeshCount;
  uint32_t         profGeomSourceCount;
  uint32_t         profGeomAccessorCount;
  uint32_t         profGeomArrayCount;
  uint32_t         profGeomInputCount;
  uint32_t         profGeomIndexArrayCount;
  uint32_t         profGeomVerticesCount;
  uint32_t         profGeomTrianglesCount;
  uint32_t         profGeomPolygonsCount;
  uint32_t         profGeomLinesCount;
  bool             profile;
  bool             stop;
} DAEState;

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

typedef struct AkController {
  /* const char * id; */
  AkOneWayIterBase     base;
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
double
dae_profile_ms(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

#define DAE_PROF_START(DST)                                                  \
  ((DST) && (DST)->profile ? dae_profile_ms() : 0.0)

#define DAE_PROF_ACC(DST, FIELD, COUNT, START)                               \
  do {                                                                       \
    if ((DST) && (DST)->profile) {                                           \
      (DST)->FIELD += dae_profile_ms() - (START);                            \
      (DST)->COUNT++;                                                        \
    }                                                                        \
  } while (0)

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
dae_url_init_from_attr(AkHeap           * __restrict heap,
                       void             * __restrict memp,
                       const xml_attr_t * __restrict att,
                       AkURL            * __restrict url) {
  const char *val;
  char       *urlstr;
  size_t      len;

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

  if (!memchr(val, '/', len) && !memchr(val, '\\', len)) {
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
  urlstr = ak_heap_strndup(heap, memp, val, len);
  ak_url_init(memp, urlstr, url);
}

AK_INLINE
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

  dae_url_init_from_attr(dst->heap, memp, att, url);

  urlQueue       = dst->heap->allocator->malloc(sizeof(*urlQueue));
  urlQueue->next = dst->urlQueue;
  urlQueue->url  = url;
  dst->urlQueue  = urlQueue;
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
                AkMeshPrimitive * __restrict prim) {
  AkDAEVerticesMapItem *item;

  if (!inp || !prim) { return; }

  item       = ak_heap_calloc(dst->heap, dst->tempmem, sizeof(*item));
  item->inp  = inp;
  item->prim = prim;

  flist_sp_insert(&dst->vertMap, item);
}

AK_INLINE
AkURL*
url_from_sz(xml_t      * __restrict xml,
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

  dae_url_init_from_attr(heap, memp, att, url);

  return url;
}

AK_INLINE
AkURL*
url_from(xml_t      * __restrict xml,
         const char * __restrict name,
         void       * __restrict memp) {
  if (!name)
    return NULL;

  return url_from_sz(xml, name, strlen(name), memp);
}

#define DAE_URL_FROM(XML, NAME, MEMP)                                        \
  url_from_sz(XML, _s_dae_##NAME, _s_dae_##NAME##_len, MEMP)

AK_EXPORT
AkGeometry*
ak_baseGeometry(AkURL * __restrict baseurl);

#endif /* dae_common_h */
